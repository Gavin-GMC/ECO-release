cmake_minimum_required(VERSION 3.15)

# Compatibility entry matching the layout of earlier release packages.
# The authoritative build definition remains the project-root CMakeLists.txt.
project(evoflow_release_cmake_entry LANGUAGES NONE)

get_filename_component(EVOFLOW_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_subdirectory("${EVOFLOW_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/evoflow-root")
