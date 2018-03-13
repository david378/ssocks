AUX_SOURCE_DIRECTORY(. DIR_SRCS)
include_directories(${PROJECT_SOURCE_DIR}/src)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib) 
add_library(socks STATIC ${DIR_SRCS})

if(WIN32)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
	target_link_libraries(socks ws2_32)
endif(WIN32)

target_link_libraries(socks)
INSTALL(
    DIRECTORY ${CMAKE_SOURCE_DIR}/src/libsocks
    DESTINATION ${PROJECT_SOURCE_DIR}/build/include
    FILES_MATCHING PATTERN "*.h*")
