#!/usr/bin/env python3

import os
import sys
import signal
import subprocess
import json
import fcntl
import select
import threading
from flask import Flask, request, Response, jsonify, send_from_directory, send_file

# 현재 실행되는 파이썬 파일의 디렉토리를 기준으로 절대경로 생성
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.join(BASE_DIR, "output")
INDEX_DIR = BASE_DIR

app = Flask(__name__)

# 환경변수 관리를 위한 전역 변수
ENV_INITIALIZED = False
CUSTOM_ENV = os.environ.copy()

# 실행 중인 프로세스 추적
current_process = None
process_lock = threading.Lock()

def parse_envset_sh():
    """envset.sh 파일을 파싱하여 환경변수를 추출합니다."""
    global ENV_INITIALIZED, CUSTOM_ENV
    
    try:
        envset_path = os.path.join(os.path.dirname(BASE_DIR), "envset.sh")
        
        if not os.path.exists(envset_path):
            return False, "envset.sh file not found"
        
        # 더 포괄적인 환경설정 스크립트 실행
        setup_script = f"""
        cd {os.path.dirname(BASE_DIR)}
        source envset.sh
        
        # ROOT 환경변수들이 제대로 설정되었는지 확인하고 출력
        echo "=== ENVIRONMENT VARIABLES ==="
        env | grep -E "(DYLD|LD_LIBRARY|PATH|ROOT|INSTALL_DIR)" | sort
        echo "=== END ENVIRONMENT ==="
        """
        
        result = subprocess.run(
            setup_script,
            shell=True,
            capture_output=True,
            text=True,
            executable='/bin/bash'
        )
        
        if result.returncode != 0:
            return False, f"Failed to source envset.sh: {result.stderr}"
        
        # 환경변수 파싱 - 더 정확한 파싱
        env_section = False
        for line in result.stdout.split('\n'):
            if line.strip() == "=== ENVIRONMENT VARIABLES ===":
                env_section = True
                continue
            elif line.strip() == "=== END ENVIRONMENT ===":
                env_section = False
                continue
            elif env_section and '=' in line:
                key, value = line.split('=', 1)
                CUSTOM_ENV[key] = value
        
        # ROOT 환경변수들을 명시적으로 설정
        if 'ROOTSYS' not in CUSTOM_ENV:
            # Homebrew ROOT의 정확한 ROOTSYS 경로 설정
            try:
                root_cellar_path = "/opt/homebrew/Cellar/root"
                if os.path.exists(root_cellar_path):
                    versions = [d for d in os.listdir(root_cellar_path) if os.path.isdir(os.path.join(root_cellar_path, d))]
                    if versions:
                        latest_version = sorted(versions)[-1]
                        CUSTOM_ENV['ROOTSYS'] = f"/opt/homebrew/Cellar/root/{latest_version}"
                    else:
                        CUSTOM_ENV['ROOTSYS'] = '/opt/homebrew'
                else:
                    CUSTOM_ENV['ROOTSYS'] = '/opt/homebrew'
            except Exception:
                CUSTOM_ENV['ROOTSYS'] = '/opt/homebrew'
        
        # 라이브러리 경로들 설정
        install_lib_path = os.path.join(os.path.dirname(BASE_DIR), "install", "lib")
        
        # ROOT 라이브러리 경로를 동적으로 찾기
        root_lib_path = "/opt/homebrew/lib/root"  # 기본값
        try:
            # ROOT 버전 찾기
            root_cellar_path = "/opt/homebrew/Cellar/root"
            if os.path.exists(root_cellar_path):
                versions = [d for d in os.listdir(root_cellar_path) if os.path.isdir(os.path.join(root_cellar_path, d))]
                if versions:
                    latest_version = sorted(versions)[-1]  # 가장 최신 버전 사용
                    potential_root_lib = f"/opt/homebrew/Cellar/root/{latest_version}/lib/root"
                    if os.path.exists(potential_root_lib):
                        root_lib_path = potential_root_lib
        except Exception:
            pass  # 기본값 사용
        
        # DYLD_LIBRARY_PATH 설정
        dyld_paths = [install_lib_path, root_lib_path]
        if 'DYLD_LIBRARY_PATH' in CUSTOM_ENV:
            existing_paths = CUSTOM_ENV['DYLD_LIBRARY_PATH'].split(':')
            for path in dyld_paths:
                if path not in existing_paths:
                    existing_paths.append(path)
            CUSTOM_ENV['DYLD_LIBRARY_PATH'] = ':'.join(existing_paths)
        else:
            CUSTOM_ENV['DYLD_LIBRARY_PATH'] = ':'.join(dyld_paths)
            
        # DYLD_FALLBACK_LIBRARY_PATH 설정 (macOS에서 더 안정적)
        fallback_paths = [install_lib_path, root_lib_path, "/opt/homebrew/lib", "/usr/lib"]
        if 'DYLD_FALLBACK_LIBRARY_PATH' in CUSTOM_ENV:
            existing_paths = CUSTOM_ENV['DYLD_FALLBACK_LIBRARY_PATH'].split(':')
            for path in fallback_paths:
                if path not in existing_paths:
                    existing_paths.append(path)
            CUSTOM_ENV['DYLD_FALLBACK_LIBRARY_PATH'] = ':'.join(existing_paths)
        else:
            CUSTOM_ENV['DYLD_FALLBACK_LIBRARY_PATH'] = ':'.join(fallback_paths)
            
        # LD_LIBRARY_PATH 설정
        ld_paths = [root_lib_path, install_lib_path, "/opt/homebrew/lib"]
        if 'LD_LIBRARY_PATH' in CUSTOM_ENV:
            existing_paths = CUSTOM_ENV['LD_LIBRARY_PATH'].split(':')
            for path in ld_paths:
                if path not in existing_paths:
                    existing_paths.append(path)
            CUSTOM_ENV['LD_LIBRARY_PATH'] = ':'.join(existing_paths)
        else:
            CUSTOM_ENV['LD_LIBRARY_PATH'] = ':'.join(ld_paths)
        
        # PATH에 ROOT bin 디렉토리 추가
        root_bin_paths = ["/opt/homebrew/bin"]
        
        # ROOTSYS가 설정되어 있다면 해당 bin 디렉토리도 추가
        if 'ROOTSYS' in CUSTOM_ENV:
            potential_root_bin = os.path.join(CUSTOM_ENV['ROOTSYS'], 'bin')
            if os.path.exists(potential_root_bin):
                root_bin_paths.insert(0, potential_root_bin)
        
        if 'PATH' in CUSTOM_ENV:
            current_paths = CUSTOM_ENV['PATH'].split(':')
            for root_bin_path in root_bin_paths:
                if root_bin_path not in current_paths:
                    current_paths.insert(0, root_bin_path)
            CUSTOM_ENV['PATH'] = ':'.join(current_paths)
        else:
            CUSTOM_ENV['PATH'] = f"{':'.join(root_bin_paths)}:{os.environ.get('PATH', '')}"
        
        ENV_INITIALIZED = True
        return True, "Environment initialized successfully"
        
    except Exception as e:
        return False, f"Error parsing envset.sh: {str(e)}"

def check_library_dependencies():
    """라이브러리 의존성을 체크합니다."""
    try:
        install_lib_path = os.path.join(os.path.dirname(BASE_DIR), "install", "lib")
        libdrc_path = os.path.join(install_lib_path, "libdrcTB.dylib")
        monit_path = os.path.join(BASE_DIR, "monit")
        
        checks = []
        
        # 1. 라이브러리 파일 존재 체크
        if os.path.exists(libdrc_path):
            checks.append("✅ libdrcTB.dylib found")
        else:
            checks.append("❌ libdrcTB.dylib NOT found")
        
        # 2. monit 실행파일 존재 체크
        if os.path.exists(monit_path):
            checks.append("✅ monit executable found")
            
            # 3. otool로 의존성 체크 (macOS specific)
            try:
                result = subprocess.run(
                    ['otool', '-L', monit_path],
                    capture_output=True,
                    text=True
                )
                if result.returncode == 0:
                    lines = result.stdout.split('\n')
                    for line in lines:
                        if 'libdrcTB.dylib' in line:
                            checks.append(f"📋 monit dependency: {line.strip()}")
                            break
                    else:
                        checks.append("⚠️  libdrcTB.dylib dependency not found in monit")
                else:
                    checks.append("⚠️  Could not check monit dependencies")
            except FileNotFoundError:
                checks.append("⚠️  otool not available (dependency check skipped)")
                
        else:
            checks.append("❌ monit executable NOT found")
        
        # 4. ROOT 라이브러리 경로 체크
        # ROOT 라이브러리 경로를 동적으로 찾기
        root_lib_path = "/opt/homebrew/lib/root"  # 기본값
        try:
            root_cellar_path = "/opt/homebrew/Cellar/root"
            if os.path.exists(root_cellar_path):
                versions = [d for d in os.listdir(root_cellar_path) if os.path.isdir(os.path.join(root_cellar_path, d))]
                if versions:
                    latest_version = sorted(versions)[-1]
                    potential_root_lib = f"/opt/homebrew/Cellar/root/{latest_version}/lib/root"
                    if os.path.exists(potential_root_lib):
                        root_lib_path = potential_root_lib
        except Exception:
            pass
            
        if os.path.exists(root_lib_path):
            checks.append(f"✅ ROOT library directory found: {root_lib_path}")
        else:
            checks.append(f"❌ ROOT library directory NOT found: {root_lib_path}")
        
        # 5. 환경변수 경로 체크
        if 'DYLD_LIBRARY_PATH' in CUSTOM_ENV:
            if install_lib_path in CUSTOM_ENV['DYLD_LIBRARY_PATH'] and root_lib_path in CUSTOM_ENV['DYLD_LIBRARY_PATH']:
                checks.append("✅ DYLD_LIBRARY_PATH includes both install/lib and ROOT/lib")
            elif install_lib_path in CUSTOM_ENV['DYLD_LIBRARY_PATH']:
                checks.append("⚠️  DYLD_LIBRARY_PATH includes install/lib but missing ROOT/lib")
            elif root_lib_path in CUSTOM_ENV['DYLD_LIBRARY_PATH']:
                checks.append("⚠️  DYLD_LIBRARY_PATH includes ROOT/lib but missing install/lib")
            else:
                checks.append("❌ DYLD_LIBRARY_PATH missing both install/lib and ROOT/lib")
        else:
            checks.append("❌ DYLD_LIBRARY_PATH not set")
            
        if 'DYLD_FALLBACK_LIBRARY_PATH' in CUSTOM_ENV:
            if install_lib_path in CUSTOM_ENV['DYLD_FALLBACK_LIBRARY_PATH'] and root_lib_path in CUSTOM_ENV['DYLD_FALLBACK_LIBRARY_PATH']:
                checks.append("✅ DYLD_FALLBACK_LIBRARY_PATH includes both install/lib and ROOT/lib")
            else:
                checks.append("⚠️  DYLD_FALLBACK_LIBRARY_PATH partially configured")
        else:
            checks.append("❌ DYLD_FALLBACK_LIBRARY_PATH not set")
            
        # 6. ROOTSYS 체크
        if 'ROOTSYS' in CUSTOM_ENV:
            checks.append(f"✅ ROOTSYS set to: {CUSTOM_ENV['ROOTSYS']}")
        else:
            checks.append("❌ ROOTSYS not set")
        
        return '\n'.join(checks)
        
    except Exception as e:
        return f"Error checking dependencies: {str(e)}"

def run_command_with_env(command, cwd=None):
    """환경변수를 포함하여 명령어를 실행합니다."""
    if cwd is None:
        cwd = BASE_DIR
        
    # 환경변수가 초기화되지 않았다면 초기화 시도
    if not ENV_INITIALIZED:
        success, message = parse_envset_sh()
        if not success:
            return subprocess.CompletedProcess(
                args=command,
                returncode=1,
                stdout="",
                stderr=f"Environment not initialized: {message}"
            )
    
    try:
        result = subprocess.run(
            command,
            shell=True,
            capture_output=True,
            text=True,
            cwd=cwd,
            env=CUSTOM_ENV,
            executable='/bin/bash'
        )
        return result
    except Exception as e:
        return subprocess.CompletedProcess(
            args=command,
            returncode=1,
            stdout="",
            stderr=str(e)
        )

@app.route('/')
def serve_index():
    return send_from_directory(INDEX_DIR, 'index.html')

def send_file_with_range_support(file_path):
    """Range 요청을 지원하는 파일 전송 함수"""
    def generate():
        with open(file_path, 'rb') as f:
            data = f.read(1024)
            while data:
                yield data
                data = f.read(1024)
    
    file_size = os.path.getsize(file_path)
    range_header = request.headers.get('Range', None)
    
    if not range_header:
        return Response(generate(), 
                       mimetype="application/octet-stream",
                       headers={'Content-Length': str(file_size),
                               'Accept-Ranges': 'bytes'})
    
    # Range 요청 처리 - 다중 범위 처리 개선
    try:
        byte_start = 0
        byte_end = file_size - 1
        
        if range_header:
            range_value = range_header.replace('bytes=', '')
            # 다중 범위인 경우 첫 번째만 사용
            if ',' in range_value:
                range_value = range_value.split(',')[0]
            
            if '-' in range_value:
                parts = range_value.split('-')
                if parts[0]:
                    byte_start = int(parts[0])
                if parts[1]:
                    byte_end = int(parts[1])
        
        # 범위 검증
        if byte_start >= file_size or byte_end >= file_size or byte_start > byte_end:
            return Response("416 Range Not Satisfiable", status=416)
            
        content_length = byte_end - byte_start + 1
        
        def generate_range():
            with open(file_path, 'rb') as f:
                f.seek(byte_start)
                remaining = content_length
                while remaining > 0:
                    chunk_size = min(1024, remaining)
                    data = f.read(chunk_size)
                    if not data:
                        break
                    yield data
                    remaining -= len(data)
        
        return Response(generate_range(),
                       206,  # Partial Content
                       mimetype="application/octet-stream",
                       headers={
                           'Content-Range': f'bytes {byte_start}-{byte_end}/{file_size}',
                           'Accept-Ranges': 'bytes',
                           'Content-Length': str(content_length)
                       })
    except Exception as e:
        # Fallback to full file
        return Response(generate(), 
                       mimetype="application/octet-stream",
                       headers={'Content-Length': str(file_size),
                               'Accept-Ranges': 'bytes'})

@app.route('/output/<path:filename>')
def serve_output_file(filename):
    try:
        file_path = os.path.join('output', filename)
        if not os.path.exists(file_path):
            return f"File {filename} not found", 404
            
        # Add cache control headers for ROOT files to ensure fresh content
        response = send_from_directory('output', filename)
        if filename.endswith('.root'):
            response.headers['Cache-Control'] = 'no-cache, no-store, must-revalidate'
            response.headers['Pragma'] = 'no-cache'
            response.headers['Expires'] = '0'
            
        return response
    except Exception as e:
        return f"Error serving file {filename}: {str(e)}", 500

@app.route('/files')
def list_root_files():
    try:
        files_info = []
        for f in os.listdir(ROOT_DIR):
            if f.endswith(".root"):
                file_path = os.path.join(ROOT_DIR, f)
                if os.path.exists(file_path):
                    # Get file creation/modification time
                    stat = os.stat(file_path)
                    # Use modification time (mtime) as it's more reliable than creation time
                    modification_time = stat.st_mtime
                    files_info.append({
                        'name': f,
                        'mtime': modification_time,
                        'size': stat.st_size
                    })
        
        # Sort by modification time (most recent first)
        files_info.sort(key=lambda x: x['mtime'], reverse=True)
        
        return jsonify(files_info)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

# 정적 파일들을 서빙하는 라우트 추가
@app.route('/<path:filename>')
def serve_static_files(filename):
    return send_from_directory(INDEX_DIR, filename)

# 명령어 실행 API 추가
@app.route('/execute', methods=['POST'])
def execute_command():
    try:
        data = request.get_json()
        command = data.get('command', '')
        
        if not command:
            return jsonify({'error': 'No command provided'}), 400
        
        # 보안을 위해 특정 명령어만 금지 (rm, cd는 위험할 수 있음)
        command_parts = command.split()
        forbidden_commands = ['rm', 'cd']
        
        if command_parts and command_parts[0] in forbidden_commands:
            return jsonify({'error': f'Command not allowed for security reasons. Forbidden: {", ".join(forbidden_commands)}'}), 403
        
        # 명령어 실행
        result = run_command_with_env(command)
        
        return jsonify({
            'command': command,
            'stdout': result.stdout,
            'stderr': result.stderr,
            'returncode': result.returncode
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

# 실시간 명령어 실행 API (스트리밍)
@app.route('/execute_stream', methods=['GET'])
def execute_stream():
    try:
        command = request.args.get('command', '')
        
        if not command:
            return "data: " + json.dumps({'type': 'error', 'content': 'No command provided'}) + "\n\n", 400
        
        # 보안을 위해 특정 명령어만 금지 (rm, cd는 위험할 수 있음)
        command_parts = command.split()
        forbidden_commands = ['rm', 'cd']
        
        if command_parts and command_parts[0] in forbidden_commands:
            return "data: " + json.dumps({'type': 'error', 'content': f'Command not allowed for security reasons. Forbidden: {", ".join(forbidden_commands)}'}) + "\n\n", 403
        
        def generate():
            # 환경변수가 초기화되지 않았다면 초기화 시도
            if not ENV_INITIALIZED:
                success, message = parse_envset_sh()
                if not success:
                    yield f"data: {json.dumps({'type': 'error', 'content': f'Environment not initialized: {message}'})}\n\n"
                    return
            
            global current_process
            
            try:
                # Popen을 사용해서 실시간 출력 스트리밍
                import io
                
                with process_lock:
                    current_process = subprocess.Popen(
                        command,
                        shell=True,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT,  # stderr를 stdout과 합침
                        text=True,
                        bufsize=1,  # 라인 버퍼링
                        universal_newlines=True,
                        cwd=BASE_DIR,
                        env=CUSTOM_ENV
                    )
                
                # 명령어 시작 알림
                yield f"data: {json.dumps({'type': 'command', 'content': f'$ {command}'})}\n\n"
                
                # 실시간으로 출력 스트리밍 - 개선된 버전
                buffer = ""
                waiting_for_content_after_escape = False
                
                while True:
                    # 프로세스가 종료되었는지 확인
                    if current_process.poll() is not None:
                        break
                    
                    try:
                        # 한 글자씩 읽어서 ANSI 시퀀스 보존
                        char = current_process.stdout.read(1)
                        if not char:
                            continue
                            
                        buffer += char
                        
                        # \x1b[F 패턴을 감지했으면 다음 내용까지 기다림
                        if buffer.endswith('\x1b[F'):
                            waiting_for_content_after_escape = True
                            continue
                        
                        # \x1b[F 후에 내용이 완성되면 함께 전송
                        if waiting_for_content_after_escape and (char == '\n' or char == '\r' or buffer.endswith('\x1b[0m')):
                            yield f"data: {json.dumps({'type': 'output', 'content': buffer.rstrip()})}\n\n"
                            buffer = ""
                            waiting_for_content_after_escape = False
                        # \r로 끝나는 라인도 덮어쓰기 대상으로 처리
                        elif char == '\r' and not waiting_for_content_after_escape:
                            yield f"data: {json.dumps({'type': 'output', 'content': buffer})}\n\n"
                            buffer = ""
                        # 일반 줄바꿈시 전송 (이스케이프 대기 중이 아닐 때만)
                        elif char == '\n' and not waiting_for_content_after_escape:
                            if buffer and buffer.strip():  # 빈 줄은 전송하지 않음
                                # f-string에서 백슬래시 사용 불가로 인한 분리
                                content = buffer.rstrip('\n')
                                yield f"data: {json.dumps({'type': 'output', 'content': content})}\n\n"
                            buffer = ""
                        # 너무 긴 버퍼는 강제로 전송 (무한 대기 방지)
                        elif len(buffer) > 1000:
                            yield f"data: {json.dumps({'type': 'output', 'content': buffer})}\n\n"
                            buffer = ""
                            waiting_for_content_after_escape = False
                            
                    except Exception as e:
                        break
                
                # 남은 버퍼 전송
                if buffer:
                    yield f"data: {json.dumps({'type': 'output', 'content': buffer})}\n\n"
                
                # 프로세스 완료 대기
                current_process.wait()
                
                # 완료 알림
                yield f"data: {json.dumps({'type': 'complete', 'content': f'Command exited with code: {current_process.returncode}'})}\n\n"
                
                # 프로세스 완료 후 정리
                with process_lock:
                    current_process = None
                
            except Exception as e:
                yield f"data: {json.dumps({'type': 'error', 'content': f'Error: {str(e)}'})}\n\n"
                # 에러 발생시에도 프로세스 정리
                with process_lock:
                    if current_process:
                        try:
                            current_process.terminate()
                        except:
                            pass
                        current_process = None
        
        return Response(
            generate(),
            mimetype='text/event-stream',
            headers={
                'Cache-Control': 'no-cache',
                'Connection': 'keep-alive',
                'Access-Control-Allow-Origin': '*',
                'Access-Control-Allow-Headers': 'Content-Type'
            }
        )
        
    except Exception as e:
        return "data: " + json.dumps({'type': 'error', 'content': str(e)}) + "\n\n", 500

# 모니터링 프로그램 실행 API
@app.route('/run_monit', methods=['POST'])
def run_monit():
    try:
        # 환경변수가 초기화되지 않았다면 먼저 초기화
        if not ENV_INITIALIZED:
            success, message = parse_envset_sh()
            if not success:
                return jsonify({
                    'command': './monit',
                    'stdout': '',
                    'stderr': f'Environment not initialized: {message}',
                    'returncode': 1
                })
        
        # 라이브러리 의존성 체크
        lib_status = check_library_dependencies()
        
        # monit 프로그램 실행
        result = run_command_with_env('./monit')
        
        # 에러가 발생한 경우 추가 정보 제공
        if result.returncode != 0 and 'Library not loaded' in result.stderr:
            additional_info = f"""

🔍 Library Loading Error Detected!

Library Status Check:
{lib_status}

💡 Troubleshooting Tips:
1. Try building the project again: cd .. && ./buildNinstall.sh
2. Check if ROOT is properly installed: which root
3. Verify library permissions: ls -la install/lib/libdrcTB.dylib

For more help with library loading issues, try running:
otool -L ./monit  (to check dependencies)
"""
            return jsonify({
                'command': './monit',
                'stdout': result.stdout,
                'stderr': result.stderr + additional_info,
                'returncode': result.returncode
            })
        
        return jsonify({
            'command': './monit',
            'stdout': result.stdout,
            'stderr': result.stderr,
            'returncode': result.returncode
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

# 환경설정 초기화 API
@app.route('/init_env', methods=['POST'])
def init_env():
    try:
        global ENV_INITIALIZED
        
        # 환경변수 파싱 및 초기화
        success, message = parse_envset_sh()
        
        if success:
            # 환경변수가 제대로 설정되었는지 확인
            env_info = []
            important_vars = ['ROOTSYS', 'DYLD_LIBRARY_PATH', 'DYLD_FALLBACK_LIBRARY_PATH', 'LD_LIBRARY_PATH', 'PATH', 'INSTALL_DIR_PATH', 'YAML_CPP_DIR']
            
            for var in important_vars:
                if var in CUSTOM_ENV:
                    env_info.append(f"{var}={CUSTOM_ENV[var]}")
                else:
                    env_info.append(f"{var}=<not set>")
            
            # 라이브러리 의존성 체크
            lib_check = check_library_dependencies()
            
            # rpath 문제 체크 및 자동 수정
            rpath_fix_result = check_and_fix_rpath()
            
            stdout_msg = f"""Environment initialization successful!

Key environment variables:
{chr(10).join(env_info)}

Library Status:
{lib_check}

RPath Fix:
{rpath_fix_result}

libdrcTB.dylib location: {os.path.join(os.path.dirname(BASE_DIR), 'install', 'lib', 'libdrcTB.dylib')}
"""
            
            return jsonify({
                'command': 'Environment Initialization',
                'stdout': stdout_msg,
                'stderr': '',
                'returncode': 0
            })
        else:
            return jsonify({
                'command': 'Environment Initialization',
                'stdout': '',
                'stderr': f'Failed to initialize environment: {message}',
                'returncode': 1
            })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

def check_and_fix_rpath():
    """monit 실행파일의 rpath를 체크하고 필요시 수정합니다."""
    try:
        monit_path = os.path.join(BASE_DIR, "monit")
        
        if not os.path.exists(monit_path):
            return "❌ monit executable not found"
        
        # 현재 rpath 확인
        result = subprocess.run(
            ['otool', '-l', monit_path],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            return "⚠️  Could not read rpath information"
        
        # 현재 ROOT 버전 찾기
        current_root_lib = None
        try:
            root_cellar_path = "/opt/homebrew/Cellar/root"
            if os.path.exists(root_cellar_path):
                versions = [d for d in os.listdir(root_cellar_path) if os.path.isdir(os.path.join(root_cellar_path, d))]
                if versions:
                    latest_version = sorted(versions)[-1]
                    current_root_lib = f"/opt/homebrew/Cellar/root/{latest_version}/lib/root"
        except Exception:
            pass
        
        if not current_root_lib:
            return "❌ Could not determine current ROOT library path"
        
        # rpath에서 ROOT 경로 찾기
        lines = result.stdout.split('\n')
        old_rpath = None
        for i, line in enumerate(lines):
            if 'LC_RPATH' in line and i + 2 < len(lines):
                path_line = lines[i + 2].strip()
                if 'path ' in path_line and 'root' in path_line:
                    old_rpath = path_line.split('path ')[1].split(' (offset')[0]
                    break
        
        # libdrcTB.dylib 위치 확인
        install_lib_path = os.path.join(os.path.dirname(BASE_DIR), "install", "lib")
        libdrc_source = os.path.join(install_lib_path, "libdrcTB.dylib")
        libdrc_target = os.path.join(current_root_lib, "libdrcTB.dylib")
        
        if not os.path.exists(libdrc_source):
            return "❌ libdrcTB.dylib not found in install/lib"
        
        # ROOT 라이브러리 디렉토리에 libdrcTB.dylib가 있는지 확인
        if os.path.exists(libdrc_target):
            if os.path.islink(libdrc_target):
                # 심볼릭 링크인 경우 올바른 경로를 가리키는지 확인
                link_target = os.readlink(libdrc_target)
                if os.path.samefile(libdrc_source, libdrc_target):
                    return f"✅ libdrcTB.dylib symbolic link already exists and correct\n   Link: {libdrc_target} -> {link_target}"
                else:
                    # 잘못된 링크 제거 후 재생성
                    try:
                        os.unlink(libdrc_target)
                    except Exception as e:
                        return f"❌ Failed to remove incorrect symbolic link: {str(e)}"
            else:
                return f"⚠️  libdrcTB.dylib already exists in ROOT lib (not a link): {libdrc_target}"
        
        # rpath 업데이트 시도 (선택사항)
        rpath_result = ""
        if old_rpath and old_rpath != current_root_lib:
            try:
                # 이전 rpath 제거 시도
                subprocess.run(
                    ['install_name_tool', '-delete_rpath', old_rpath, monit_path],
                    capture_output=True,
                    text=True,
                    check=False
                )
                
                # 새 rpath 추가 시도
                add_result = subprocess.run(
                    ['install_name_tool', '-add_rpath', current_root_lib, monit_path],
                    capture_output=True,
                    text=True
                )
                
                if add_result.returncode == 0:
                    rpath_result = f"✅ RPath updated: {old_rpath} -> {current_root_lib}\n"
                else:
                    rpath_result = f"⚠️  RPath update failed (will use symlink): {add_result.stderr}\n"
                    
            except Exception as e:
                rpath_result = f"⚠️  RPath update error (will use symlink): {str(e)}\n"
        elif old_rpath == current_root_lib:
            rpath_result = f"✅ RPath already correct: {current_root_lib}\n"
        else:
            rpath_result = "⚠️  No ROOT rpath found, will create symlink\n"
        
        # 심볼릭 링크 생성
        try:
            # sudo 없이 시도 (권한이 있는 경우)
            try:
                os.symlink(libdrc_source, libdrc_target)
                symlink_result = f"✅ Created symbolic link: {libdrc_target} -> {libdrc_source}"
            except PermissionError:
                # sudo 권한이 필요한 경우
                sudo_result = subprocess.run(
                    ['sudo', 'ln', '-sf', libdrc_source, libdrc_target],
                    capture_output=True,
                    text=True,
                    input='\n'  # 빈 입력으로 패스워드 프롬프트 건너뛰기 시도
                )
                
                if sudo_result.returncode == 0:
                    symlink_result = f"✅ Created symbolic link (sudo): {libdrc_target} -> {libdrc_source}"
                else:
                    symlink_result = f"❌ Failed to create symbolic link: {sudo_result.stderr}"
                    
        except Exception as e:
            symlink_result = f"❌ Error creating symbolic link: {str(e)}"
        
        return rpath_result + symlink_result
        
    except Exception as e:
        return f"❌ Error in rpath check/fix: {str(e)}"

# RPath 수정 전용 API
@app.route('/fix_rpath', methods=['POST'])
def fix_rpath():
    try:
        result = check_and_fix_rpath()
        
        return jsonify({
            'command': 'RPath Fix',
            'stdout': result,
            'stderr': '',
            'returncode': 0 if '✅' in result else 1
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/SetENV', methods=['POST'])
def SetENV():
    try:
        # monit 프로그램 실행
        result = run_command_with_env('cd .. ; source envset.sh ; cd monit')
        
        return jsonify({
            'command': 'cd .. ; source envset.sh ; cd monit',
            'stdout': result.stdout,
            'stderr': result.stderr,
            'returncode': result.returncode
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

# 프로세스 종료 API
@app.route('/kill_process', methods=['POST'])
def kill_process():
    try:
        global current_process
        
        with process_lock:
            if current_process is not None and current_process.poll() is None:
                # 프로세스가 실행 중인 경우 종료
                current_process.terminate()
                # 강제 종료가 필요한 경우
                try:
                    current_process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    current_process.kill()
                    current_process.wait()
                
                return jsonify({
                    'success': True,
                    'message': 'Process terminated successfully'
                })
            else:
                return jsonify({
                    'success': False,
                    'message': 'No running process to kill'
                })
                
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/find_monit_processes', methods=['GET'])
def find_monit_processes():
    try:
        import subprocess
        # Find all processes containing './monit'
        result = subprocess.run(['ps', 'aux'], capture_output=True, text=True)
        if result.returncode != 0:
            return jsonify({'success': False, 'message': 'Failed to get process list'})
        
        processes = []
        lines = result.stdout.strip().split('\n')
        
        for line in lines[1:]:  # Skip header
            if './monit' in line:
                # Parse process info
                parts = line.split(None, 10)  # Split into max 11 parts
                if len(parts) >= 11:
                    pid = parts[1]
                    command = parts[10]
                    processes.append({
                        'pid': pid,
                        'command': command,
                        'full_line': line
                    })
        
        return jsonify({
            'success': True, 
            'processes': processes,
            'count': len(processes)
        })
    except Exception as e:
        return jsonify({'success': False, 'message': str(e)})

@app.route('/kill_all_monit', methods=['POST'])
def kill_all_monit():
    try:
        import subprocess
        import signal
        
        # Get list of ./monit processes first
        find_result = find_monit_processes()
        if not find_result.get_json()['success']:
            return find_result
        
        processes = find_result.get_json()['processes']
        
        if not processes:
            return jsonify({'success': True, 'message': 'No ./monit processes found', 'killed_count': 0})
        
        killed_pids = []
        failed_pids = []
        
        for proc in processes:
            try:
                pid = int(proc['pid'])
                os.kill(pid, signal.SIGTERM)
                killed_pids.append(pid)
                
                # Give process time to terminate gracefully
                import time
                time.sleep(0.5)
                
                # Check if still running and force kill if necessary
                try:
                    os.kill(pid, 0)  # Check if process exists
                    os.kill(pid, signal.SIGKILL)  # Force kill
                except OSError:
                    pass  # Process already terminated
                
            except (ValueError, OSError) as e:
                failed_pids.append(proc['pid'])
        
        message = f"Killed {len(killed_pids)} ./monit process(es)"
        if failed_pids:
            message += f", failed to kill: {failed_pids}"
        
        return jsonify({
            'success': True, 
            'message': message,
            'killed_count': len(killed_pids),
            'failed_count': len(failed_pids),
            'killed_pids': killed_pids,
            'failed_pids': failed_pids
        })
        
    except Exception as e:
        return jsonify({'success': False, 'message': str(e)})

# 실행 중인 프로세스 상태 확인 API
@app.route('/process_status', methods=['GET'])
def process_status():
    try:
        global current_process
        
        with process_lock:
            if current_process is not None and current_process.poll() is None:
                return jsonify({
                    'running': True,
                    'pid': current_process.pid
                })
            else:
                return jsonify({
                    'running': False
                })
                
    except Exception as e:
        return jsonify({
            'running': False,
            'error': str(e)
        })

# 보안 코드 검증 API
@app.route('/verify_code', methods=['POST'])
def verify_code():
    try:
        data = request.get_json()
        code = data.get('code', '')
        
        # 간단한 보안 코드 (실제 환경에서는 더 안전한 방법 사용)
        SECURITY_CODE = "TB2025"
        
        if code == SECURITY_CODE:
            return jsonify({
                'success': True,
                'message': 'Access granted'
            })
        else:
            return jsonify({
                'success': False,
                'message': 'Invalid access code'
            })
            
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/api/system-info')
def get_system_info():
    try:
        import psutil
        memory = psutil.virtual_memory()
        cpu_percent = psutil.cpu_percent(interval=1)
        
        return jsonify({
            'memory': {
                'used': round(memory.used / 1024 / 1024 / 1024, 2),  # GB
                'total': round(memory.total / 1024 / 1024 / 1024, 2),  # GB
                'percent': memory.percent
            },
            'cpu': {
                'percent': cpu_percent
            }
        })
    except ImportError:
        # Fallback if psutil is not available
        import os
        import resource
        
        # Get memory usage (rough estimate)
        memory_usage = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
        # On macOS, ru_maxrss is in bytes; on Linux, it's in KB
        if os.name == 'posix' and os.uname().sysname == 'Darwin':  # macOS
            memory_mb = memory_usage / 1024 / 1024
        else:  # Linux
            memory_mb = memory_usage / 1024
            
        return jsonify({
            'memory': {
                'used': round(memory_mb / 1024, 2),  # GB
                'total': 'N/A',
                'percent': 'N/A'
            },
            'cpu': {
                'percent': 'N/A'
            }
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/DRC_DQM_manual.pdf')
def serve_dqm_manual():
    try:
        pdf_path = os.path.join(os.path.dirname(__file__), 'DRC_DQM_manual.pdf')
        if os.path.exists(pdf_path):
            return send_file(pdf_path, mimetype='application/pdf', as_attachment=False)
        else:
            return "DQM Manual PDF not found. Please ensure DRC_DQM_manual.pdf is in the monit directory.", 404
    except Exception as e:
        return f"Error serving DQM manual: {str(e)}", 500

if __name__ == '__main__':
    print(f"✅ Server running on http://localhost:8000")
    print(f"📁 Serving ROOT files from: {ROOT_DIR}")
    print(f"🌐 Serving web files from: {INDEX_DIR}")
    print(f"💻 Command execution enabled (forbidden: rm, cd)")
    print(f"🔧 Environment initialization available")
    app.run(host='0.0.0.0', port=8000, debug=True)