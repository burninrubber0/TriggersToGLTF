cmake_minimum_required(VERSION 3.14)
project(TriggersToGLTF CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCES
	${SOURCES}
	src/main.cpp
	src/converter.cpp
	src/trigger-data.cpp
	src/binary-io/data-stream.cpp
	)

set(HEADERS
	${HEADERS}
	include/converter.h
	include/trigger-data.h
	include/types.h
	include/binary-io/data-stream.h
	)

add_executable(TriggersToGLTF ${SOURCES} ${HEADERS})

# Qt
find_package(Qt6 COMPONENTS Core REQUIRED)

# tinygltf
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory("${ROOT}/external/tinygltf")

target_include_directories(TriggersToGLTF PRIVATE "${ROOT}/include" "${ROOT}/external/tinygltf")
target_link_libraries(TriggersToGLTF PRIVATE Qt6::Core)

# VS stuff
set_property(DIRECTORY ${ROOT} PROPERTY VS_STARTUP_PROJECT TriggersToGLTF)
source_group(TREE ${ROOT} FILES ${SOURCES} ${HEADERS})

if (WIN32)
	add_custom_command(TARGET TriggersToGLTF POST_BUILD
		COMMAND Qt6::windeployqt ARGS $<TARGET_FILE:TriggersToGLTF>
	)
endif()
