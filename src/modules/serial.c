#include "../includes/serial.h"

serial serial1;

void transfer_bit() {
    serial1.value <<= 1;
    serial1.value |= 1;
    serial1.shifted_bits++;
}
