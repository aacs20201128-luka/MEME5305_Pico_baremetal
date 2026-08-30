#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define UART_ID     uart0
#define BAUD_RATE   115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

int main(void){
    stdio_init_all();//非常重要，因為它會初始化 SDK 的 stdio backend，包括你在 CMake 開啟的 USB stdio。 printf記得要加!

    // 透過 Pin Mux，將 GP0、GP1 切換成 UART 功能
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // 啟用 UART0，設定 Baud Rate
    uart_init(UART_ID, BAUD_RATE);

    // 不使用 CTS / RTS 硬體流量控制
    uart_set_hw_flow(UART_ID, false, false);

    // 設定為 8N1
    uart_set_format(
      UART_ID,
      8,                // 8 Data Bits
      1,                // 1 Stop Bit
      UART_PARITY_NONE  // No Parity
    );

    while(true){
        printf("LUX=123.45\n");//同時輸出到 USB stdio
        uart_puts(UART_ID,"LUX=123.45\n");
        sleep_ms(1000);
    }

    return 0;
}