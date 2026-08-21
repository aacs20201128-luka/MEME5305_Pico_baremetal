#include <stdint.h>
#include <stdio.h>
#include "pico/stdlib.h" //學習

int main(void)
{
  stdio_init_all();
  uint32_t value;

  //UART 不像 I2C 一樣必須使用 pull-up。
  //TX 通常不需要 pull-up；RX 可視需要開 pull-up，避免輸入 floating。
  //當RX沒接對方、沒有人控制腳位時，pull-up幫它維持High，避免腳位亂飄。
  value = *(volatile uint32_t *)0x40014004; // GPIO0_CTRL
  value &= ~0x1F;
  value |= 0x02;
  *(volatile uint32_t *)0x40014004 = value;
  // value = *(volatile uint32_t *)0x4001c004; // Pad Control
  // value |= 0x08;
  // value &= ~(1 << 2);
  // *(volatile uint32_t *)0x4001c004 = value;

  value = *(volatile uint32_t *)0x4001400C; // GPIO1_CTRL
  value &= ~0x1F;
  value |= 0x02;
  *(volatile uint32_t *)0x4001400C = value;
  value = *(volatile uint32_t *)0x4001c008; // Pad Control
  value |= 0x08;
  value &= ~(1 << 2);
  *(volatile uint32_t *)0x4001C008 = value;

  ////UART0 0x40034000
  // UARTEN 就是 UART 的總開關
  value = *(volatile uint32_t *)0x40034030; // UARTCR
  value &= ~0x301;                          // 00 1111 1110 //關閉 RXE、TXE、UARTEN
  *(volatile uint32_t *)0x40034030 = value;

  /*
  | Register       | 功能                         | 存什麼 |
  | ---------------| --------------------------   |-------|
  | UARTIBRD 0x024 | Integer Baud Rate Divisor    |整數部分|
  | UARTFBRD 0x028 | Fractional Baud Rate Divisor |小數部分|
  */
  //  Integer Baud Rate Register, UARTIBRD
  //  RP2040 裡面不是只有一個 clock，而是有很多 clock。
  //    clk_peri 的 clock source 通常設定成來自 clk_sys，
  //    所以兩者通常會是同樣的頻率。
  //  那麼這個 125 MHz 就是拿來代入前面的 UARTCLK。
  //  UARTCLK = 125 MHz
  //  Required baud rate = 115200
  //  整數部分 = integer(UARTCLK / (16 × Baud Rate)) =
  //    integer(125000000 / (16 × 115200)) = 67
  //  UARTIBRD = 67;
  //  UARTFBRD = 52; // 小數部分 = integer((0.817 × 64) + 0.5) = 52
  //    6 bit 能表示的組合數就是 2^6 = 64 種
  //  C 編譯器會自動把你寫的十進制常數轉成對應的二進位後寫進 register
  *(volatile uint32_t *)0x40034024 = 67; // UARTIBRD
  *(volatile uint32_t *)0x40034028 = 52; // UARTFBRD  

  // 「UART 串列通訊線路格式控制暫存器」
  //   Line Control Register, UARTLCR_H 為什麼叫LINE
  //   因為這裡的 Line 不是「程式碼的一行」，
  //   而是指 UART 的通訊線路 / serial line。
  //   Parity：額外加一個檢查位元，用來簡單偵測資料傳輸是否出錯。常見有
  //     Odd parity、Even parity、No parity。
  //   FIFO enable：開啟 UART 的暫存佇列，讓 TX/RX 可以一次暫存多筆資
  //     料，不需要 CPU 每收到或送出 1 byte 就立刻處理。
  //   開 FIFO 時，UART 內部可以先排隊存多個 byte。CPU 不需要每收到 1
  //     byte 就立刻來讀，也不需要每送 1 byte 就立刻補資料，
  //     對高速通訊比較穩，也比較省 CPU。
  //   關 FIFO 時，UART 幾乎就是「1 byte 緩衝」。如果 CPU 沒及時處理，
  //     接收時比較容易 overflow，傳送時也需要更頻繁地餵資料。
  *(volatile uint32_t *)0x4003402C = 0x70; // 0111 0000
  // 8-N-1 + FIFO

  //UARTCR 開啟
  value = *(volatile uint32_t *)0x40034030;
  value |= 0x301;   // RXE=1, TXE=1, UARTEN=1
  *(volatile uint32_t *)0x40034030 = value;


  // UARTIMSC：所有「可以產生 UART interrupt 的事件」
  //  通常都會在 UARTIMSC 裡有對應的 mask bit。
  // UARTMIS：ISR 裡面通常看這個
  //  判斷「目前是哪個已啟用的 interrupt 真的發生了」
  // UARTICR：用來清 interrupt flag某些中斷處理完要寫這個清掉


  // Register	  重要性	    用途
  // UARTDR	  ★★★★★	  真正讀寫 UART 資料
  // UARTFR	  ★★★★★	  判斷現在可不可以讀/寫  readonly

  // 因為 TX FIFO 是「等待送出去的資料排隊區」。
  // 如果 FIFO 已經滿了，你還繼續往 UARTDR 寫資料，就可能造成資料無法正
  //  確放進去，依硬體設計可能被忽略或造成錯誤。

  // FIFO 開啟：你看的 TXFF、RXFE 是在看「整個 FIFO」滿不滿、空不空。
  // FIFO 關閉：UART 還是有基本的收發暫存機制，只是等同很淺的 buffer，
  //  所以你仍然要判斷能不能寫、能不能讀。
  // FF = Full
  // FE = Empty
  // TX：滿就等。RX：空就等，根本沒東西可以讀，所以只能等。

  //BUSY 最常在這種情況有用：你準備 關 UART、切 baud rate、改 GPIO 功
  //  能、關電源之前，要確認最後的資料真的送完。

  /*所以 UARTFR 是用來看目前 UART 的傳輸狀態；如果 CPU 丟資料太快，導致 TX FIFO 滿了，CPU 就先不要再往 UARTDR 寫資料，而是等 UART 硬體慢慢把 TX FIFO 裡的資料送出去。至於「FIFO 滿了要先停」這件事，是 UART 硬體負責把 TXFF 狀態設出來，CPU 再透過程式讀 UARTFR 來判斷要不要繼續寫資料。*/
  while (true) //0000
  {
    while(*(volatile uint32_t *)0x40034018 & (1 << 5) ){}
    *(volatile uint32_t *)0x40034000 = 'L';//剛好0~7bit
    //'A' 是一個字元常數，編譯器會把它轉成對應的數值。

    //直接把你要送的 1 byte 寫進 UARTDR，硬體會把低 8 bit，也就是 DATA
    //[7:0]，當成要送出去的資料。字元就剛好1byte了。

    while (*(volatile uint32_t *)0x40034018 & (1 << 5)) {}
    *(volatile uint32_t *)0x40034000 = 'U';
    while (*(volatile uint32_t *)0x40034018 & (1 << 5)) {}
    *(volatile uint32_t *)0x40034000 = 'X';
    while (*(volatile uint32_t *)0x40034018 & (1 << 5)) {}
    *(volatile uint32_t *)0x40034000 = '=';
    while (*(volatile uint32_t *)0x40034018 & (1 << 5)) {}
    *(volatile uint32_t *)0x40034000 = '1';
    while (*(volatile uint32_t *)0x40034018 & (1 << 5)) {}
    *(volatile uint32_t *)0x40034000 = '2';
    while (*(volatile uint32_t *)0x40034018 & (1 << 5)) {}
    *(volatile uint32_t *)0x40034000 = '3';
    while (*(volatile uint32_t *)0x40034018 & (1 << 5)) {}
    *(volatile uint32_t *)0x40034000 = 46;//'.' = 46 = 0x2E
    while (*(volatile uint32_t *)0x40034018 & (1 << 5)) {}
    *(volatile uint32_t *)0x40034000 = '6';
    while (*(volatile uint32_t *)0x40034018 & (1 << 5)) {}
    *(volatile uint32_t *)0x40034000 = '6';
    while (*(volatile uint32_t *)0x40034018 & (1 << 5)) {}
    *(volatile uint32_t *)0x40034000 = '\n';//0x0A

    printf("LUX=123.66\n");
    sleep_ms(1000);
  }
  return 0;
}