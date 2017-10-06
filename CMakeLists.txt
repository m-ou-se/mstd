project(mstd)
cmake_minimum_required(VERSION 3.1)

# Add an interface target for usage with add_subdirectory().
add_library(mstd INTERFACE)
target_include_directories(mstd INTERFACE include)

# Install headers.
include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Configure and install CMake configuration files.
configure_file(config.cmake.in "${PROJECT_NAME}Config.cmake")
set(CMAKE_INSTALL_CMAKEDIR "lib/cmake/${PROJECT_NAME}" CACHE PATH "CMake project files")
install(
	FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
	DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
)
