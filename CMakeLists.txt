cmake_minimum_required(VERSION 3.20)

project(ProjectPhoenix 
    VERSION 0.1.0 
    LANGUAGES CXX 
    DESCRIPTION "Kerbal Space Program Clone - C++20"
)

# ============================================================================
# Configuración global
# ============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================================================
# Dependencias externas
# ============================================================================

# GLM (header-only)
# Si GLM no está disponible en el sistema, puede descargarse desde:
# https://github.com/g-truc/glm/releases
# y colocarse en: extern/glm/ o /usr/include/

find_package(glm QUIET)
if(NOT glm_FOUND)
    message(STATUS "GLM not found via find_package, searching for headers...")
    
    # Try to find GLM headers in common locations
    find_path(GLM_INCLUDE_DIR 
        NAMES glm/glm.hpp
        PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        /opt/homebrew/include
        ${CMAKE_CURRENT_SOURCE_DIR}/extern
        ${CMAKE_CURRENT_SOURCE_DIR}
        ENV GLM_PATH
    )
    
    if(GLM_INCLUDE_DIR)
        message(STATUS "✓ GLM headers found at: ${GLM_INCLUDE_DIR}")
        # Create a INTERFACE library for GLM
        add_library(glm INTERFACE IMPORTED)
        set_target_properties(glm PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE_DIR}"
        )
    else()
        message(FATAL_ERROR 
            "❌ GLM headers not found!\n"
            "\n"
            "Please install GLM from one of these options:\n"
            "1. Ubuntu/Debian: sudo apt-get install libglm-dev\n"
            "2. macOS: brew install glm\n"
            "3. Fedora: sudo dnf install glm-devel\n"
            "4. Download from GitHub:\n"
            "   https://github.com/g-truc/glm/releases\n"
            "   Extract to: ${CMAKE_CURRENT_SOURCE_DIR}/extern/\n"
            "\n"
            "Or set GLM path: cmake -DGLM_INCLUDE_DIR=/path/to/glm .."
        )
    endif()
endif()

# ============================================================================
# Directorios
# ============================================================================

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ============================================================================
# Archivos fuente
# ============================================================================

set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/physics/Orbit.cpp
    ${SRC_DIR}/physics/CelestialBody.cpp
    ${SRC_DIR}/physics/Atmosphere.cpp
    ${SRC_DIR}/physics/AeroForces.cpp
    ${SRC_DIR}/parts/Part.cpp
    ${SRC_DIR}/parts/Engine.cpp
    ${SRC_DIR}/vessels/Vessel.cpp
    ${SRC_DIR}/world/WorldManager.cpp
    ${SRC_DIR}/world/SphereOfInfluence.cpp
    ${SRC_DIR}/ui/AsciiRenderer.cpp
    ${SRC_DIR}/ui/MissionDisplay.cpp
)

set(HEADERS
    ${INCLUDE_DIR}/math/Constants.hpp
    ${INCLUDE_DIR}/physics/Orbit.hpp
    ${INCLUDE_DIR}/physics/CelestialBody.hpp
    ${INCLUDE_DIR}/physics/Atmosphere.hpp
    ${INCLUDE_DIR}/physics/AeroForces.hpp
    ${INCLUDE_DIR}/parts/Part.hpp
    ${INCLUDE_DIR}/parts/Engine.hpp
    ${INCLUDE_DIR}/vessels/Vessel.hpp
    ${INCLUDE_DIR}/world/WorldManager.hpp
    ${INCLUDE_DIR}/world/SphereOfInfluence.hpp
    ${INCLUDE_DIR}/ui/AsciiRenderer.hpp
    ${INCLUDE_DIR}/ui/MissionDisplay.hpp
)

# ============================================================================
# Ejecutable principal
# ============================================================================

add_executable(phoenix ${SOURCES} ${HEADERS})

target_include_directories(phoenix 
    PRIVATE 
        ${INCLUDE_DIR}
)

target_link_libraries(phoenix
    PRIVATE
        glm::glm
)

# ============================================================================
# Opciones de compilación por plataforma
# ============================================================================

if(UNIX AND NOT APPLE)
    # Linux
    target_compile_options(phoenix PRIVATE -Wall -Wextra -Wpedantic)
elseif(APPLE)
    # macOS
    target_compile_options(phoenix PRIVATE -Wall -Wextra -Wpedantic)
elseif(WIN32)
    # Windows (MSVC)
    target_compile_options(phoenix PRIVATE /W4)
endif()

# ============================================================================
# Propiedades del proyecto
# ============================================================================

set_target_properties(phoenix PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    VERSION ${PROJECT_VERSION}
)

# ============================================================================
# Instalación (opcional)
# ============================================================================

install(TARGETS phoenix RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/phoenix)

# ============================================================================
# Demo 3D — phoenix_3d  (requiere Raylib 5)
# ============================================================================

# Physics-only sources (no UI, no main)
set(PHYSICS_SOURCES
    ${SRC_DIR}/physics/Orbit.cpp
    ${SRC_DIR}/physics/CelestialBody.cpp
    ${SRC_DIR}/physics/Atmosphere.cpp
    ${SRC_DIR}/physics/AeroForces.cpp
    ${SRC_DIR}/parts/Part.cpp
    ${SRC_DIR}/parts/Engine.cpp
    ${SRC_DIR}/vessels/Vessel.cpp
    ${SRC_DIR}/world/WorldManager.cpp
    ${SRC_DIR}/world/SphereOfInfluence.cpp
)

find_package(raylib 5 QUIET)
if(NOT raylib_FOUND)
    find_library(RAYLIB_LIB raylib
        PATHS /opt/homebrew/lib /usr/local/lib /usr/lib)
    find_path(RAYLIB_INCLUDE raylib.h
        PATHS /opt/homebrew/include /usr/local/include /usr/include)
    if(RAYLIB_LIB AND RAYLIB_INCLUDE)
        add_library(raylib SHARED IMPORTED)
        set_target_properties(raylib PROPERTIES
            IMPORTED_LOCATION "${RAYLIB_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${RAYLIB_INCLUDE}")
        set(raylib_FOUND TRUE)
    endif()
endif()

if(raylib_FOUND)
    message(STATUS "✓ Raylib found — building phoenix_3d")
    add_executable(phoenix_3d
        ${SRC_DIR}/demo_3d.cpp
        ${PHYSICS_SOURCES}
    )
    target_include_directories(phoenix_3d PRIVATE ${INCLUDE_DIR})
    target_link_libraries(phoenix_3d PRIVATE glm::glm raylib)
    if(APPLE)
        target_link_libraries(phoenix_3d PRIVATE
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
        )
        target_compile_options(phoenix_3d PRIVATE -Wall -Wextra)
    elseif(UNIX)
        target_link_libraries(phoenix_3d PRIVATE GL m pthread dl rt X11)
        target_compile_options(phoenix_3d PRIVATE -Wall -Wextra)
    endif()
    set_target_properties(phoenix_3d PROPERTIES
        CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
else()
    message(STATUS "⚠ Raylib not found — phoenix_3d skipped (brew install raylib)")
endif()


# ============================================================================
# Información de compilación
# ============================================================================

message(STATUS "╔════════════════════════════════════════╗")
message(STATUS "║    PROJECT PHOENIX - CMake Config      ║")
message(STATUS "╠════════════════════════════════════════╣")
message(STATUS "│ Version: ${PROJECT_VERSION}")
message(STATUS "│ C++ Standard: 20")
message(STATUS "│ Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "│ Source: ${SRC_DIR}")
message(STATUS "│ Include: ${INCLUDE_DIR}")
message(STATUS "│ GLM: ${glm_DIR}")
message(STATUS "╚════════════════════════════════════════╝")
