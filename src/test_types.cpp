#include "types.h"
#include<string>
#include <exception>
#include <cmath>
#include <sstream>

/*class ErrorType : public std::exception {
private:
	std::string message;
public:
	explicit ErrorType(const std::string& msg) : message(msg) {}
	const char* what() const noexcept override {
		return message.c_str();
	}
};

bool test_notNull(char* symb) {
	if not(symb) return 0;
}

bool test_for_bit(char* bit, bool not_null) {
	if (bit == NULL && not_null) return true;
	if (bit == NULL && !not_null) return false;
	if (bit != '0' && bit != '1') return false;
	else return true;
}

bool test_for_tinyint(char* tinyint, bool not_null) {
	if (tinyint == NULL && not_null) return true;
	if (tinyint == NULL && !not_null) return false;
	if (strlen(tinyint) > 3) return 0;
	for (di : tinyint) {
		if (!isdigit(di))
			return false;
	}
	if (strcmp("255", tinyint) && strlen(tinyint) == 3) return 1;
	else if (strlen(tinyint) < 3) return 1;
	else return 0;
}

bool test_for_smallint(char* smallint, bool not_null) {
	if (smallint == NULL && not_null) return true;
	if (smallint == NULL && !not_null) return false;
	if (smallint[0] == '-') {
		if (strlen(smallint) > 6) return false;
		smallint++;
		for (di : smallint) {
			if (!isdigit(di))
				return false;
		}
		if (strcmp("32768", smallint) >= 0 && strlen(smallint) == 5)return true;
		else if (strlen(smallint) < 5) return true;
		else return false;
	}

	if (smallint[0] != '-') {
		if (strlen(smallint) > 5) return false;
		for (di : smallint) {
			if (!isdigit(di))
				return false;
		}
		else if (strcmp("32767", smallint) >= 0 && strlen(smallint) == 5) return true;
		else if (strlen(smallint) < 5) return true;
		else return false;
	}
}

bool test_for_int(char* integer, bool not_null) {
	if (integer == NULL && not_null) return true;
	if (integer == NULL && !not_null) return false;
	if (integer[0] == '-') {
		if (strlen(integer) > 11) return false;
		integer++;
		for (di : integer) {
			if (!isdigit(di))
				return false;
		}
		if (strcmp("2147483648", integer) >= 0 && strlen(integer) == 10)return true;
		else if (strlen(integer) < 10) return true;
		else return false;
	}

	if (integer[0] != '-') {
		for (di : integer) {
			if (!isdigit(di))
				return false;
		}
		if (strlen(integer) > 10) return false;
		else if (strcmp("2147483647", integer) >= 0 && strlen(integer) == 10) return true;
		else if (strlen(integer) < 10) return true;
		else return false;
	}
}


bool test_for_bigint(char* bigint, bool not_null) {
	if (bigint == NULL && not_null) return true;
	if (bigint == NULL && !not_null) return false;
	if (bigint[0] == '-') {
		if (strlen(bigint) > 19) return false;
		bigint++;
		for (di : bigint) {
			if (!isdigit(di))
				return false;
		}
		if (strcmp("9223372036854775808", bigint) >= 0 && strlen(bigint) == 19)return true;
		else if (strlen(bigint) < 19) return true;
		else return false;
	}

	if (bigint[0] != '-') {
		for (di : bigint) {
			if (!isdigit(di))
				return false;
		}
		if (strlen(bigint) > 19) return false;
		else if (strcmp("9223372036854775807", bigint) >= 0 && strlen(integer) == 19) return true;
		else if (strlen(bigint) < 19) return true;
		else return false;
	}
}

bool test_for_float(char* float_type, bool not_null) {
	if (float_type == NULL && not_null) return true;
	if (float_type == NULL && !not_null) return false;
	for (di : float_type) {
		if (!(isdigit(di) || di == '.' || di == '-'))
			return false;
	}
	std::stringstream ss(float_type);
	double value;
	if (!(ss >> value)) {
		trow ErrorType("Ќе удовлетвор€ет требовани€ми типа данных");
		return false;
	}
	return true;
}

bool test_for_real(char* real, bool not_null) {
	if (real == NULL && not_null) return true;
	if (real == NULL && !not_null) return false;
	for (di : real) {
		if (!(isdigit(di) || di == '.' || di == '-'))
			return false;
	}
	std::stringstream ss(real);
	float value;
	if (!(ss >> value)) {
		trow ErrorType("Ќе удовлетвор€ет требовани€ми типа данных");
		return false;
	}
	return true;
}
*/