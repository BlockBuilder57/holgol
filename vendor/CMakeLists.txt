# vendor CMakeLists.txt

add_subdirectory(websocketpp)

# nlohmann json
# We don't need tests, install.
# or single includes.
set(JSON_BuildTests OFF)
set(JSON_Install OFF)
set(JSON_MultipleHeaders ON)
# This will also make anything using ctest not build tests
set(BUILD_TESTING OFF CACHE BOOL "no tests" FORCE)
add_subdirectory(json)