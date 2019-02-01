cmake_minimum_required(VERSION 2.6)
if (USE_VALGRIND)
add_definitions(-DACO_USE_VALGRIND -std=c++11)
endif()
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/..)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/../../../src)

set(CMAKE_EXE_LINKER_FLAGS " -lpthread -ldl -lrt -lstdc++")
ENABLE_LANGUAGE(ASM)
aux_source_directory(./ _SRC)
aux_source_directory(../ _SRC)

add_executable(co_test ${_SRC} ../../acosw.S ${PROJECT_SOURCE_DIR}/../../../src/Coroutine.cpp)

TARGET_LINK_LIBRARIES(co_test
	-Wl,--whole-archive

	-Wl,--no-whole-archive
)

#add_dependencies(co_test libaco.a)

add_custom_command(TARGET co_test  PRE_BUILD
COMMAND echo "build start"
)

add_custom_command(TARGET co_test  POST_BUILD
COMMAND echo "build over"
)

