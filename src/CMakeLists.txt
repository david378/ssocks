
add_subdirectory(libsocks)
include_directories(${PROJECT_SOURCE_DIR}/src)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)

if(WIN32)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
	set(winopt_src win_getopt.c win_getopt.h)
	set(nsocks_src auth-util.c configd-util.c nsocks.c config.h auth-util.h configd-util.h ${winopt_src})
	set(rcsocks_src auth-util.c configd-util.c rcsocks.c config.h auth-util.h configd-util.h ${winopt_src})
	set(rssocks_src auth-util.c configd-util.c rssocks.c config.h auth-util.h configd-util.h ${winopt_src})
	set(ssocks_src auth-util.c configd-util.c ssocks.c config.h auth-util.h configd-util.h ${winopt_src})
	set(ssocksd_src auth-util.c configd-util.c ssocksd.c config.h auth-util.h configd-util.h ${winopt_src})
	add_executable(nsocks ${nsocks_src} )
	target_link_libraries(nsocks socks ws2_32)

	add_executable(rcsocks ${rcsocks_src})
	target_link_libraries(rcsocks socks ws2_32)

	add_executable(rssocks ${rssocks_src})
	target_link_libraries(rssocks socks ws2_32)

	add_executable(ssocks ${ssocks_src})
	target_link_libraries(ssocks socks ws2_32)

	add_executable(ssocksd ${ssocksd_src})
	target_link_libraries(ssocksd socks ws2_32)
endif(WIN32)

if(UNIX)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -static -Os -s")
	set(nsocks_src auth-util.c configd-util.c nsocks.c config.h auth-util.h configd-util.h)
	set(rcsocks_src auth-util.c configd-util.c rcsocks.c config.h auth-util.h configd-util.h)
	set(rssocks_src auth-util.c configd-util.c rssocks.c config.h auth-util.h configd-util.h)
	set(ssocks_src auth-util.c configd-util.c ssocks.c config.h auth-util.h configd-util.h)
	set(ssocksd_src auth-util.c configd-util.c ssocksd.c config.h auth-util.h configd-util.h)
	add_executable(nsocks ${nsocks_src})
	target_link_libraries(nsocks socks)

	add_executable(rcsocks ${rcsocks_src})
	target_link_libraries(rcsocks socks)

	add_executable(rssocks ${rssocks_src})
	target_link_libraries(rssocks socks)

	add_executable(ssocks ${ssocks_src})
	target_link_libraries(ssocks socks)

	add_executable(ssocksd ${ssocksd_src})
	target_link_libraries(ssocksd socks)
endif(UNIX)