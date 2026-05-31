#include <stdint.h>
#include "mbox.h"
extern void anticompiler  ( unsigned int );
extern void drop_to_el1(void);
extern unsigned long __kernel;
extern unsigned char _end;

#define MMIO_BASE         0x3F000000
#define UART0_BASE        (MMIO_BASE + 0x201000)
#define GPIO_BASE         (MMIO_BASE + 0x200000)
#define SDHOST_BASE       (MMIO_BASE + 0x202000)
#define EMMC_BASE         (MMIO_BASE + 0x300000)

// command flags
#define CMD_NEED_APP        0x80000000
#define CMD_RSPNS_48        0x00020000
#define CMD_ERRORS_MASK     0xfff9c004
#define CMD_RCA_MASK        0xffff0000

#define SD_OK                0
#define SD_TIMEOUT          -1
#define SD_ERROR            -2


// INTERRUPT register settings
#define INT_DATA_TIMEOUT    0x00100000
#define INT_CMD_TIMEOUT     0x00010000
#define INT_READ_RDY        0x00000020
#define INT_CMD_DONE        0x00000001

#define INT_ERROR_MASK      0x017E8000

// COMMANDs
#define CMD_GO_IDLE         0x00000000
#define CMD_ALL_SEND_CID    0x02010000
#define CMD_SEND_REL_ADDR   0x03020000
#define CMD_CARD_SELECT     0x07030000
#define CMD_SEND_IF_COND    0x08020000
#define CMD_STOP_TRANS      0x0C030000
#define CMD_READ_SINGLE     0x11220010
#define CMD_READ_MULTI      0x12220032
#define CMD_SET_BLOCKCNT    0x17020000
#define CMD_APP_CMD         0x37000000
#define CMD_SET_BUS_WIDTH   (0x06020000|CMD_NEED_APP)
#define CMD_SEND_OP_COND    (0x29020000|CMD_NEED_APP)
#define CMD_SEND_SCR        (0x33220010|CMD_NEED_APP)

#define GPFSEL0   ((volatile unsigned int*)(GPIO_BASE + 0x00))
#define GPFSEL1   ((volatile unsigned int*)(GPIO_BASE + 0x04))
#define GPFSEL2   ((volatile unsigned int*)(GPIO_BASE + 0x08))
#define GPFSEL3   ((volatile unsigned int*)(GPIO_BASE + 0x0C))
#define GPFSEL4   ((volatile unsigned int*)(GPIO_BASE + 0x10))
#define GPFSEL5   ((volatile unsigned int*)(GPIO_BASE + 0x14))
#define GPHEN1    ((volatile unsigned int*)(GPIO_BASE + 0x68))
#define GPPUD     ((volatile unsigned int*)(GPIO_BASE + 0x94))
#define GPPUDCLK0 ((volatile unsigned int*)(GPIO_BASE + 0x98))
#define GPPUDCLK1 ((volatile unsigned int*)(GPIO_BASE + 0x9C))


#define UART_DR                 ((volatile unsigned int*)(UART0_BASE + 0x00))
#define UART_FR                 ((volatile unsigned int*)(UART0_BASE + 0x18))
#define UART_IBRD               ((volatile unsigned int*)(UART0_BASE + 0x24))
#define UART_FBRD               ((volatile unsigned int*)(UART0_BASE + 0x28))
#define UART_LCRH               ((volatile unsigned int*)(UART0_BASE + 0x2C))
#define UART_CR                 ((volatile unsigned int*)(UART0_BASE + 0x30))
#define UART_ICR                ((volatile unsigned int*)(UART0_BASE + 0x44))
#define UART_IMSC               ((volatile unsigned int*)(UART0_BASE + 0x38))


#define EMMC_ARG2               (*(volatile uint32_t*)(EMMC_BASE + 0x0))
#define EMMC_RESP0              (*(volatile uint32_t*)(EMMC_BASE + 0x10)) 
#define EMMC_CMDTM              (*(volatile uint32_t*)(EMMC_BASE + 0xc))
#define EMMC_BLKSIZECNT         (*(volatile uint32_t*)(EMMC_BASE + 0x4))
#define EMMC_ARG1               (*(volatile uint32_t*)(EMMC_BASE + 0x8))
#define EMMC_DATA               (*(volatile uint32_t*)(EMMC_BASE + 0x20))
#define EMMC_STATUS             (*(volatile uint32_t*)(EMMC_BASE + 0x24))
#define EMMC_CONTROL0           (*(volatile uint32_t*)(EMMC_BASE + 0x28))
#define EMMC_CONTROL1           (*(volatile uint32_t*)(EMMC_BASE + 0x2c))
#define EMMC_INTERRUPT          (*(volatile uint32_t*)(EMMC_BASE + 0x30))
#define EMMC_IRPT_MASK          (*(volatile uint32_t*)(EMMC_BASE + 0x34))
#define EMMC_IRPT_EN            (*(volatile uint32_t*)(EMMC_BASE + 0x38))
#define EMMC_SLOTISR_VER        (*(volatile uint32_t*)(EMMC_BASE + 0xfc))
#define C1_SRST_HC          0x01000000
#define PAYLOAD_LBA 2048
#define PAYLOAD_SIZE 512

#define HOST_SPEC_NUM       0x00ff0000
#define HOST_SPEC_NUM_SHIFT 16
#define HOST_SPEC_V3        2
#define HOST_SPEC_V2        1
#define HOST_SPEC_V1        0

#define SCR_SD_BUS_WIDTH_4  0x00000400
#define SCR_SUPP_SET_BLKCNT 0x02000000
// added by my driver
#define SCR_SUPP_CCS        0x00000001

#define SR_READ_AVAILABLE   0x00000800
#define SR_DAT_INHIBIT      0x00000002
#define SR_CMD_INHIBIT      0x00000001
#define SR_APP_CMD          0x00000020

#define ACMD41_VOLTAGE      0x00ff8000
#define ACMD41_CMD_COMPLETE 0x80000000
#define ACMD41_CMD_CCS      0x40000000
#define ACMD41_ARG_HC       0x51ff8000

unsigned long sd_scr[2], sd_ocr, sd_rca, sd_err, sd_hv;

void uart_init(void)
{
    
    int timeout = 1000000;
    while ((*UART_FR & (1 << 3)) && --timeout) { }
    *UART_CR = 0;

    mbox[0] = 9*4;
    mbox[1] = MBOX_REQUEST;
    mbox[2] = MBOX_TAG_SETCLKRATE; // set clock rate
    mbox[3] = 12;
    mbox[4] = 8;
    mbox[5] = 2;           // UART clock
    mbox[6] = 48000000;     // 48Mhz
    mbox[7] = 0;           // clear turbo
    mbox[8] = MBOX_TAG_LAST;
    mbox_call(MBOX_CH_PROP);


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

char uart_getc() {
    char r;
    /* wait until something is in the buffer */
    do{asm volatile("nop");}while(*UART_FR&0x10);
    /* read it and return */
    r=(char)(*UART_DR);
    /* convert carrige return to newline */
    return r=='\r'?'\n':r;
}
void uart_hex(uint32_t value){
    const char* hex_digits = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--)
    {
        char hex_char = hex_digits[(value >> (i * 4)) & 0xF];
        uart_putc(hex_char);
    }
    uart_putc('\n');
}

void uart_bin(uint32_t v){
    for (int i = 31; i >= 0; i--)
    {
        uart_putc((v & (1 << i)) ? '1' : '0');
    }
    uart_putc('\n');
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

void wait_msec(unsigned int n)
{
    register unsigned long f, t, r;
    // get the current counter frequency
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(f));
    // read the current counter
    asm volatile ("mrs %0, cntpct_el0" : "=r"(t));
    // calculate required count increase
    unsigned long i=((f/1000)*n)/1000;
    // loop while counter increase is less than i
    do{asm volatile ("mrs %0, cntpct_el0" : "=r"(r));}while(r-t<i);
}

void uart_dump(void *ptr)
{
    unsigned long a,b,d;
    unsigned char c;
    for(a=(unsigned long)ptr;a<(unsigned long)ptr+512;a+=16) {
        uart_hex(a); uart_puts(": ");
        for(b=0;b<16;b++) {
            c=*((unsigned char*)(a+b));
            d=(unsigned int)c;d>>=4;d&=0xF;d+=d>9?0x37:0x30;uart_send(d);
            d=(unsigned int)c;d&=0xF;d+=d>9?0x37:0x30;uart_send(d);
            uart_send(' ');
            if(b%4==3)
                uart_send(' ');
        }
        for(b=0;b<16;b++) {
            c=*((unsigned char*)(a+b));
            uart_send(c<32||c>=127?'.':c);
        }
        uart_send('\r');
        uart_send('\n');
    }
}

int sd_status(unsigned int mask)
{
    int cnt = 500000; while((EMMC_STATUS & mask) && !(EMMC_INTERRUPT & INT_ERROR_MASK) && cnt--) wait_msec(1);
    return (cnt <= 0 || (EMMC_INTERRUPT & INT_ERROR_MASK)) ? SD_ERROR : SD_OK;
}


int sd_set_clock(uint32_t base_clock, uint32_t desired_clock) {
    long r,cnt,ccs=0;
    r=*GPFSEL4; r&=~(7<<(7*3)); 
    *GPFSEL4=r;
    *GPPUD=2; 
    for (r = 0; r < 150; r++) anticompiler(r); 
    *GPPUDCLK1=(1<<15); 
    for (r = 0; r < 150; r++) anticompiler(r); 
    *GPPUD=0; 
    *GPPUDCLK1=0;
    r=*GPHEN1; 
    r|=1<<15; 
    *GPHEN1=r;

    // GPIO_CLK, GPIO_CMD
    r=*GPFSEL4; r|=(7<<(8*3))|(7<<(9*3)); 
    *GPFSEL4=r;
    *GPPUD=2; for (r = 0; r < 150; r++) anticompiler(r);
    *GPPUDCLK1=(1<<16)|(1<<17);
    for (r = 0; r < 150; r++) anticompiler(r); 
    *GPPUD=0; 
    *GPPUDCLK1=0;

    // GPIO_DAT0, GPIO_DAT1, GPIO_DAT2, GPIO_DAT3
    r=*GPFSEL5; r|=(7<<(0*3)) | (7<<(1*3)) | (7<<(2*3)) | (7<<(3*3)); 
    *GPFSEL5=r;
    *GPPUD=2; 
    for (r = 0; r < 150; r++) anticompiler(r);
    *GPPUDCLK1=(1<<18) | (1<<19) | (1<<20) | (1<<21);
    for (r = 0; r < 150; r++) anticompiler(r); 
    *GPPUD=0; 
    *GPPUDCLK1=0;

    sd_hv = (EMMC_SLOTISR_VER & HOST_SPEC_NUM) >> HOST_SPEC_NUM_SHIFT;
    uart_puts("\n\nEMMC: GPIO set up\n");
    //disable SD clock before configuring it, by clearing the 2nd bit (1<<2) in control0 register
    //EMMC_CONTROL0 = 0; // reset control0 register to default state, which also disables SD clock
    
    //EMMC_CONTROL1 &= ~(1 << 2); // disable SD clock
    //EMMC_CONTROL1 &= ~(1 << 24); //Reset the complete host circuit

    EMMC_CONTROL0 = 0; EMMC_CONTROL1 |= (1 << 24); // reset host controller
    cnt=10000; do{wait_msec(10);} while( (EMMC_CONTROL1 & (1 << 24)) && cnt-- );
    if(cnt<=0) {
        uart_puts("ERROR: failed to reset EMMC\n");
        return -1;
    }
    uart_puts("EMMC: reset OK\n");

    while (EMMC_STATUS & (1 << 0));  // CMD inhibit
    while (EMMC_STATUS & (1 << 1));  // DAT inhibit

        //CLK_INTLEN enable: 0th bit (1<<0) in control0 register, which enables the internal clock
    //CLK_EN SD clock enable: 2nd bit (1<<2) in control0 register, which enables the SD clock
    EMMC_CONTROL1 |= (1 << 0) | (1 << 19) | (1 << 18) | (1 << 17) | (0 << 16); // enable internal clock and set frequency to 400 KHz (for 48 MHz base clock)
    //1111 (15 / 0xF) => timeout disabled
    //1110 (14 / 0XE) => largest timeout, which is 15.36 seconds for 48 MHz base clock, which should be sufficient for initialization
    while (!(EMMC_CONTROL1 & (1 << 1))); // wait for internal clock to stabilize, signified by 1st bit in control1 register
    wait_msec(10);

    cnt = 100000;
    while((EMMC_STATUS & ((1<<0)|(1<<1))) && cnt--) wait_msec(1);
    if(cnt<=0) {
        uart_puts("ERROR: timeout waiting for inhibit flag\n");
        return -1;
    }
    EMMC_CONTROL1 &= ~(1 << 2); //turn off internal clock before changing the divider, as per ARASAN SD controller specification, which states that the divider can only be changed when the internal clock is off
    //internal clock setup for base emmc around 50 mHz - 100 mHz, :
    //ARASAN SD base clock is 250Hz, but we can only set it to 48 MHz in QEMU, so we will use that as our base clock for calculations
    //desired clock is 400 KHz for initialization, so we need to calculate the divider value using the formula:
    uint32_t divider = (base_clock + (2 * desired_clock) - 1) / (2 * desired_clock);
    //clamp the divider to max of 0x3FF (10 bits) and min of 2, as per the ARASAN SD controller specification, which states that the divider must be between 2 and 1023 (0x3FF)
    if (divider < 2)
    divider = 2;
    if (divider > 0x3FF)
    divider = 0x3FF;
     
    EMMC_CONTROL1 &= ~((0xFF << 8) | (0x3 << 6));
    EMMC_CONTROL1 |= ((divider & 0xFF) << 8);
    EMMC_CONTROL1 |= (((divider >> 8) & 0x3) << 6);

    uart_puts("SD card clock Information: \n");
    uart_puts("dividor(d): "); uart_bin(divider);uart_puts("\n");
    EMMC_CONTROL1 |= (1 << 2); // enable SD clock, which is the 2nd bit (1<<2) in control0 register
    uart_puts("EMMC_CONTROL1: ");uart_hex(EMMC_CONTROL1);uart_puts("\n");
    uart_puts("EMMC_STATUS: ");uart_hex(EMMC_STATUS);uart_puts("\n");
    wait_msec(10);
    cnt=10000; while(!(EMMC_CONTROL1 & (1 << 2))&& cnt--) wait_msec(10);
    if(cnt<=0) {
        uart_puts("ERROR: failed to get stable clock\n");
        return -1;
    }
    //uart_send(0x20);
    uart_puts("EMMC: SD clock set to desired frequency\n");
    EMMC_IRPT_MASK = 0xFFFFFFFF; // enable all interrupts
    EMMC_IRPT_EN = 0xFFFFFFFF; // unmask all interrupts
    //no nneed to stabilize, while (!(EMMC_CONTROL1 & (1 << 2))); // wait for SD clock to stabilize, signified by 2nd bit in control1 register
    
    /*
    sd_cmd(CMD_CARD_SELECT,sd_rca);
    if(sd_err) return sd_err;

    if(sd_status(SR_DAT_INHIBIT)) return SD_TIMEOUT;
    *EMMC_BLKSIZECNT = (1<<16) | 8;
    sd_cmd(CMD_SEND_SCR,0);
    if(sd_err) return sd_err;
    if(sd_int(INT_READ_RDY)) return SD_TIMEOUT;

    r=0; cnt=100000; while(r<2 && cnt) {
        if( *EMMC_STATUS & SR_READ_AVAILABLE )
            sd_scr[r++] = *EMMC_DATA;
        else
            wait_msec(1);
    }
    if(r!=2) return SD_TIMEOUT;
    if(sd_scr[0] & SCR_SD_BUS_WIDTH_4) {
        sd_cmd(CMD_SET_BUS_WIDTH,sd_rca|2);
        if(sd_err) return sd_err;
        *EMMC_CONTROL0 |= C0_HCTL_DWITDH;
    }
    // add software flag
    uart_puts("EMMC: supports ");
    if(sd_scr[0] & SCR_SUPP_SET_BLKCNT)
        uart_puts("SET_BLKCNT ");
    if(ccs)
        uart_puts("CCS ");
    uart_puts("\n");
    sd_scr[0]&=~SCR_SUPP_CCS;
    sd_scr[0]|=ccs;
    return SD_OK;
    */
}
int sd_int(unsigned int mask)
{
    unsigned int r, m=mask | INT_ERROR_MASK;
    int cnt = 1000000; while(!(EMMC_INTERRUPT & m) && cnt--) wait_msec(1);
    r=EMMC_INTERRUPT;
    if(cnt<=0 || (r & INT_CMD_TIMEOUT) || (r & INT_DATA_TIMEOUT) ) { EMMC_INTERRUPT=r; return SD_TIMEOUT; } else
    if(r & INT_ERROR_MASK) { EMMC_INTERRUPT=r; return SD_ERROR; }
    EMMC_INTERRUPT=mask;
    return 0;
}

void send_data(){
    while (!(EMMC_INTERRUPT & (1 << 5)) && !(EMMC_INTERRUPT & (1 << 8)));   // data done & read ready
    
    for (int i = 0; i < 128; i++) { // 512 bytes / 4
        while (!(EMMC_STATUS & (1 << 11))); // read ready
    uint32_t data = EMMC_DATA;
    uart_hex(data); // or store it
}
    while (!(EMMC_INTERRUPT & (1 << 1))); // data done
    EMMC_INTERRUPT = 0xFFFFFFFF;

}
int sd_send_command() {
    uint32_t arg;
    uint32_t cmd_index;
    //unsigned char *buffer=&_end;
    uint8_t *buffer = (uint8_t*)0x200000;
    EMMC_INTERRUPT = 0xFFFFFFFF;
    EMMC_IRPT_EN = 0xFFFFFFFF;
    EMMC_IRPT_MASK = 0xFFFFFFFF;
    //uint32_t blocks = (PAYLOAD_SIZE + 511) / 512;
    //EMMC_BLKSIZECNT = (1 << 16) | 512;
    EMMC_ARG1 = 0; // Set argument for the command, correct
    EMMC_ARG2=0; //This register contains the argument for the SD card specific command ACMD23(SET_WR_BLK_ERASE_COUNT). ARG2 must be set before the ACMD23 commandis issued using the CMDTM register.
    //cmd_ind = 18  //read muiltple
    //TM_MULTI_BLOCK = 1 //muiltple
    //cmd_index = 17; //read one
    //TM_MULTI_BLOCK = 0; //read single 
    //need to set BLKSIZECNT for data transfer commands,

    while (EMMC_STATUS & (1 << 0)); // wait CMD_INHIBIT clear
    while (EMMC_STATUS & (1 << 1)); // wait DATA_INHIBIT clear

    //CMD0 check
    EMMC_CMDTM = (0 << 24);
    uart_puts("CMDTM0: ");uart_bin(EMMC_CMDTM);

    EMMC_ARG1 = 0x1AA;
    //CMD8 check

    uint32_t cmdtm8 =     //OCR for SDHC or SDXC cards, which support block addressing, so the argument is 0 for CMD58
    (8 << 24) |   // CMD8
    (1 << 19) |    // CRC check
    (1 << 20) |    // index check
    (2 << 16);     // 48-bit response

    EMMC_CMDTM = cmdtm8;
    sd_int(INT_CMD_DONE); // wait for command complete;
    uart_puts("CMDTM8: ");uart_bin(EMMC_CMDTM); uart_puts("\n");
    volatile uint32_t r = EMMC_RESP0;
    uart_puts("Response: ");uart_bin(r);uart_puts("\n");
    uart_puts("INTERRUPT: ");uart_hex(EMMC_INTERRUPT);uart_puts("\n");
    
    EMMC_INTERRUPT = 0xFFFFFFFF;

    
    EMMC_ARG1 = 512;

    int c=0,d, lba=0, num=1;
    if(num<1) num=1;
    uart_puts("sd_readblock lba ");uart_hex(lba);uart_puts(" num ");uart_hex(num);uart_puts("\n");
    while (EMMC_STATUS & (1 << 0)); // wait CMD_INHIBIT clear
    while (EMMC_STATUS & (1 << 1)); // wait DATA_INHIBIT clear

    
    EMMC_INTERRUPT = 0xFFFFFFFF;

    r = 0;
    int cnt = 1000000;
volatile uint32_t ccs=0;

uint32_t cmdtm55 =
    (55 << 24) |
    (0 << 22) |   // CMD_TYPE = normal
    (0 << 21) |   // NOT data
    (1 << 20) |   // index check
    (1 << 19) |   // CRC check
    (2 << 16);    // R1

    uint32_t acmd41 =
    (41 << 24) |
    (0 << 22) |   // CMD_TYPE = normal
    (0 << 21) |   // NOT data
    (1 << 20) |   // index check
    (1 << 19) |   // CRC check
    (2 << 16);    // R1


do {
    EMMC_INTERRUPT = 0xFFFFFFFF;

EMMC_ARG1 = 0;
    EMMC_CMDTM = cmdtm55;
    
    for (volatile int i = 0; i < 1000; i++){anticompiler(0);} // small delay
if (sd_int(INT_CMD_DONE) == 0) {
    uart_puts("CMD55 successful\n");
} else {
    uart_puts("CMD55 failed\n");
}
    EMMC_INTERRUPT = 0xFFFFFFFF;


    EMMC_ARG1 = 0x40300000; // HCS + voltage window
    EMMC_CMDTM = acmd41;
    for (volatile int i = 0; i < 1000; i++) anticompiler(0); // small delay
    if(sd_int(INT_CMD_DONE)){
        uart_puts("CMD55 succesful\n");}
        else
        {
            uart_puts("CMD55 failed\n");
            uart_puts("INTERRUPT: ");uart_hex(EMMC_INTERRUPT);uart_puts("\n");
            return -1;
        }


    r = EMMC_RESP0;
    uart_puts("ACMD41 resp: ");uart_hex(r);uart_puts("\n");
    cnt--;

} while (!(r & (1 << 31)) && --cnt);

    //while (EMMC_STATUS & (1 << 0)); // wait CMD_INHIBIT clear
   // while (EMMC_STATUS & (1 << 1)); // wait DATA_INHIBIT clear

EMMC_ARG1= 0x00000000;
/*
   uint32_t cmdtm58 =     //OCR for SDHC or SDXC cards, which support block addressing, so the argument is 0 for CMD58
    (58 << 24) |   // CMD58
    (1 << 19) |    // CRC check
    (1 << 20) |    // index check
    (2 << 16);     // 48-bit response

    EMMC_CMDTM = cmdtm58;
    sd_int(INT_CMD_DONE);
    uart_puts("CMDTM58: ");uart_bin(EMMC_CMDTM);uart_puts("\n");
    uint32_t ocr = EMMC_RESP0;
    uart_puts("OCR: ");uart_bin(ocr);uart_puts("\n");
    ccs = (ocr >> 30) & 1;
    uart_puts("Card Capacity Status (CCS): ");uart_bin(ccs);uart_puts("\n"); */

    EMMC_INTERRUPT = 0xFFFFFFFF;


    while (EMMC_STATUS & (1 << 1)); // wait DATA_INHIBIT clear
    uart_puts("!data line isnt busy!\n");
    
    while (EMMC_STATUS & (1 << 0)); // wait CMD_INHIBIT clear
    uart_puts("!command line isnt busy!\n");

EMMC_ARG1 = 0;
    

uint32_t cmdtm2 = 
(2 << 24) | 
(0 << 22) | 
(0 << 21) | 
(1 << 19) | 
(1 << 20) | 
(1 << 16); // 136-bit response
EMMC_CMDTM = cmdtm2;
sd_int(INT_CMD_DONE);
uart_puts("CMDTM2: ");uart_bin(EMMC_CMDTM);uart_puts("\n");


    EMMC_INTERRUPT = 0xFFFFFFFF;
    while (EMMC_STATUS & (1 << 1)); // wait DATA_INHIBIT clear
    uart_puts("!data line isnt busy!\n");
    
    while (EMMC_STATUS & (1 << 0)); // wait CMD_INHIBIT clear
    uart_puts("!command line isnt busy!\n");

EMMC_ARG1 = 0;

uint32_t cmdtm3 =
    (3 << 24) |
    (0 << 22) |
    (0 << 21) |
    (1 << 19) |
    (1 << 20) |
    (2 << 16);   // R1
EMMC_CMDTM = cmdtm3;
uart_puts("CMDTM3: ");uart_bin(EMMC_CMDTM);uart_puts("\n");
sd_int(INT_CMD_DONE);


    EMMC_INTERRUPT = 0xFFFFFFFF;
    while (EMMC_STATUS & (1 << 1)); // wait DATA_INHIBIT clear
    uart_puts("!data line isnt busy!\n");
    
    while (EMMC_STATUS & (1 << 0)); // wait CMD_INHIBIT clear
    uart_puts("!command line isnt busy!\n");

uint32_t rca = EMMC_RESP0 & 0xFFFF0000;
EMMC_ARG1 = rca;

uart_puts("RCA: ");uart_hex(rca);uart_puts("\n");




uint32_t cmdtm7 =
    (7 << 24) |
    (0 << 22) |
    (0 << 21) |
    (1 << 19) |
    (1 << 20) |
    (3 << 16);   // R1b (busy)
EMMC_CMDTM = cmdtm7;
sd_int(INT_CMD_DONE);
uart_puts("CMDTM7: ");uart_bin(EMMC_CMDTM);uart_puts("\n");

while (EMMC_STATUS & (1 << 1));
uart_puts("STATE after CMD7: ");uart_hex((EMMC_RESP0 >> 9) & 0xF);uart_puts("\n");


    EMMC_INTERRUPT = 0xFFFFFFFF;
    lba = 4114;

    EMMC_ARG1 = lba;

    while (EMMC_STATUS & (1 << 1)); // wait DATA_INHIBIT clear
    uart_puts("!data line isnt busy!\n");
    
    while (EMMC_STATUS & (1 << 0)); // wait CMD_INHIBIT clear
    uart_puts("!command line isnt busy!\n");
 
    EMMC_BLKSIZECNT = (3 << 16) | 512;  //and comment this no need for single block read.

    uint32_t cmdtm =
    //(cmd_index << 24) |   // CMD index
    //(0 << 16) |   // no data
    //(0 << 19) |   // CRC not checked
    //(0 << 20);    // no response//proper way
    //(17 << 24) |   // CMD17
    (18 << 24)|  // comment this out for single block read 
    (1 << 21) |   // data present
    (1 << 4)  |   // read (data from card)
    (1 << 5)  |   // multi-block
    (1 << 19) |// CRC check
    (1 << 20) |// index check
    (1 << 2)  |  // comment this out for single block read
    (2 << 16) |// 48-bit response 
    (1 << 1);   //comment this too
    EMMC_CMDTM = cmdtm;

uint32_t timeout = 1000000;
uart_puts("CMDTM: ");uart_bin(EMMC_CMDTM);uart_puts("\n"); 
sd_int(INT_CMD_DONE);


uint32_t state = (EMMC_RESP0 >> 9) & 0xF;
uart_puts("STATE: ");uart_hex(state);uart_puts("\n");
r = EMMC_RESP0;uart_puts("Response: ");uart_bin(r);uart_puts("\n");
uart_puts("INTERRUPT: ");uart_hex(EMMC_INTERRUPT);uart_puts("\n");
//while (EMMC_STATUS & (1 << 0)); // wait CMD_INHIBIT clear
//while (EMMC_STATUS & (1 << 1)); // wait DATA_INHIBIT clear

uart_puts("=================================DEBUG=================================\n");
uart_puts("EMMC_INTERRUPT: ");uart_hex(EMMC_INTERRUPT);uart_puts("\n");
uart_puts("EMMC_STATUS: ");uart_hex(EMMC_STATUS);uart_puts("\n");
uart_puts("INT_READ_RDY: ");uart_hex(INT_READ_RDY);uart_puts("\n");

uart_puts("EMMC_CONTROL0: ");uart_hex(EMMC_CONTROL0);uart_puts("\n");


//while (!(EMMC_INTERRUPT & INT_READ_RDY));
while (!(EMMC_INTERRUPT  & (1 << 5))) wait_msec(10000);
uart_puts("Data is ready to be read\n");
/*
if (r != 0) {
    uart_puts("ERROR: Timeout waiting for ready to read\n");
    uart_hex(r);
    return 0;
}*/
uart_puts("Reading data...\n");
int b; 
    for(b = 0; b < 3; b++) {
        for(d=0;d<128;d++){ 
        while (!(EMMC_STATUS & (1 << 11))); // wait FIFO ready
        ((uint32_t*)buffer)[d] = EMMC_DATA;
        }
        c++; buffer+=128;
    }
    //EMMC_CMDTM = (1 << 4); //from card to host 
    while (!(EMMC_INTERRUPT & (1 << 1))); // DATA_DONE usually

//while(EMMC_STATUS & (1 << 9));  //can i read new data?
//uart_dump(&_end);
uint8_t *address = (uint8_t*)0x200000;
uart_dump(address);
/*
while (1) {
    uint32_t irpt = EMMC_INTERRUPT;

    if (irpt & 0xFFFF0000) {
        uart_puts("Error occurred\n");
        uart_bin(irpt);
        EMMC_INTERRUPT = irpt;
        break;
    }

    if (irpt & 1) {  // Command complete
        uart_puts("Command complete\n");
        EMMC_INTERRUPT = irpt;
        while(EMMC_STATUS & (1 << 1)){
        
        uart_puts("Interrupt: "); uart_bin(irpt);uart_puts("\n");
        uart_puts("CMDTM: "); uart_bin(EMMC_CMDTM);uart_puts("\n");
        // wait for data ready
        while (!(EMMC_INTERRUPT & (1 << 8)));  // 
        while(!(EMMC_INTERRUPT & (1 << 5)));  // READ_RDY
        uart_puts("Sending data...\n");






        send_data();
        // wait transfer complete
        while (!(EMMC_INTERRUPT & (1 << 1)));  // DATA_DONE
        }
        uart_puts("Command Completed but with errors\n");
        break;
    }


    if (--timeout == 0) {
        uart_puts("CMD TIMEOUT\n");
        uart_bin(irpt);
        uart_bin(EMMC_CMDTM);
        break;
    }
}

/*
 uart_bin(EMMC_INTERRUPT);
   while (!(EMMC_INTERRUPT & 1)) {
    uart_puts("yo whtas up man\n");
    if (EMMC_INTERRUPT & 0xFFFF0000) {
        uart_puts("Error occured\n");
        uart_bin(EMMC_INTERRUPT);
        break;
    }
    if((EMMC_INTERRUPT & 0xFFFF0000) == 0) {
        uart_puts("Data Transfer complete\n");
        uart_bin(EMMC_INTERRUPT);
        uart_bin(EMMC_CMDTM);
        break;
    }
    if (--timeout == 0) {
        uart_puts("CMD TIMEOUT\n");
        uart_bin(EMMC_INTERRUPT);
        //uart_bin((EMMC_INTERRUPT >> 17) & 0x1);
        uart_bin(EMMC_CMDTM);
        break;
    }
} */
}


static void invalidate_icache(void)
{
    asm volatile(
        "dsb sy\n"
        "ic iallu\n"
        "dsb sy\n"
        "isb\n"
        :
        :
        : "memory"
    );

   // asm volatile(
   //     "ldr x0, =__kernel\n"
   //     "br x0\n"
   //     :
   //     :
   //     : "x0"
   // );
}

/*
static inline void drop_el(void){

    asm volatile(
        ".global el1_entry\n"

        "mrs x0, scr_el3\n"
        "orr x0, x0, #(1 << 10)\n"
        "orr x0, x0, #(1 << 0)\n"
        "msr scr_el3, x0\n"

        "mov x0, #0x3c5\n"
        "msr spsr_el3, x0\n"

        "ldr x0, =el1_entry\n"
        "msr elr_el3, x0\n"

        "eret\n"

        "el1_entry:\n"

        "ldr x0, =0x400000\n"
        "mov sp, x0\n"

        "dsb sy\n"
        "isb\n"
        "ic iallu\n"
        "dsb sy\n"
        "isb\n"

       // "ldr x0, =0x200000\n"
       // "br x0\n"
    );

}
*/

void jump_to_kernel(void)
{
    uart_puts("Jumping to kernel...\n");

    void (*kernel_entry)(void) = (void (*)(void))__kernel;

    unsigned long el;
    asm volatile("mrs %0, CurrentEL" : "=r"(el));
    uart_puts("EXCEPTION LEVEL: ");uart_hex(el);uart_puts("\n");
    unsigned long IA;

    asm volatile("adr %0, .": "=r"(IA));

    uart_puts("Instruction Address: ");uart_hex(IA);uart_puts("\n");

    invalidate_icache();

    uart_puts("Cache invalidated.\n");

    //drop_el();

    drop_to_el1();
    //drop_to_el1();

    //kernel();

    while (1)
    {
    }
}


void main(void)
{
    uart_init();

    for (volatile int i = 0; i < 50000; i++){anticompiler(0);} // Delay to ensure UART is ready;

    //uart_putc('s');
    //uart_putc('y');
    //uart_putc('s');
    //uart_putc(' ');
    //uart_putc('i');
    //uart_putc('n');
    //uart_putc('i');
    //uart_putc('t');
    //uart_puts("Hello, World from Bootloader!\n");

    if(sd_set_clock( 100000000, 400000))
    {
        uart_puts("EMMC: Support OK\n");
    }

    sd_send_command(); 
        //while(1) {
        //uart_send(uart_getc());
    //}

//uart_puts("for some apparent i dont trust this so if this address really from RAM lets check 0x80000\n");
//uart_dump((void*)0x80000);
//uart_puts("\n");
//uart_dump((void*)0x80200);
//uart_puts("\n");

    typedef void (*kernel_entry_t)(void);
    //uart_dump((void*)0x200474);
    jump_to_kernel();
    //kernel_entry_t k = (kernel_entry_t)0x200000;
/*
asm volatile(
    "mov x0, #'J'\n"
    "ldr x1, =0x3F201000\n"
    "str w0, [x1]\n"

    "msr daifset, #0xf\n"

    "dsb sy\n"
    "isb\n"

    "ic iallu\n"

    "dsb sy\n"
    "isb\n"

    "ldr x4, =0x200000\n"
    "br x4\n"
);*/
   // uart_puts("[OK] Kernel entry point: ");
    //uart_hex((uint32_t)k);uart_puts("\n");
   // k();
        //uart_puts("Hello, World from Bootloader!\n");
        //uart_puts("Uart Initalization Complete\n");
}