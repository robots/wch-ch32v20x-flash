cmake_minimum_required(VERSION 3.10)

project(test C CXX ASM)

#set(CMAKE_EXECUTABLE_SUFFIX_C   .elf)

set(CMAKE_C_STANDARD 11)

set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/linker.ld)

# float
#set(MACHINE -march=rv32imafc -mabi=ilp32f)
# no float
set(MACHINE -march=rv32imac -mabi=ilp32)

add_compile_options(-march=rv32imac_zicsr -mabi=ilp32 -msmall-data-limit=8 -mno-save-restore)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)
add_compile_options(-Og -g)

include_directories(
	src/
	lib/wch/
)

add_link_options(-march=rv32imac -mabi=ilp32)
add_link_options(-nostartfiles -Wl,-cref,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
add_link_options(--specs=nano.specs)
add_link_options(-T ${LINKER_SCRIPT})

add_executable(${PROJECT_NAME}
	lib/wch/core_riscv.c

	src/main.c

	src/startup_ch32v20x_D6.S

	${LINKER_SCRIPT}
)

#target_link_libraries(${PROJECT_NAME}

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} -Oihex ${PROJECT_NAME} ${PROJECT_NAME}.hex
	COMMAND ${CMAKE_OBJCOPY} -Obinary ${PROJECT_NAME} ${PROJECT_NAME}.bin
)
