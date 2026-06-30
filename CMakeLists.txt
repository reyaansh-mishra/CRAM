cmake_minimum_required(VERSION 3.28)
set(CMAKE_C_COMPILER clang)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(cram C)


execute_process(
    COMMAND uname -r
    OUTPUT_VARIABLE KERNEL_RELEASE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(KERNEL_DIR1 "/home/reyaansh/Github/Linux")
set(KERNEL_DIR2 "/usr/lib/modules/7.1.2-3-cachyos/build")


set(CRAM_SOURCES
    src/cram_main.c
    src/cram_dev.c
	src/cram_blob.c
)

add_custom_target(cram_module ALL
	COMMAND V=1 $(MAKE) -C ${KERNEL_DIR2} M=${CMAKE_CURRENT_SOURCE_DIR}/src LLVM=1 modules
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building cram kernel module"
)

# add_custom_target(cp_compile_flags ALL
#     COMMAND /usr/lib/modules/7.0.12-1-cachyos/build/scripts/clang-tools/gen_compile_commands.py -d ${CMAKE_SOURCE_DIR}/build
#     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#     COMMENT "Copying Compile Commands"
# )

set(CMAKE_C_FLAGS "-isystem ${KERNEL_DIR}/include \
    -isystem ${KERNEL_DIR}/arch/x86/include \
    -isystem ${KERNEL_DIR}/include/generated \
    -D__KERNEL__ \
    -DMODULE \
    -nostdinc")
