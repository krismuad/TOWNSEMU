add_executable(disasm disasm.cpp)
target_link_libraries(disasm cpu cpputil)
add_test(NAME disasm COMMAND disasm)

add_executable(disasm_addr disasm_addr.cpp)
target_link_libraries(disasm_addr towns townssound yssimplesound yssimplesound_nownd)
add_test(NAME disasm_addr COMMAND disasm_addr)

add_executable(vram_mapping vram_mapping.cpp)
target_link_libraries(vram_mapping cpputil)
add_test(NAME vram_mapping COMMAND vram_mapping)

add_executable(wildcard wildcard.cpp)
target_link_libraries(wildcard cpputil)
add_test(NAME wildcard COMMAND wildcard)
