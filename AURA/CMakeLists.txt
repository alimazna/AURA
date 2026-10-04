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
  # Link the CRT statically so the shipped aura.exe is self-contained and does
  # not require the Visual C++ redistributable on a clean target machine.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
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

# ---------------------------------------------------------------------------
# Desktop GUI (Phase 9 / GUI-0001).
#
# Dear ImGui + GLFW + OpenGL presentation over the existing runtime. The GUI is
# OPTIONAL (default OFF) so the core/console build and the Linux test matrix stay
# dependency-free. When enabled, the dependencies are pinned and fetched
# reproducibly (no system-installed copies, no developer paths).
#
#   aura_gui --gui       interactive control center
#   aura_gui --self-test headless integration smoke (what CI proves)
# ---------------------------------------------------------------------------
option(AURA_BUILD_GUI "Build the AURA desktop control center (Dear ImGui + GLFW + OpenGL)" OFF)

if(AURA_BUILD_GUI)
  include(FetchContent)

  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  # Prefer X11 over Wayland on Linux (Wayland needs wayland-scanner, which is not
  # guaranteed on CI runners). These hints are inert on Windows/macOS.
  set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
  set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
  )
  FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.9
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(glfw imgui)

  # ImGui has no upstream CMake build; compile the core + GLFW/OpenGL3 backends as
  # a static library. The OpenGL3 backend uses ImGui's embedded GL loader, so no
  # GLAD/GLEW dependency is required. The OpenGL2 backend (fixed-function, GLSL
  # #version 120) is the legacy fallback for hardware without an OpenGL 3.3
  # context (e.g. Intel HD Graphics 3000); it needs no loader either.
  add_library(aura_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
  )
  target_include_directories(aura_imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
  )
  target_link_libraries(aura_imgui PUBLIC glfw)

  find_package(OpenGL REQUIRED)
  target_link_libraries(aura_imgui PUBLIC OpenGL::GL)

  # Optional real ASTRA brand mark. stb_image (bundled, single header, public
  # domain) is only used to upload a PNG/JPEG logo when one is actually present;
  # if the loader is unavailable the header simply draws the wordmark. The asset
  # directory is staged next to the binary and copied to the packaged app.
  add_executable(aura_gui ${CMAKE_CURRENT_SOURCE_DIR}/tools/aura_gui.cpp)
  target_link_libraries(aura_gui PRIVATE aura_foundation aura_imgui ${AURA_PLATFORM_LIBS})
  target_include_directories(aura_gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
  target_compile_definitions(aura_gui PRIVATE
    AURA_HAVE_STB_IMAGE=1
    AURA_BRAND_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/brand"
  )
  # The AURA terminal type roles (Roboto-Medium / Cousine) ship with Dear ImGui.
  # Point the executable at their source location, and also stage a copy next to
  # the binary under `fonts/` so the build tree and packaged app are self-contained.
  target_compile_definitions(aura_gui PRIVATE
    AURA_FONT_DIR="${imgui_SOURCE_DIR}/misc/fonts")
  add_custom_command(TARGET aura_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:aura_gui>/fonts
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${imgui_SOURCE_DIR}/misc/fonts/Roboto-Medium.ttf
            ${imgui_SOURCE_DIR}/misc/fonts/Cousine-Regular.ttf
            $<TARGET_FILE_DIR:aura_gui>/fonts
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:aura_gui>/assets/brand
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/brand
            $<TARGET_FILE_DIR:aura_gui>/assets/brand
    COMMENT "Staging AURA terminal fonts and brand assets")

  # Headless GUI integration smoke, registered with CTest. It exercises the same
  # runtime ownership, recovery evaluation and snapshot projection the window
  # uses (nine-timeframe mapping, shadow-only invariant, checkpoint, recovery
  # report) without a rendering surface. Interactive runtime is NOT verified here.
  add_test(NAME GuiSelfTest COMMAND aura_gui --self-test)

  install(TARGETS aura_gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  # Stage the terminal fonts and the brand asset directory beside the binary so
  # the packaged app is self-contained.
  install(FILES
    ${imgui_SOURCE_DIR}/misc/fonts/Roboto-Medium.ttf
    ${imgui_SOURCE_DIR}/misc/fonts/Cousine-Regular.ttf
    DESTINATION ${CMAKE_INSTALL_BINDIR}/fonts)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets/brand/
          DESTINATION ${CMAKE_INSTALL_BINDIR}/assets/brand)
endif()

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
