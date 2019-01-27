INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${FF_INSTALL_PATH}/include)
INCLUDE_DIRECTORIES(${FF_SRC_PATH})
set(CMAKE_EXE_LINKER_FLAGS " -lpthread -ldl -lrt -lstdc++ -L${FF_INSTALL_PATH}/lib -lpthread")

aux_source_directory(./ _SRC)

add_executable(ff_test ${_SRC})

TARGET_LINK_LIBRARIES(ff_test
	-Wl,--start-group
	libavcodec.a
	libavutil.a
	libavformat.a
	libavfilter.a
	libswresample.a
	libswscale.a
	libpostproc.a
	-Wl,--end-group
    pthread
    z
)

#add_dependencies(ff_test tlsf)

add_custom_command(TARGET ff_test  PRE_BUILD
COMMAND echo "build start"
)

add_custom_command(TARGET ff_test  POST_BUILD
COMMAND echo "build over"
)

