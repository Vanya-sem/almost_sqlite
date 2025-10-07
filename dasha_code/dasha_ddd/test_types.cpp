#include "types.h"

int test_for_bit(char bit) {//проверка что занчение BIT (1,0)
    if (bit != 0 && bit != 1) return 0;
    else return 1;
}

int test_for_tinyint(char tinyint) {//проверка что значение tinyint
    int val = (unsigned char)tinyint; 
    if (val < 0 || val > 255) return 0;
    else return 1;
}