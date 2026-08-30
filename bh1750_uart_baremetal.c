#include <stdint.h>
#include <stdio.h>
#include "pico/stdlib.h"

//在 Pico W / RP2040 上，不使用作業系統，直接以 C 操作硬體 Register，屬於 Bare-metal / Register-level programming；
//  但目前仍使用 Pico SDK 提供 stdio_init_all()、sleep_ms() 等輔助功能。

//GP4.GP5 I2C0
int main(void){
    stdio_init_all();
    sleep_ms(10000);
    printf("1. Program Start\n");

    uint32_t value;
    value = *(volatile uint32_t *)0x4000C000;
    value |= (1 << 3);
    *(volatile uint32_t *)0x4000C000 = value;
    value &= ~(1 << 3);
    *(volatile uint32_t *)0x4000C000 = value;
    while ((*(volatile uint32_t *)0x4000C008 & (1 << 3)) == 0){}

    value = *(volatile uint32_t *)0x40014024;
    value &= ~0x1F;
    value |= 0x03;
    *(volatile uint32_t *)0x40014024 = value;

    value = *(volatile uint32_t *)0x4001c014;
    value |= 0x08;
    value &= ~(1 << 2);
    *(volatile uint32_t *)0x4001C014 = value;

    value = *(volatile uint32_t *)0x4001402C;
    value &= ~0x1F;
    value |= 0x03;
    *(volatile uint32_t *)0x4001402C = value;

    value = *(volatile uint32_t *)0x4001c018;
    value |= (1 << 3);
    value &= ~(1 << 2);
    *(volatile uint32_t *)0x4001c018 = value;

    value = *(volatile uint32_t *)0x4004406C;
    value &= ~0x01;
    *(volatile uint32_t *)0x4004406C = value;

    *(volatile uint32_t *)0x40044014 = 642;
    *(volatile uint32_t *)0x40044018 = 587;

    value = *(volatile uint32_t *)0x40044004;
    value &= ~0x3FF;
    value |= 0x23;
    *(volatile uint32_t *)0x40044004 = value;

    value = *(volatile uint32_t *)0x40044000;
    value &= ~0x7F;
    value |= 0x63;
    *(volatile uint32_t *)0x40044000 = value;

    value = *(volatile uint32_t *)0x4004406C;
    value |= 0x01;
    *(volatile uint32_t *)0x4004406C = value;
    printf("2. I2C Init Done\n");

    *(volatile uint32_t *)0x40044010 = (0x200 | 0x10);
    printf("3. BH1750 Command Sent\n");

    sleep_ms(180);


    /* UART */
    //I2C0 有，但 UART0 沒有做同樣的 reset。
    // Reset UART0, bit 22
    printf("4. Before UART Reset\n");
    value = *(volatile uint32_t *)0x4000C000;
    value |= (1 << 22);
    *(volatile uint32_t *)0x4000C000 = value;
    value &= ~(1 << 22);
    *(volatile uint32_t *)0x4000C000 = value;
    printf("5. UART Reset Released\n");
    while ((*(volatile uint32_t *)0x4000C008 & (1 << 22)) == 0) {}
    printf("6. UART Reset Done\n");
    //如果現在是在學完整的 Register-level 初始化，建議 UART 區塊最前面也加入 UART0 reset。
    // GP12 = UART0 TX
    value = *(volatile uint32_t *)0x40014064; // GPIO12_CTRL
    value &= ~0x1F;
    value |= 0x02;
    *(volatile uint32_t *)0x40014064 = value;

    // GP13 = UART0 RX
    value = *(volatile uint32_t *)0x4001406C; // GPIO13_CTRL
    value &= ~0x1F;
    value |= 0x02;
    *(volatile uint32_t *)0x4001406C = value;

    // GP13 RX pull-up
    value = *(volatile uint32_t *)0x4001C038; // GPIO13 Pad
    value |= (1 << 3);
    value &= ~(1 << 2);
    *(volatile uint32_t *)0x4001C038 = value;

    value = *(volatile uint32_t *)0x40034030;
    value &= ~0x301;                         
    *(volatile uint32_t *)0x40034030 = value;

    *(volatile uint32_t *)0x40034024 = 67;
    *(volatile uint32_t *)0x40034028 = 52;

    *(volatile uint32_t *)0x4003402C = 0x70;

    value = *(volatile uint32_t *)0x40034030;
    value |= 0x301;
    *(volatile uint32_t *)0x40034030 = value;
    printf("7. UART Init Done\n");

    while(true){
        printf("8. Start I2C Read\n");

        *(volatile uint32_t *)0x40044010 = 0x100;  // READ High Byte 
        *(volatile uint32_t *)0x40044010 = 0x300;  // READ Low Byte + STOP

        printf("9. READ Commands Sent\n");
        while (*(volatile uint32_t *)0x40044078 == 0) {} 
        printf("10. High Received\n");
        uint8_t high = *(volatile uint32_t *)0x40044010 & 0xFF;

        while (*(volatile uint32_t *)0x40044078 == 0) {}
        uint8_t low = *(volatile uint32_t *)0x40044010 & 0xFF;

        uint16_t raw =((uint16_t)high << 8) | low;
        float lux = raw / 1.2f;

        printf("LUX=%.2f\n", lux);

        // UART TX
        char buffer[32];
        snprintf(buffer, sizeof(buffer),"LUX=%.2f\n", lux);
        for (int i = 0; buffer[i] != '\0'; i++)
        {
            while (*(volatile uint32_t *)0x40034018 &(1 << 5)) {}
            *(volatile uint32_t *)0x40034000 =buffer[i];
        }

        sleep_ms(500);
    }

    return 0;
}