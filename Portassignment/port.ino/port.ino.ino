#define MOTOR_CLOCK_PIN 2   // PORTD
#define DAC_CLOCK_PIN 7     // PORTD

#define CLOCK_BITS ((1 << MOTOR_CLOCK_PIN) | (1 << DAC_CLOCK_PIN))

void send_row(uint16_t *row);

//rol rotate to right by source bits
//lsl logical shift left

#define start(out, in)  asm volatile("lsl %0" : "=r" (in)  : "0" (in)); \
                        asm volatile("rol %0" : "=r" (out));

#define build(out, in)  asm volatile("lsl %0" : "=r" (in)  : "0" (in)); \
                        asm volatile("rol %0" : "=r" (out) : "0" (out));

static inline __attribute__((always_inline)) void send_row_bytes(uint8_t *byte) {
    register uint8_t array[18], pl, pd, pc;
    int i;
    
    pd = PORTD | CLOCK_BITS;
    
    array[ 0] = byte[ 0];
    array[ 1] = byte[ 2];
    array[ 2] = byte[ 4];
    array[ 3] = byte[ 6];
    array[ 4] = byte[ 8];
    array[ 5] = byte[10];
    array[ 6] = byte[12];
    array[ 7] = byte[14];
    array[ 8] = byte[16];
    array[ 9] = byte[18];
    array[10] = byte[20];
    array[11] = byte[22];
    array[12] = byte[24];
    array[13] = byte[26];
    array[14] = byte[28];
    array[15] = byte[30];
    array[16] = byte[32];
    array[17] = byte[34];
    
    for(i = 8; i > 0; i--) {
        start(pl, array[ 8]);
        build(pl, array[ 9]);
        build(pl, array[ 6]);
        build(pl, array[ 7]);
        build(pl, array[ 4]);
        build(pl, array[ 5]);
        build(pl, array[ 2]);
        build(pl, array[ 3]);
        
        PORTD = pd & ~CLOCK_BITS;           // clock goes low
        pd >>= 2;
        build(pd, array[ 1]);
        build(pd, array[ 0]);
        
        start(pc, array[13]);
        build(pc, array[10]);
        build(pc, array[12]);
        build(pc, array[11]);
        build(pc, array[16]);
        build(pc, array[17]);
        build(pc, array[14]);
        build(pc, array[15]);
        
        PORTL = pl;
        PORTD = pd;                         // clock goes high
        PORTC = pc;
    }
}

void send_row(uint16_t *row) {
    send_row_bytes((uint8_t*)row + 1);      // send all MSB's
    send_row_bytes((uint8_t*)row);          // send all LSB's
}

// L, R, DAC 1-16
uint16_t row[18] = {
    0xCD3F, 0x222F,
    0xDF31, 0x2B17, 0x200C, 0xFBFA,
    0x2743, 0xA795, 0x46AE, 0x9DF2,
    0xE4E9, 0x57B1, 0x9145, 0xD64D,
    0xD784, 0xAA0D, 0x1B70, 0x967D,
};

int main (void)
{
    board_init();

    // Insert application code here, after the board has been initialized.
    
    send_row(row);
    PORTD &= ~CLOCK_BITS;                   // clock goes low
}

/* 132 to first bit out clock high
    25 to clock low, 25 to clock high    x7
    80 to clock low, 25 to clock high
    25 to clock low, 25 to clock high    x7
    36 to clock low

    3.125 us/bit max (320 kHz)
    ~60 us total
*/
