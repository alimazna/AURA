# AURA — top-level build.
#
# AURA is header-only C++17/20, but the audit (GAP_AUDIT_2026-10-02) identified the
# absence of any authoritative build system as a P0 gap. This file provides one:
# it compiles a real static library from the foundation translation units, builds
# every behavioural test suite, and registers them with CTest so the full suite
# runs with a single `ctest`.
#
# The same source tree is intended to build on Windows with MSVC or clang-cl
# (the source has no GNU-only constructs); building on Windows is an environment
# task that is not yet proven here (see project-control/BLOCKED.md).

cmake_minimum_required(VERSION 3.16)
project(AURA LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Foundation translation units (the only non-header .cpp source in src/).
add_library(aura_foundation STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/foundation/Hasher.cpp
)
target_include_directories(aura_foundation PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Every behavioural test target: <target-name> <source-path>.
set(AURA_TEST_TARGETS
  "DegradationTests:src/resilience/DegradationTests.cpp"
  "RuntimeTests:src/runtime/RuntimeTests.cpp"
  "TimeframeStateTests:src/runtime/TimeframeStateTests.cpp"
  "AnalysisPipelineTests:src/runtime/AnalysisPipelineTests.cpp"
  "SignalPipelineTests:src/runtime/SignalPipelineTests.cpp"
  "ObservationTests:src/observation/ObservationTests.cpp"
  "LearningTests:src/learning/LearningTests.cpp"
  "ResearchTests:src/research/ResearchTests.cpp"
  "EvolutionTests:src/evolution/EvolutionTests.cpp"
  "ValidationTests:src/validation/ValidationTests.cpp"
  "GovernanceTests:src/governance/GovernanceTests.cpp"
  "OperatingWindowTests:src/operatingwindow/OperatingWindowTests.cpp"
  "DesktopTests:src/desktop/DesktopTests.cpp"
  "TelegramTests:src/telegram/TelegramTests.cpp"
  "RealWorldTests:src/realworld/RealWorldTests.cpp"
  "Mt5ReceiverTests:src/mt5/Mt5ReceiverTests.cpp"
  "EndToEndTests:src/runtime/EndToEndTests.cpp"
  "PersistenceTests:src/runtime/PersistenceTests.cpp"
)

find_package(Threads REQUIRED)

# Winsock is required by the Windows build of the socket transport.
set(AURA_PLATFORM_LIBS Threads::Threads)
if(WIN32)
  list(APPEND AURA_PLATFORM_LIBS ws2_32)
endif()

enable_testing()

# Real host executable (console host; the shadow-only operational entrypoint).
add_executable(aura ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_pipeline.cpp)
target_link_libraries(aura PRIVATE aura_foundation ${AURA_PLATFORM_LIBS})

foreach(entry IN LISTS AURA_TEST_TARGETS)
  string(REPLACE ":" ";" parts "${entry}")
  list(GET parts 0 target)
  list(GET parts 1 source)
  add_executable(${target} ${CMAKE_CURRENT_SOURCE_DIR}/${source})
  target_link_libraries(${target} PRIVATE aura_foundation ${AURA_PLATFORM_LIBS})
  add_test(NAME ${target} COMMAND ${target})
endforeach()

enable_testing()

message(STATUS "AURA: ${CMAKE_CXX_STANDARD} | tests: ${CMAKE_CXX_COMPILER_ID}")

# Packaging: the console host and public headers install cleanly. The rich desktop
# GUI is a separate Windows deliverable (see project-control/BLOCKED.md).
include(GNUInstallDirs)
install(TARGETS aura RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aura
        FILES_MATCHING PATTERN "*.h")
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
