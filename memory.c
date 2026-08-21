#include <stdint.h>
#include <stdio.h>
#include "pico/stdlib.h"//學習

// Register Address = Peripheral Base Address + Register Offset
// 實際位址          = Base Address            + Offset
//GPIO4_CTRL Address= IO_BANK0_BASE           + GPIO4_CTRL Offset
// IO_BANK0 Base Address = 0x40014000
// GPIO4_CTRL Offset     = 0x00000024
//                        ----------
// GPIO4_CTRL Address    = 0x40014024
// GPIO4_CTRL 位址 = 0x40014024
// FUNCSEL = bits [4:0]
// 要填的值 = 3 = 0b00011
//因為 FUNCSEL 是 5 個 bit：bits [4:0]
//所以 mask 就是：00000000 00000000 00000000 00011111 = 0x1F
//接著如果我們想把原本 bits [4:0] 清成 0，可以：value &= ~0x1F; //相同才是1，原本value的[5:31]會被保留 但[4:0]被清0
//然後第 2 步，把 3 填回去：value |= 0x03;只有低位元會跟著value做位元運算，[4:0]bit原本的0會因為0x03而被設成1

int main(void)
{
    stdio_init_all();//目前可以保留，要看到 USB Serial 輸出
    uint32_t value;

    //可以把它想成「先把 I2C0 這個硬體模組叫醒，然後確認它真的醒了」。
    //「讓 I2C0 controller 這顆硬體模組本身解除 reset，開始能工作。」
    // 應該先真的把 I2C0 reset 一次，再解除。
    // 先讓 I2C0 進入 Reset
    value = *(volatile uint32_t *)0x4000C000;
    value |= (1 << 3);
    *(volatile uint32_t *)0x4000C000 = value;
    // 再解除 I2C0 Reset
    value &= ~(1 << 3);
    *(volatile uint32_t *)0x4000C000 = value;
    // RESET_DONE offset = 0x08 
    // 等待 I2C0 bit3 = 1，代表 reset 完成
    while ((*(volatile uint32_t *)0x4000C008 & (1 << 3)) == 0){}//它是硬體回報狀態。


    //gpio4_ctrl 是一個指標，它指向一個「volatile 的 32-bit unsigned integer」。
    //volatile uint32_t *gpio4_ctrl;
    //register 剛好就是 32-bit，所以很適合用
    value = *(volatile uint32_t *)0x40014024;//將0x40014024轉換成uint32_t指標並且解引用取值
    value &= ~0x1F;
    value |= 0x03;
    *(volatile uint32_t *)0x40014024 = value;//將0x40014024解引用後改值

    //GPIO4_CTRL是在：IO_BANK0裡面。可是 pull-up / pull-down 不在 GPIO4_CTRL 裡。
    //它屬於另一個 peripheral：PADS_BANK0
    //所以現在完全套用剛才同一套公式：GPIO4 Pad Register Address = 0x4001c000 + 0x14
    value = *(volatile uint32_t *)0x4001c014;
    value |= 0x08;//0000 1000 //(1 << 3);不要只理解成「左移」，也可以直接理解成：製作 bit 3 的 mask 
    value &= ~(1 << 2);//0000 0100
    *(volatile uint32_t *)0x4001C014 = value;


    //GPIO5
    value = *(volatile uint32_t *)0x4001402C;
    value &= ~0x1F;//0xFFFFFFE0   (11111111 11111111 11111111 1110 0000)
    value |= 0x03;//0000 0011
    *(volatile uint32_t *)0x4001402C = value;

    value = *(volatile uint32_t *)0x4001c018;
    value |= (1 << 3);//0000 1000
    value &= ~(1 << 2);//1111 1011
    *(volatile uint32_t *)0x4001c018 = value;

    //從 Datasheet 找出 I2C0_BASE 是多少
    // 0x40044000 整個 I2C0 Controller 的 register 區域起點。也是I2C0 Base Address。
    // Pico W = I2C Master
    // BH1750 = I2C Slave
    // BH1750 address = 0x23
    // 速度 = 100 kHz

    value = *(volatile uint32_t *)0x4004406C;
    value &= ~0x01;//先把 bit 0 清成 0，讓 I2C0 controller 關閉。
    *(volatile uint32_t *)0x4004406C = value;

    //這兩個 register 都只能在 IC_ENABLE[0] = 0，也就是 I2C controller 關閉時修改。
    //HCNT 和 LCNT 到底要填多少，才能得到 100 kHz？
    //HCNT ≈ 幾百個 peripheral-clock cycles
    //LCNT ≈ 幾百個 peripheral-clock cycles
    //已知：
    // ic_clk = 125 MHz
    // I2C = Standard Mode
    // SCL = 100 kHz      「幫我把 I2C0 初始化成 100 kHz。」
    // 125 MHz → 0.000000008 秒 = 8 ns            1 / 125,000,000
    //      1 ns(奈秒) = 0.000000001 秒  十億分之一秒
    // 100 kHz → 0.00001 秒 = 10 µs               1 / 100,000
    //      1 µs(微秒) = 0.000001 秒  百萬分之一秒
    // I2C controller 每 8 ns 數一次，而 SCL 一圈需要約 10 µs，那它大概要數多少次才能湊出一個 SCL 週期
    // 1 µs = 1000 ns
    // 10 µs = 10,000 ns
    // 10,000 ns ÷ 8 ns = 1250
    // 如果完全不考慮額外延遲，I2C controller 大概要數 1250 個 ic_clk cycle，才能形成一個 100 kHz 的完整 SCL 週期。
    // 所以最粗略可以先想 : High + Low ≈ 1250 cycles
    // 前面找到的 Standard Mode 規格裡  
    // SCL Low  最少約 4.7 µs
    //  ic_clk 一次 = 8 ns
    //  4700 ÷ 8 = 587.5 cycles。 也就是大約： 588 個 ic_clk cycles
    // SCL High 如果暫時用這張表呈現的 5.2 µs
    //  5200 ÷ 8 = 650 cycles
    // 總共 ≈ 1238 cycles 。 1238 ≈ 1250 已經非常接近了。
    // 我們已知：
    // SCL Low Time = 4.7 µs
    // ic_clk       = 125 MHz    
    // 125 MHz → 每一個 clock = 8 ns
    // 4.7 µs = 4700 ns
    // (4.3.14.2.1章節) 的下方公式
    //      4700 = (LCNT + 1) × 8
    //      LCNT + 1 = 4700 ÷ 8 ; 等於 587.5
    //      LCNT = (587.5 - 1)  ; 等於 586.5  //但 Register 不可能寫586.5，它只能放整數。
    // 實際 cycles (586 + 1) × 8 ns = 4696 ns; 等於 4.696 µs ， 比 4.7 µs 少一點點。 所以不適合。
    // 587 + 1 = 588  × 8 ns ; 等於 4704 ns 這就滿足 4.704 µs ≥ 4.7 µs
    // 因為硬體公式裡有： LCNT + 1 。 這個 +1 很重要。 所以 LCNT register = 587 ， 588 = 實際 clock cycles

    // 前面那張表的公式:SCL High Time = (HCNT + SPKLEN + 7) × 8 ns 。 先忽略 SCL Fall Time
    // Standard Speed 那張表裡：Minimum IC_*_SPKLEN = 1
    // 5.2 µs = 5200 ns
    // 代進去：5200 = (HCNT + 1 + 7) × 8  //5200 ÷ 8 = 650
    // 所以：HCNT + 8 = 650 ; HCNT = 642
    // 因此目前我們先得到：
    // IC_SS_SCL_LCNT = 587
    // IC_SS_SCL_HCNT = 642
    // 對應地址 : HCNT → 0x40044014 ; LCNT → 0x40044018
    *(volatile uint32_t *)0x40044014 = 642;
    *(volatile uint32_t *)0x40044018 = 587;
    //先記住：這是我們依照目前「clk_sys = 125 MHz、忽略 rise/fall time」推導出的學習版數值。

    //接下來就輕鬆很多了。下一步看：
    // IC_TAR
    // Offset = 0x04
    value = *(volatile uint32_t *)0x40044004;
    //bits [9:0] = IC_TAR ; 這裡放「I2C Master 要找的目標裝置地址」。
    //因為你現在是一般 7-bit address，而且：bit 11、bit 10 都是 0 
    value &= ~0x3FF;   // 清掉 bits [9:0]
    value |= 0x23;     // BH1750 address
    *(volatile uint32_t *)0x40044004 = value;

    //IC_CON 只有在 I2C disabled 時才能寫。
    //目前官方 Pico SDK 即使是 100 kHz，也是把 controller 設成 FAST mode
    //「我們自己按照 RP2040 Datasheet 做的一套 100 kHz Standard Mode 實驗版」
    value = *(volatile uint32_t *)0x40044000;
    //read-modify-write
    value &= ~0x7F;// 先清 IC_CON bits [6:0]
    value |= 0x63;//.... 110 0011
    *(volatile uint32_t *)0x40044000 = value;

    //下一步就可以把 I2C controller 重新 enable：
    value = *(volatile uint32_t *)0x4004406C;
    value |= 0x01;//Enable  bit0 = 1
    *(volatile uint32_t *)0x4004406C = value;

    //下一步就進到真正的「送資料」： 把 BH1750 的 0x10 command 送進 I2C TX FIFO。
    // bits [7:0] = DAT
    // bit 8
    // CMD = 0 → 寫資料
    // CMD = 1 → 發出讀取要求
    // 我們現在的任務是：把 BH1750 的 0x10 寫出去，然後結束這次 I²C transaction。
    // 為甚麼是0x10請參考: BH1750FVI.PDF Continuously H-Resolution Mode 
    // RESTART  bit 10	0	    現在不需要 Restart
    // STOP     bit 9	1	    0x10 是這次傳輸最後一個 byte，送完要 STOP
    // CMD      bit 8	0	    我們現在是 Write
    // DAT      [7:0]	0x10	BH1750 command
    *(volatile uint32_t *)0x40044010 = (0x200 | 0x10);//0x210;

    //送完 0x10 後，不能馬上發 READ。需要時間產生第一次 measurement result
    sleep_ms(180);//學習階段完全可以保留 #include "pico/stdlib.h"

    while (true)
    {
        //下一階段就是 讀 BH1750 的兩個 byte。
        //i2c_read_blocking(..., data, 2, false);底層並不是單純「讀某個記憶體位址」。
        //CPU 要先透過 IC_DATA_CMD 告訴 I²C Controller：「幫我從 0x23 讀一個 byte。」
        //CMD = 1 → READ
        //*(volatile uint32_t *)0x40044010 = 0x100; //這句不是「直接取得 BH1750 資料」。去跟 0x23 要資料回傳 1 Byte
        //在第二個 command 上加 STOP=1，告訴 I2C Controller：這個 Byte 收完後結束這次 transaction。
        //讀 Low Byte，讀完就 STOP
        *(volatile uint32_t *)0x40044010 = 0x100;  // READ High Byte 
        *(volatile uint32_t *)0x40044010 = 0x300;  // READ Low Byte + STOP

        //確認 RX FIFO 裡有資料  IC_RXFLR Offset = 0x78
        // 等 High Byte
        while (*(volatile uint32_t *)0x40044078 == 0) {} //等到不是 0，就代表至少有一個 byte 進 RX FIFO 了。
        uint8_t high = *(volatile uint32_t *)0x40044010 & 0xFF; //為什麼 & 0xFF？因為真正資料在：bits [7:0]
        // 等 Low Byte
        while (*(volatile uint32_t *)0x40044078 == 0) {}
        uint8_t low = *(volatile uint32_t *)0x40044010 & 0xFF; //所以只保留最低 8 bit。
        //去讀 0x40044010 這個 register 的 32-bit 值。 接著 &0xFF 只留下 bits [7:0]，其他 bit 全部遮掉。
        //uint8_t是無符號8bit的int型別喔!


        uint16_t raw =((uint16_t)high << 8) | low;
        float lux = raw / 1.2f;

        printf("Current Light: %.2f lx\n", lux);
        sleep_ms(500);
    }
    return 0;
}