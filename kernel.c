#include <stdint.h>
extern void anticompiler  ( unsigned int );

#define MMIO_BASE         0x3F000000
#define UART0_BASE        (MMIO_BASE + 0x201000)
#define GPIO_BASE         (MMIO_BASE + 0x200000)
#define GPFSEL0   ((volatile unsigned int*)(GPIO_BASE + 0x00))
#define GPFSEL1   ((volatile unsigned int*)(GPIO_BASE + 0x04))
#define GPFSEL2   ((volatile unsigned int*)(GPIO_BASE + 0x08))
#define GPFSEL3   ((volatile unsigned int*)(GPIO_BASE + 0x0C))
#define GPFSEL4   ((volatile unsigned int*)(GPIO_BASE + 0x10))
#define GPFSEL5   ((volatile unsigned int*)(GPIO_BASE + 0x14))
#define GPHEN1    ((volatile unsigned int*)(GPIO_BASE + 0x68))
#define GPPUD     ((volatile unsigned int*)(GPIO_BASE + 0x94))
#define GPPUDCLK0 ((volatile unsigned int*)(GPIO_BASE + 0x98))
#define GPPUDCLK1 ((volatile unsigned int*)(GPIO_BASE + 0x9C))*/


#define UART_DR                 ((volatile unsigned int*)(UART0_BASE + 0x00))
#define UART_FR                 ((volatile unsigned int*)(UART0_BASE + 0x18))
#define UART_IBRD               ((volatile unsigned int*)(UART0_BASE + 0x24))
#define UART_FBRD               ((volatile unsigned int*)(UART0_BASE + 0x28))
#define UART_LCRH               ((volatile unsigned int*)(UART0_BASE + 0x2C))
#define UART_CR                 ((volatile unsigned int*)(UART0_BASE + 0x30))
#define UART_ICR                ((volatile unsigned int*)(UART0_BASE + 0x44))
#define UART_IMSC               ((volatile unsigned int*)(UART0_BASE + 0x38))



void uart_init(void)
{
    
    int timeout = 1000000;
    while ((*UART_FR & (1 << 3)) && --timeout) { }
    *UART_CR = 0;


    //*UART_ICR = 0x7FF;
    *UART_LCRH = 0;
    unsigned int r;

    r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15));
    r |=  (4 << 12) | (4 << 15);  //alt0
    *GPFSEL1 = r;

    *GPPUD = 0;
    for (r = 0; r < 150; r++) anticompiler(r);
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    for (r = 0; r < 150; r++) anticompiler(r);

    *GPPUDCLK0 = 0;

    //*UART_LCRH = 0;
    *UART_ICR = 0x7FF;
    *UART_IBRD = 26;
    *UART_FBRD = 3;

    *UART_LCRH = (1 << 4) | (1 << 5) | (1 << 6);
    for (volatile int i = 0; i < 1500; i++);
    *UART_CR = (1 << 0) | (1 << 8) | (1 << 9);
}

void uart_putc(char c)    //acts like sendc
{
    while ((*UART_FR) & (1 << 5)) {anticompiler(c);} // Wait while TX FIFO full
    *UART_DR = c;

    if (c == '\b')
	{
		while ((*UART_FR) & (1 << 5)) {anticompiler(c);}

		*UART_DR = ' ';

		while ((*UART_FR) & (1 << 5)) {anticompiler(c);}

		*UART_DR = 0x08;
	}
}

void uart_puts(const char* s)
{
    while (*s)
    {
        if (*s =='\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

void uart_send(unsigned int c){
    while (*UART_FR & (1 << 5)) {
        // wait while FIFO is full
    }
    *UART_DR = c;
}



void kernel_main() {
   uart_init();
    uart_puts("Hello from kernel!\n");
        while(1) {
        anticompiler(1);
    }
}