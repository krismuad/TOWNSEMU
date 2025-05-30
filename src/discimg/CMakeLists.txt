add_library(discimg discimg.h discimg.cpp)
target_link_libraries(discimg cpputil)
target_include_directories(discimg PUBLIC .)

add_executable(testmsf testmsf.cpp)
target_link_libraries(testmsf discimg)
add_test(NAME DISCIMG_MSF COMMAND testmsf)

add_executable(testiso testiso.cpp)
target_link_libraries(testiso discimg)
add_test(NAME DISCIMG_ISO COMMAND testiso ${CMAKE_CURRENT_SOURCE_DIR}/../../testdata/ISOIMG/iso4sect.iso)
add_executable(testcue testcue.cpp)
target_link_libraries(testcue discimg)

add_executable(testmds testmds.cpp)
target_link_libraries(testmds discimg)

add_executable(test_cdimage test.cpp)
target_link_libraries(test_cdimage discimg)


add_executable(bincue2wav bincue2wav.cpp)
target_link_libraries(bincue2wav discimg yssimplesound cpputil)
if(MSVC)
	target_link_libraries(bincue2wav yssimplesound_dsound)
elseif(APPLE)
	find_library(COCOA_LIB Cocoa)
	mark_as_advanced(COCOA_LIB)
	target_link_libraries(bincue2wav yssimplesound_macosx ${COCOA_LIB})
elseif(UNIX)
	target_link_libraries(bincue2wav yssimplesound_linux_alsa)
endif()
