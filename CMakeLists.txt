cmake_minimum_required(VERSION 3.10)

project(SimplePrinter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# PDFIUM
# =========================
set(PDFIUM_DIR ${CMAKE_SOURCE_DIR}/external/pdfium)

# =========================
# OPTIONS
# =========================
option(USE_WINDOWS_SUBSYSTEM "Build as Windows GUI app" ON)

# =========================
# SOURCES
# =========================
file(GLOB SOURCES
    "src/*.cpp"
    "src/app/*.cpp"
    "src/config/*.cpp"
    "src/controller/*.cpp"
    "src/controller/home/*.cpp"
    "src/controller/print/*.cpp"
    "src/counter/*.cpp"
    "src/platform/*.cpp"
    "src/printer/*.cpp"
    "src/renderer/*.cpp"
    "src/ui/*.cpp"
    "src/ui/home/*.cpp"
)

# =========================
# RESOURCE (optional)
# =========================
set(RESOURCE_FILE "${CMAKE_SOURCE_DIR}/resource/resource.rc")

# =========================
# BUILD TARGET
# =========================
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${RESOURCE_FILE}
)

# =========================
# INCLUDE
# =========================
target_include_directories(${PROJECT_NAME} PRIVATE
    include
    ${PDFIUM_DIR}/include
    resource
)

# =========================
# WINDOWS SETTINGS
# =========================
if (WIN32 AND USE_WINDOWS_SUBSYSTEM)
    target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
endif()

# =========================
# LINK LIBRARIES
# =========================
target_link_directories(${PROJECT_NAME} PRIVATE
    ${PDFIUM_DIR}/lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    gdiplus
    shell32
    msimg32
    comctl32
    uxtheme   
    pdfium
    ole32
)


# =========================
# COMPILE OPTIONS
# =========================
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O2
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
)

# =========================
# STATIC (UCRT64 / MINGW)
# =========================
if (MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
        -Wl,--allow-multiple-definition
    )
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PDFIUM_DIR}/bin/pdfium.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)





# =========================
# LAUNCHER
# =========================
add_executable(SimplePrinterLauncher
    launcher/launcher.cpp
)

# QUAN TRỌNG
set_target_properties(SimplePrinterLauncher PROPERTIES
    WIN32_EXECUTABLE OFF
)

target_link_options(SimplePrinterLauncher PRIVATE
    -municode
)

target_compile_definitions(SimplePrinterLauncher PRIVATE
    UNICODE
    _UNICODE
)

target_compile_options(SimplePrinterLauncher PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O2
)

if (MINGW)
    target_link_options(SimplePrinterLauncher PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
        -mconsole
    )
endif()