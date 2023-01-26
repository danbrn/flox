# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(floxTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(flox_test src/flox_test.cpp)
target_link_libraries(
    flox_test PRIVATE
    gfx::gfx
    Catch2::Catch2WithMain
)
target_compile_features(flox_test PRIVATE cxx_std_20)

target_include_directories(
    flox_test PRIVATE
    "${GFX_INCLUDE_DIRS}"
    "${SDL2_INCLUDE_DIRS}"
    "${SDL2_IMAGE_INCLUDE_DIRS}"
    "${SDL2_TTF_INCLUDE_DIRS}"
)

catch_discover_tests(flox_test)

# ---- End-of-file commands ----

add_folders(Test)
