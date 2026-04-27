cmake_minimum_required(VERSION 3.10)

project(SimplePrinter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# OPTIONS
# =========================
option(USE_WINDOWS_SUBSYSTEM "Build as Windows GUI app" ON)

# =========================
# SOURCES
# =========================
file(GLOB SOURCES
    "src/*.cpp"
    "src/ui/*.cpp"
    "src/ui/components/*.cpp"
    "src/app/*.cpp"
)

# =========================
# RESOURCE (optional)
# =========================
set(RESOURCE_FILE "")
if (EXISTS "${CMAKE_SOURCE_DIR}/resource/resource.rc")
    set(RESOURCE_FILE resource/resource.rc)
endif()

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
    include/app
    include/ui
    include/ui/components
    resource
)

# =========================
# WINDOWS SETTINGS
# =========================
# if (WIN32 AND USE_WINDOWS_SUBSYSTEM)
#     set_target_properties(${PROJECT_NAME} PROPERTIES
#         LINK_FLAGS "-mwindows"
#     )
# endif()

# =========================
# LINK LIBRARIES
# =========================
target_link_libraries(${PROJECT_NAME} PRIVATE
    gdiplus
    shell32
    msimg32
    comctl32
    uxtheme   
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