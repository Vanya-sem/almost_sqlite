#include "types.h"

int test_for_bit(char bit) {
	if (bit != 0 && bit != 1) return 0;
	else return 1;
}

int test_for_tinyint(char tinyint) {
	if (tinyint < 0 || tinyint > 255) return 0;
	else return 1;
}

