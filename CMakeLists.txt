cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    flox
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Copy data files ----
file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data" DESTINATION "share/flox")

# ---- Configuration ----
configure_file(src/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h @ONLY)

find_package(fmt REQUIRED)
find_package(gfx REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)

# ---- Declare executable ----

add_executable(flox_exe src/main.cpp src/types.cpp)
add_executable(flox::exe ALIAS flox_exe)

set_property(TARGET flox_exe PROPERTY OUTPUT_NAME flox)

target_compile_features(flox_exe PRIVATE cxx_std_20)

target_link_libraries(flox_exe PRIVATE fmt::fmt gfx::gfx)

target_include_directories(
    flox_exe ${warning_guard}
    PUBLIC
    "${SDL2_INCLUDE_DIRS}"
    "${SDL2_IMAGE_INCLUDE_DIRS}"
    "${SDL2_TTF_INCLUDE_DIRS}"
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)
# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT flox_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of flox"
  )
endif()

include(cmake/dev-mode.cmake)
