cmake_minimum_required(VERSION 2.8)
if (USE_VALGRIND)
add_definitions(-DACO_USE_VALGRIND)
endif()

INCLUDE_DIRECTORIES(./)
ENABLE_LANGUAGE(ASM)
aux_source_directory(./ _SRC)

add_library(aco STATIC ${_SRC} acosw.S)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
