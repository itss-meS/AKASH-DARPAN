# ==============================================================
#  AKASH-DARPAN  —  CMakeLists.txt
#  आकाश दर्पण  |  Sky Mirror
#  Adaptive Knowledge of Atmospheric Shack-Hartmann
#  Dynamic Adaptive Real-time Processing & Reconstruction Node
#
#  ISRO Adaptive Optics Mission Control System  —  v1.0.0
#
#  Required:
#    CMake  >= 3.16
#    C++17  (GCC 9+ | Clang 10+ | MSVC 2019+)
#    OpenGL  3.3 Core
#    GLFW3   3.3+
#    GLAD    (OpenGL 3.3 loader)  → third_party/glad/
#    ImGui   v1.90+               → third_party/imgui/
#    ImPlot                       → third_party/implot/
#    stb_image.h                  → third_party/stb/
#
#  Optional (improves performance):
#    OpenMP  — parallel centroiding across sub-apertures
#    AVX2    — SIMD fast-math (auto via -march=native)
#
#  Usage:
#    mkdir build && cd build
#    cmake .. -DCMAKE_BUILD_TYPE=Release
#    cmake --build . --parallel
#    ./bin/akash_darpan [path/to/bmp/frames]
# ==============================================================

cmake_minimum_required(VERSION 3.16)

# ── Project declaration ────────────────────────────────────
project(AKASH_DARPAN
    VERSION      1.0.0
    DESCRIPTION  "ISRO Adaptive Optics Mission Control System (SH-WFS)"
    HOMEPAGE_URL "https://www.isro.gov.in"
    LANGUAGES    C CXX
)

# ── C++ standard ──────────────────────────────────────────
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)   # strict ISO C++17
set(CMAKE_C_STANDARD            11)
set(CMAKE_C_STANDARD_REQUIRED   ON)

# ── Default build type ─────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Build type: Debug | Release | RelWithDebInfo | MinSizeRel" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
        STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# ── Output directories ─────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ── Export compile commands (for clangd / IDEs) ────────────
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================
#  COMPILER FLAGS
# ==============================================================
if(MSVC)
    # ── MSVC (Visual Studio) ─────────────────────────────
    add_compile_options(
        /W3                             # Warning level 3
        /wd4244                         # narrowing: int→float (in ImGui)
        /wd4267                         # size_t→int  (in ImGui)
        /wd4305                         # double→float truncation
        /MP                             # multi-processor compilation
        $<$<CONFIG:Release>:/O2>        # optimise for speed
        $<$<CONFIG:Release>:/arch:AVX2> # AVX2 SIMD
        $<$<CONFIG:Release>:/fp:fast>   # fast floating-point
        $<$<CONFIG:Release>:/GL>        # whole-program optimisation
        $<$<CONFIG:Debug>:/Od>          # no optimisation
        $<$<CONFIG:Debug>:/Zi>          # debug symbols
        $<$<CONFIG:Debug>:/RTC1>        # run-time checks
    )
    add_link_options(
        $<$<CONFIG:Release>:/LTCG>      # link-time code generation
    )
    add_definitions(
        -D_CRT_SECURE_NO_WARNINGS       # suppress fopen/sscanf warnings
        -DNOMINMAX                      # prevent min/max macro conflicts
        -D_USE_MATH_DEFINES             # M_PI in <cmath>
    )
else()
    # ── GCC / Clang ──────────────────────────────────────
    add_compile_options(
        -Wall -Wextra
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-march=native>   # use all available CPU features
        $<$<CONFIG:Release>:-ffast-math>
        $<$<CONFIG:Release>:-funroll-loops>
        $<$<CONFIG:Release>:-fomit-frame-pointer>
        $<$<CONFIG:Release>:>           # link-time optimisation
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g3>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
    )
    # AddressSanitizer in Debug (opt-in via -DAKASH_ASAN=ON)
    option(AKASH_ASAN "Enable AddressSanitizer in Debug builds" OFF)
    if(AKASH_ASAN)
        add_compile_options($<$<CONFIG:Debug>:-fsanitize=address,undefined>)
        add_link_options   ($<$<CONFIG:Debug>:-fsanitize=address,undefined>)
    endif()
    # LTO linker flag (GCC needs it on link line too)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_link_options($<$<CONFIG:Release>:>)
    endif()
endif()

# ==============================================================
#  FIND SYSTEM PACKAGES
# ==============================================================

# ── OpenGL ────────────────────────────────────────────────
find_package(OpenGL REQUIRED)
message(STATUS "[AKASH] OpenGL       : ${OPENGL_gl_LIBRARY}")

# ── GLFW3 — try CMake config → pkg-config → manual ────────
find_package(glfw3 3.3 QUIET CONFIG)
if(TARGET glfw)
    set(AKASH_GLFW_TARGET glfw)
    message(STATUS "[AKASH] GLFW3        : CMake config (target: glfw)")
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GLFW3 QUIET glfw3)
    endif()
    if(GLFW3_FOUND)
        message(STATUS "[AKASH] GLFW3        : pkg-config  (${GLFW3_LIBRARIES})")
    else()
        find_library(GLFW3_LIBRARY NAMES glfw glfw3
            HINTS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
                  /opt/homebrew/lib)
        find_path(GLFW3_INCLUDE_DIR NAMES GLFW/glfw3.h
            HINTS /usr/include /usr/local/include /opt/homebrew/include)
        if(GLFW3_LIBRARY AND GLFW3_INCLUDE_DIR)
            set(GLFW3_FOUND      TRUE)
            set(GLFW3_LIBRARIES  ${GLFW3_LIBRARY})
            set(GLFW3_INCLUDE_DIRS ${GLFW3_INCLUDE_DIR})
            message(STATUS "[AKASH] GLFW3        : manual    (${GLFW3_LIBRARY})")
        else()
            message(FATAL_ERROR
                "[AKASH] GLFW3 not found. Install with:\n"
                "  Ubuntu/Debian : sudo apt install libglfw3-dev\n"
                "  Fedora/RHEL   : sudo dnf install glfw-devel\n"
                "  macOS         : brew install glfw\n"
                "  Windows       : vcpkg install glfw3:x64-windows\n"
            )
        endif()
    endif()
endif()

# ── OpenMP (optional — parallel centroiding) ──────────────
find_package(OpenMP QUIET)
if(OpenMP_CXX_FOUND)
    message(STATUS "[AKASH] OpenMP       : enabled  (${OpenMP_CXX_VERSION})")
else()
    message(STATUS "[AKASH] OpenMP       : not found (centroiding single-threaded)")
endif()

# ── Threads (pthreads on Linux) ────────────────────────────
find_package(Threads REQUIRED)

# ==============================================================
#  THIRD-PARTY DEPENDENCY CHECKS
# ==============================================================
set(AKASH_THIRD_PARTY_OK TRUE)
foreach(_dep
    "third_party/glad/src/glad.c"
    "third_party/glad/include/glad/glad.h"
    "third_party/glad/include/KHR/khrplatform.h"
    "third_party/imgui/imgui.h"
    "third_party/imgui/imgui.cpp"
    "third_party/imgui/imgui_draw.cpp"
    "third_party/imgui/imgui_tables.cpp"
    "third_party/imgui/imgui_widgets.cpp"
    "third_party/imgui/backends/imgui_impl_glfw.h"
    "third_party/imgui/backends/imgui_impl_glfw.cpp"
    "third_party/imgui/backends/imgui_impl_opengl3.h"
    "third_party/imgui/backends/imgui_impl_opengl3.cpp"
    "third_party/implot/implot.h"
    "third_party/implot/implot.cpp"
    "third_party/implot/implot_items.cpp"
    "third_party/stb/stb_image.h"
)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_dep}")
        message(WARNING "[AKASH] Missing dependency: ${_dep}")
        set(AKASH_THIRD_PARTY_OK FALSE)
    endif()
endforeach()

if(NOT AKASH_THIRD_PARTY_OK)
    message(WARNING
        "[AKASH] Some third-party files are missing.\n"
        "        Run:  ./setup_and_build.sh   (Linux/macOS)\n"
        "        Or:   build_windows.bat      (Windows)\n"
        "        See:  docs/TECHNICAL_REFERENCE.md  Section 4"
    )
endif()

# ==============================================================
#  THIRD-PARTY STATIC LIBRARIES
# ==============================================================

# ── GLAD — OpenGL 3.3 Core function loader ─────────────────
add_library(glad STATIC
    third_party/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    third_party/glad/include
)
if(UNIX AND NOT APPLE)
    target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
endif()
set_target_properties(glad PROPERTIES
    C_STANDARD           11
    C_STANDARD_REQUIRED  ON
    POSITION_INDEPENDENT_CODE ON
)

# ── Dear ImGui — immediate-mode GUI ───────────────────────
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_compile_definitions(imgui PUBLIC
    IMGUI_IMPL_OPENGL_LOADER_GLAD   # tell imgui_impl_opengl3 to use GLAD
)
target_link_libraries(imgui PUBLIC glad OpenGL::GL)

# Link GLFW to imgui backend
if(TARGET glfw)
    target_link_libraries(imgui PUBLIC glfw)
else()
    target_include_directories(imgui PUBLIC ${GLFW3_INCLUDE_DIRS})
    target_link_libraries(imgui PUBLIC ${GLFW3_LIBRARIES})
endif()

# Suppress all third-party warnings
if(NOT MSVC)
    target_compile_options(imgui PRIVATE -w)
endif()

# ── ImPlot — scientific plotting widget ───────────────────
set(IMPLOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot")

add_library(implot STATIC
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_items.cpp
)
target_include_directories(implot PUBLIC ${IMPLOT_DIR})
target_link_libraries(implot PUBLIC imgui)

if(NOT MSVC)
    target_compile_options(implot PRIVATE -w)
endif()

# ==============================================================
#  AKASH-DARPAN CORE LIBRARY
#  Separating core logic from main() enables unit testing
#  and future embedding into other applications.
# ==============================================================
set(AKASH_CORE_SOURCES
    src/wavefront_sensor.cpp
    src/centroid_engine.cpp
    src/wavefront_reconstructor.cpp
    src/dm_controller.cpp
    src/turbulence_analyzer.cpp
    src/image_loader.cpp
    src/renderer.cpp
    src/telemetry.cpp
    src/playback_controller.cpp
)

set(AKASH_CORE_HEADERS
    include/akash_types.h
    include/calibration_manager.h
    include/centroid_engine.h
    include/config_parser.h
    include/dm_controller.h
    include/image_loader.h
    include/math_utils.h
    include/playback_controller.h
    include/renderer.h
    include/shader_loader.h
    include/telemetry.h
    include/turbulence_analyzer.h
    include/wavefront_reconstructor.h
    include/wavefront_sensor.h
)

add_library(akash_core STATIC
    ${AKASH_CORE_SOURCES}
    ${AKASH_CORE_HEADERS}   # listed for IDE visibility
)

# Public include paths (propagated to any target that links akash_core)
target_include_directories(akash_core PUBLIC
    include                          # AKASH-DARPAN headers
    third_party/glad/include         # glad/glad.h
    third_party/stb                  # stb_image.h
    ${IMGUI_DIR}                     # imgui.h
    ${IMGUI_DIR}/backends            # imgui_impl_*.h
    ${IMPLOT_DIR}                    # implot.h
)

# Core compile definitions
target_compile_definitions(akash_core PUBLIC
    AKASH_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    AKASH_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    AKASH_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    AKASH_VERSION_STRING="${PROJECT_VERSION}"
    $<$<CONFIG:Debug>:AKASH_DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG>
)

# Link dependencies
target_link_libraries(akash_core PUBLIC
    imgui                            # Dear ImGui (includes GLAD + OpenGL + GLFW)
    implot                           # ImPlot charts
    OpenGL::GL                       # OpenGL
    Threads::Threads                 # pthreads on Linux
)

# OpenMP: parallel centroiding over sub-apertures
if(OpenMP_CXX_FOUND)
    target_link_libraries(akash_core PUBLIC OpenMP::OpenMP_CXX)
    target_compile_definitions(akash_core PUBLIC AKASH_USE_OPENMP=1)
endif()

# Platform-specific system libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(akash_core PUBLIC m)   # libm (math)
elseif(WIN32)
    target_link_libraries(akash_core PUBLIC ws2_32 opengl32)
elseif(APPLE)
    find_library(COCOA_LIB     Cocoa)
    find_library(IOKIT_LIB     IOKit)
    find_library(COREVIDEO_LIB CoreVideo)
    target_link_libraries(akash_core PUBLIC
        ${COCOA_LIB} ${IOKIT_LIB} ${COREVIDEO_LIB})
endif()

# ==============================================================
#  MAIN EXECUTABLE  —  akash_darpan
# ==============================================================
add_executable(akash_darpan
    src/main.cpp
)

target_link_libraries(akash_darpan PRIVATE
    akash_core
)

# macOS app bundle
if(APPLE)
    set_target_properties(akash_darpan PROPERTIES
        MACOSX_BUNDLE             TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "AKASH-DARPAN"
        MACOSX_BUNDLE_INFO_STRING "ISRO Adaptive Optics Mission Control"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION       "${PROJECT_VERSION}"
    )
endif()

# Windows: GUI subsystem in Release (suppress console window)
if(WIN32)
    set_target_properties(akash_darpan PROPERTIES
        WIN32_EXECUTABLE $<$<CONFIG:Release>:TRUE>
    )
endif()

# Linux RPATH (find shared libs at runtime relative to binary)
if(UNIX AND NOT APPLE)
    set_target_properties(akash_darpan PROPERTIES
        INSTALL_RPATH        "$ORIGIN/../lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# ==============================================================
#  SYNTHETIC DATA GENERATOR TOOL
#  Standalone tool — no GUI / ImGui dependency
# ==============================================================
add_executable(gen_synthetic_data
    tools/gen_synthetic_data.cpp
)
set_target_properties(gen_synthetic_data PROPERTIES
    CXX_STANDARD           17
    CXX_STANDARD_REQUIRED  ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(NOT MSVC)
    target_compile_options(gen_synthetic_data PRIVATE -O2)
endif()

# ==============================================================
#  UNIT TESTS  (CTest integration)
# ==============================================================
enable_testing()

add_executable(test_core
    tests/test_core.cpp
)
target_include_directories(test_core PRIVATE
    include                      # akash_types.h, math_utils.h, etc.
)
# test_core is header-only — no akash_core link needed
set_target_properties(test_core PROPERTIES
    CXX_STANDARD          17
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(NOT MSVC)
    target_compile_options(test_core PRIVATE -O1 -Wall)
endif()

# Register with CTest
add_test(NAME AkashCoreTests
    COMMAND test_core
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
set_tests_properties(AkashCoreTests PROPERTIES
    TIMEOUT 30
    LABELS  "unit"
)

# ==============================================================
#  POST-BUILD STEPS  —  copy runtime assets
# ==============================================================

# 1. Shaders (loaded from disk at runtime for hot-reload support)
add_custom_command(TARGET akash_darpan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:akash_darpan>/shaders"
    COMMENT "[AKASH-DARPAN] Copying GLSL shaders → build/bin/shaders/"
)

# 2. Default config file
add_custom_command(TARGET akash_darpan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/akash_config.ini"
        "$<TARGET_FILE_DIR:akash_darpan>/akash_config.ini"
    COMMENT "[AKASH-DARPAN] Copying akash_config.ini → build/bin/"
)

# 3. Documentation
add_custom_command(TARGET akash_darpan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/docs"
        "$<TARGET_FILE_DIR:akash_darpan>/docs"
    COMMENT "[AKASH-DARPAN] Copying docs → build/bin/docs/"
)

# 4. Create empty data/ directory
add_custom_command(TARGET akash_darpan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:akash_darpan>/data"
    COMMENT "[AKASH-DARPAN] Creating data/ directory"
)

# ==============================================================
#  INSTALL RULES
# ==============================================================
include(GNUInstallDirs)

install(TARGETS akash_darpan gen_synthetic_data
    RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"
    BUNDLE   DESTINATION "."
)
install(DIRECTORY shaders/
    DESTINATION "${CMAKE_INSTALL_BINDIR}/shaders"
)
install(DIRECTORY docs/
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)
install(FILES akash_config.ini README.md
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

# ==============================================================
#  CONFIGURATION SUMMARY  (printed at cmake .. time)
# ==============================================================
string(REPEAT "═" 56 _line)
message(STATUS "")
message(STATUS "╔${_line}╗")
message(STATUS "║  AKASH-DARPAN  v${PROJECT_VERSION}  —  Configuration Summary     ║")
message(STATUS "╠${_line}╣")
message(STATUS "║  Compiler       : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "║  C++ standard   : C++${CMAKE_CXX_STANDARD}")
message(STATUS "║  Build type     : ${CMAKE_BUILD_TYPE}")
message(STATUS "║  OpenGL         : ${OPENGL_gl_LIBRARY}")
message(STATUS "║  OpenMP         : ${OpenMP_CXX_FOUND}")
message(STATUS "║  ASAN (Debug)   : ${AKASH_ASAN}")
message(STATUS "║  Install prefix : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "╠${_line}╣")
message(STATUS "║  Build targets:")
message(STATUS "║    akash_darpan        — Mission Control GUI")
message(STATUS "║    gen_synthetic_data  — BMP sequence generator")
message(STATUS "║    test_core           — Unit tests (ctest)")
message(STATUS "╠${_line}╣")
message(STATUS "║  Quick-start:")
message(STATUS "║    cmake --build . --parallel")
message(STATUS "║    ctest --output-on-failure           # run tests")
message(STATUS "║    ./bin/gen_synthetic_data ./data 200 # generate frames")
message(STATUS "║    ./bin/akash_darpan [./data]         # launch dashboard")
message(STATUS "╠${_line}╣")
message(STATUS "║  Keyboard shortcuts in the dashboard:")
message(STATUS "║    SPACE  — play / pause")
message(STATUS "║    ← →   — step frame")
message(STATUS "║    R      — reset session")
message(STATUS "║    S      — save config")
message(STATUS "║    C      — toggle calibration window")
message(STATUS "║    F5     — hot-reload GLSL shaders")
message(STATUS "╚${_line}╝")
message(STATUS "")

# ==============================================================
#  PHYSICS TESTS
# ==============================================================
add_executable(test_physics
    tests/test_physics.cpp
)
target_include_directories(test_physics PRIVATE include)
set_target_properties(test_physics PROPERTIES
    CXX_STANDARD 17
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(NOT MSVC)
    target_compile_options(test_physics PRIVATE -O1 -Wall)
endif()

add_test(NAME AkashPhysicsTests
    COMMAND test_physics
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
set_tests_properties(AkashPhysicsTests PROPERTIES
    TIMEOUT 30
    LABELS  "physics"
)
