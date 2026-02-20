cmake_minimum_required(VERSION 3.16)
project(tdr-lsp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../scene-io scene-io-build EXCLUDE_FROM_ALL)

add_executable(tdr-lsp lsp/main.cpp)

target_compile_features(tdr-lsp PUBLIC cxx_std_23)

# Platform-specific compiler flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# Linux and macOS support
	target_compile_options(tdr-lsp PRIVATE -std=gnu++2b -Wall -Wextra)
	# Link stdc++fs on Linux (required for std::filesystem)
	if (UNIX AND NOT APPLE)
		target_link_libraries(tdr-lsp PRIVATE stdc++fs)
	endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# Windows MSVC compiler
	target_compile_options(tdr-lsp PRIVATE /std:c++latest /W4)
endif()

target_link_libraries(tdr-lsp PRIVATE scene-io)

target_include_directories(tdr-lsp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lsp)

set_target_properties(tdr-lsp PROPERTIES OUTPUT_NAME "tdr-lsp")

# Install configuration
include(GNUInstallDirs)
install(TARGETS tdr-lsp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
