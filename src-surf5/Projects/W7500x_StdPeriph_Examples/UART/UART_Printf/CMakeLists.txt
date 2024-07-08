# Build the executable based on the source files
set(TARGET_NAME UART_Printf)

add_executable(${TARGET_NAME}

  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/UART/UART_Printf/main.c 
  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/UART/UART_Printf/W7500x_it.c
  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/UART/UART_Printf/retarget.c
  ${CMAKE_SOURCE_DIR}/Libraries/CMSIS/Device/WIZnet/W7500/Source/GCC/startup_w7500.s
  )

target_include_directories(${TARGET_NAME} PUBLIC
  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/UART/UART_Printf
  )

target_link_libraries(${TARGET_NAME} PRIVATE
    SYSTEM_W7500X_FILES  
    GPIO_FILES
    UART_FILES
    ADC_FILES
    CRG_FILES
    DUALTIMER_FILES
    EXTI_FILES
    FLASH_FILES
    MIIM_FILES
    MISC_FILES
    PWM_FILES
    RNG_FILES
    RTC_FILES
    SSP_FILES
    WDT_FILES
    WZTOE_FILES
    )

complie_link_option(${TARGET_NAME})
create_binary_file(${TARGET_NAME})
create_elf_file(${TARGET_NAME})

