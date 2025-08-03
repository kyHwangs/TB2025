cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(dual-readout_TB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(yaml-cpp_DIR /Users/Shared/cvmfs/sft.cern.ch/lcg/releases/yamlcpp/0.6.3-d05b2/arm64-mac15-clang170-opt/lib/cmake/yaml-cpp)

include(GNUInstallDirs)

find_package(ROOT REQUIRED)
# find_package(yaml-cpp REQUIRED)


include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  # /Users/Shared/cvmfs/sft.cern.ch/lcg/releases/yamlcpp/0.6.3-d05b2/arm64-mac15-clang170-opt/include
)

file(GLOB sources
  ${PROJECT_SOURCE_DIR}/src/*.cc
)

file(GLOB headers
  ${PROJECT_SOURCE_DIR}/include/*.h
)

include(${ROOT_USE_FILE})
ROOT_GENERATE_DICTIONARY(G__drcTB ${headers} LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/include/LinkDef.hh MODULE drcTB)

add_library(drcTB SHARED ${sources} G__drcTB.cxx)

target_include_directories(drcTB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${ROOT_INCLUDE_DIRS} ${YAML_CPP_INCLUDE_DIR})

target_link_libraries(
  drcTB
  ${ROOT_LIBRARIES}
  # ${YAML_CPP_LIBRARIES}
)

target_include_directories(drcTB PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

set_target_properties(drcTB PROPERTIES PUBLIC_HEADER "${headers}")

install(TARGETS drcTB EXPORT drcTBTargets
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT dev
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/libdrcTB_rdict.pcm"
  "${CMAKE_CURRENT_BINARY_DIR}/libdrcTB.rootmap"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
