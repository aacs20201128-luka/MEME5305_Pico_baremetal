#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"//額外去CMakeLists引用函式庫

#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5

#define BH1750_ADDR 0x23
#define BH1750_CONTINUOUS_HIGH_RES 0x10


int main()
{
    stdio_init_all();

    //// 初始化 I2C0，速度設為 100 kHz
    i2c_init(I2C_PORT , 100 * 1000);//i2c0

    //透過 Pin Mux 將 GP4、GP5 設定成 I2C 功能
    gpio_set_function(I2C_SDA , GPIO_FUNC_I2C);//4
    gpio_set_function(I2C_SCL , GPIO_FUNC_I2C);//5

    //開啟 Pico 內部上拉
    gpio_pull_up(I2C_SDA);//4
    gpio_pull_up(I2C_SCL);//5

    //要求 BH1750 進入連續高解析度模式
    uint8_t command = BH1750_CONTINUOUS_HIGH_RES;//0x10

    if(i2c_write_blocking(
        I2C_PORT,//i2c0
        BH1750_ADDR,//0x23
        &command,
        1,
        false) != 1 ){
            printf("BH1750 command failed!\n");
            return 1;
        }
    
    sleep_ms(180);//第一次量測最多需要約 180 ms

    while (true) {
        uint8_t data[2];//I²C 傳輸的基本單位就是 8-bit 的 byte

        int result = i2c_read_blocking(
            I2C_PORT,
            BH1750_ADDR,
            data,
            2,
            false
        );

        if(result == 2){
            //BH1750 的輸出就是以兩個 byte 傳回完整量測值。
            uint16_t raw = ((uint16_t)data[0] << 8 ) | data[1];//把「高 8 位」和「低 8 位」拼成完整的 16-bit 數字。
            float lux = raw / 1.2f; //亮度lux
            printf("Light: %.2f lx\n",lux);
        }else{
            printf("BH1750 read failed!\n");
        }

        sleep_ms(500);

        //printf("Hello, world!123\n");
        //sleep_ms(1000);
    }
}
