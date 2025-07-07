# dual-readout_TB

Package for Offline TB 2023 Analysis

## Dependencies

All the dependencies can be sourced from CVMFS automatically by running compile (or environment set) script.

* ROOT
* python3
* YAML

## How to compile the package (required only once)
- Scripts for both MacOS and CentOS are prepared.
- Use proper script by replacing `<your OS>` with `macos` or `centos`
```sh
source buildNinstall_<your OS>.sh

e.g.) source buildNinstall_centos.sh
```
**Caution) Current CMakeLists.txt is for CentOS. If one wants to compile the package in MacOS, please remove current CMakeLists.txt and copy CMakeLists_macos.txt to CMakeLists.txt before running buildNinstall_macos.sh**

## How to setup environment (required every time after restarting or re-accessing the server)
```sh
source envset_<your OS>.sh

e.g) source envset_centos.sh
```

## How to compile the analysis scripts
**Caution) Works only after compiling the package or setting up the environment**
```sh
cd analysis
./compile_<your OS> <analysis code in cpp>

e.g.) ./compile_centos.sh TBanalysis_ex
```