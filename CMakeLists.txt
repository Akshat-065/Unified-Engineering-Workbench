cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
project(UnifiedEngineeringWorkbench VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
option(WORKBENCH_USE_SYSTEM_DEPENDENCIES
  "Prefer exact-version packages found through CMAKE_PREFIX_PATH" ON)
option(WORKBENCH_FETCHCONTENT_QUIET "Suppress FetchContent download output" OFF)

include(FetchContent)
set(FETCHCONTENT_QUIET "${WORKBENCH_FETCHCONTENT_QUIET}" CACHE BOOL
  "Control FetchContent download output" FORCE)

# Release archives avoid Git history negotiation and are verified by a fixed
# SHA-256 digest. A matching package from CMAKE_PREFIX_PATH takes precedence.
FetchContent_Declare(httplib
  URL https://codeload.github.com/yhirose/cpp-httplib/tar.gz/refs/tags/v0.18.3
  URL_HASH SHA256=A0567BCD6C3FE5CEF1B329B96245119047F876B49E06CC129A36A7A8DFFE173E)
FetchContent_Declare(nlohmann_json
  URL https://codeload.github.com/nlohmann/json/tar.gz/refs/tags/v3.12.0
  URL_HASH SHA256=4B92EB0C06D10683F7447CE9406CB97CD4B453BE18D7279320F7B2F025C10187)
FetchContent_Declare(googletest
  URL https://codeload.github.com/google/googletest/tar.gz/refs/tags/v1.15.2
  URL_HASH SHA256=7B42B4D6ED48810C5362C265A17FAEBE90DC2373C885E5216439D37927F02926)

if(FETCHCONTENT_BASE_DIR)
  set(_workbench_fetchcontent_base "${FETCHCONTENT_BASE_DIR}")
else()
  set(_workbench_fetchcontent_base "${CMAKE_BINARY_DIR}/_deps")
endif()

if(WORKBENCH_USE_SYSTEM_DEPENDENCIES)
  find_package(httplib 0.18.3 EXACT CONFIG QUIET)
  find_package(nlohmann_json 3.12.0 EXACT CONFIG QUIET)
endif()

if(TARGET httplib::httplib)
  message(STATUS "Dependency cpp-httplib: using installed version ${HTTPLIB_VERSION}")
else()
  # Reuse a complete matching source tree in this build's FetchContent cache.
  # This covers the existing build/vs18-x64/_deps checkout without trusting
  # incomplete or differently-versioned directories.
  if(NOT FETCHCONTENT_SOURCE_DIR_HTTPLIB)
    set(_workbench_httplib_source "${_workbench_fetchcontent_base}/httplib-src")
    if(EXISTS "${_workbench_httplib_source}/httplib.h")
      file(STRINGS "${_workbench_httplib_source}/httplib.h" _workbench_httplib_version_line
        REGEX "^#define CPPHTTPLIB_VERSION ")
      string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" _workbench_httplib_version
        "${_workbench_httplib_version_line}")
      if(_workbench_httplib_version STREQUAL "0.18.3")
        set(FETCHCONTENT_SOURCE_DIR_HTTPLIB "${_workbench_httplib_source}")
      else()
        message(STATUS "Dependency cpp-httplib: cached source version '${_workbench_httplib_version}' is not v0.18.3")
      endif()
    endif()
  endif()
  if(FETCHCONTENT_SOURCE_DIR_HTTPLIB)
    message(STATUS "Dependency cpp-httplib: using pre-populated source ${FETCHCONTENT_SOURCE_DIR_HTTPLIB}")
  else()
    message(STATUS "Dependency cpp-httplib v0.18.3: fetching SHA-256-verified release archive")
  endif()
  set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(httplib)
endif()

if(TARGET nlohmann_json::nlohmann_json)
  message(STATUS "Dependency nlohmann/json: using installed version ${nlohmann_json_VERSION}")
else()
  if(NOT FETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON)
    # The legacy nlohmann_json-src directory may be left incomplete by an
    # interrupted Git clone. Only reuse a separate source tree after checking
    # its actual version; never treat the partial directory as populated.
    set(_workbench_nlohmann_source "${_workbench_fetchcontent_base}/nlohmann_json-3.12.0-src")
    set(_workbench_nlohmann_version_file
      "${_workbench_nlohmann_source}/include/nlohmann/detail/abi_macros.hpp")
    if(EXISTS "${_workbench_nlohmann_version_file}")
      file(STRINGS "${_workbench_nlohmann_version_file}" _workbench_nlohmann_major_line
        REGEX "^#define NLOHMANN_JSON_VERSION_MAJOR ")
      file(STRINGS "${_workbench_nlohmann_version_file}" _workbench_nlohmann_minor_line
        REGEX "^#define NLOHMANN_JSON_VERSION_MINOR ")
      file(STRINGS "${_workbench_nlohmann_version_file}" _workbench_nlohmann_patch_line
        REGEX "^#define NLOHMANN_JSON_VERSION_PATCH ")
      string(REGEX MATCH "[0-9]+" _workbench_nlohmann_major "${_workbench_nlohmann_major_line}")
      string(REGEX MATCH "[0-9]+" _workbench_nlohmann_minor "${_workbench_nlohmann_minor_line}")
      string(REGEX MATCH "[0-9]+" _workbench_nlohmann_patch "${_workbench_nlohmann_patch_line}")
      if("${_workbench_nlohmann_major}.${_workbench_nlohmann_minor}.${_workbench_nlohmann_patch}" STREQUAL "3.12.0")
        set(FETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON "${_workbench_nlohmann_source}")
      endif()
    endif()
  endif()
  if(FETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON)
    message(STATUS "Dependency nlohmann/json: using pre-populated source ${FETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON}")
  else()
    message(STATUS "Dependency nlohmann/json v3.12.0: fetching SHA-256-verified release archive")
  endif()
  FetchContent_MakeAvailable(nlohmann_json)
endif()

if(BUILD_TESTING)
  find_package(GTest 1.15.2 EXACT CONFIG QUIET)
  if(TARGET GTest::gtest_main)
    message(STATUS "Dependency GoogleTest: using installed version ${GTest_VERSION}")
  else()
    if(NOT FETCHCONTENT_SOURCE_DIR_GOOGLETEST)
      set(_workbench_googletest_source "${_workbench_fetchcontent_base}/googletest-src")
      set(_workbench_googletest_version_file "${_workbench_googletest_source}/CMakeLists.txt")
      if(EXISTS "${_workbench_googletest_version_file}")
        file(STRINGS "${_workbench_googletest_version_file}" _workbench_googletest_version_line
          REGEX "^set\\(GOOGLETEST_VERSION ")
        string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" _workbench_googletest_version
          "${_workbench_googletest_version_line}")
        if(_workbench_googletest_version STREQUAL "1.15.2")
          set(FETCHCONTENT_SOURCE_DIR_GOOGLETEST "${_workbench_googletest_source}")
        endif()
      endif()
    endif()
    if(FETCHCONTENT_SOURCE_DIR_GOOGLETEST)
      message(STATUS "Dependency GoogleTest: using pre-populated source ${FETCHCONTENT_SOURCE_DIR_GOOGLETEST}")
    else()
      message(STATUS "Dependency GoogleTest v1.15.2: fetching SHA-256-verified release archive")
    endif()
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()
endif()

add_library(workbench_core src/core/status.cpp src/thermal/solver.cpp src/thermal/results.cpp
  src/geometry/ascii_stl.cpp src/geometry/temperature_csv.cpp src/performance/benchmark.cpp
  src/hardware/mock_adapters.cpp src/ml/backpropagation.cpp)
target_include_directories(workbench_core PUBLIC include)
target_compile_features(workbench_core PUBLIC cxx_std_17)

add_executable(workbench_server src/api/server.cpp)
target_link_libraries(workbench_server PRIVATE workbench_core httplib::httplib nlohmann_json::nlohmann_json)
if(WIN32)
  target_link_libraries(workbench_server PRIVATE ws2_32)
  target_link_libraries(workbench_core PRIVATE advapi32)
endif()

add_executable(workbench_thermal src/thermal/main.cpp)
target_link_libraries(workbench_thermal PRIVATE workbench_core)

add_executable(workbench_performance src/performance/main.cpp)
target_link_libraries(workbench_performance PRIVATE workbench_core)

add_executable(workbench_ml src/ml/main.cpp)
target_link_libraries(workbench_ml PRIVATE workbench_core)

if(BUILD_TESTING)
  add_executable(workbench_core_tests tests/core/status_test.cpp)
  target_link_libraries(workbench_core_tests PRIVATE workbench_core GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(workbench_core_tests)

  add_executable(workbench_thermal_tests tests/thermal/solver_test.cpp)
  target_link_libraries(workbench_thermal_tests PRIVATE workbench_core GTest::gtest_main)
  gtest_discover_tests(workbench_thermal_tests)

  add_executable(workbench_geometry_tests tests/geometry/geometry_test.cpp)
  target_link_libraries(workbench_geometry_tests PRIVATE workbench_core GTest::gtest_main)
  gtest_discover_tests(workbench_geometry_tests)

  add_executable(workbench_performance_tests tests/performance/benchmark_test.cpp)
  target_link_libraries(workbench_performance_tests PRIVATE workbench_core GTest::gtest_main)
  gtest_discover_tests(workbench_performance_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

  add_executable(workbench_hardware_tests tests/hardware/mock_adapters_test.cpp)
  target_link_libraries(workbench_hardware_tests PRIVATE workbench_core GTest::gtest_main)
  gtest_discover_tests(workbench_hardware_tests)

  add_executable(workbench_ml_tests tests/ml/backpropagation_test.cpp)
  target_link_libraries(workbench_ml_tests PRIVATE workbench_core GTest::gtest_main)
  gtest_discover_tests(workbench_ml_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

endif()
