INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${FF_INSTALL_PATH}/include)
INCLUDE_DIRECTORIES(${FF_SRC_PATH})
set(CMAKE_EXE_LINKER_FLAGS " -lpthread -ldl -lrt -lstdc++ -L${PROJECT_BINARY_DIR}/lib")

aux_source_directory(./ _SRC)

add_executable(ff_test ${_SRC})

TARGET_LINK_LIBRARIES(ff_test
	-Wl,--whole-archive
	avcodec
	avutil
	avformat
	avfilter
	swresample
	swscale
	postproc
	aco
	-Wl,--no-whole-archive
)

add_dependencies(ff_test libaco.a)

add_custom_command(TARGET ff_test  PRE_BUILD
COMMAND echo "build start"
COMMAND cp ${FF_INSTALL_PATH}/lib/*.a ${PROJECT_BINARY_DIR}/lib
)

add_custom_command(TARGET ff_test  POST_BUILD
COMMAND echo "build over"
)

