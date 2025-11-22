// validator.h
#pragma once
#include <string>
#include <cctype>
#include "types.h"

// Максимальное значение float ≈ 3.402823 × 10³⁸
#define FLOAT_MAX 3.402823e+38f
#define FLOAT_MIN -3.402823e+38f

// Минимальное положительное ≈ 1.175494 × 10⁻³⁸  
#define FLOAT_MIN_POSITIVE 1.175494e-38f

class ErrorType : public std::exception {
private:
    std::string message;
public:
    ErrorType() : message("Unknown database error") {}
    explicit ErrorType(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};

class SimpleValidator {
public:
    static bool validate_before_write(const char* input, All_types type_id,
        bool not_null = false, int max_length = 0) {
        // NULL
        if (type_id != All_types::TEXT && (input == nullptr || input[0] == '\0' || strcmp(input, "NULL") == 0)) {
            if (not_null) {
                throw ErrorType("NULL value violates NOT NULL constraint");
            }
            return true;
        }

        bool isValid = false;

        switch (type_id) {
        case All_types::BIT:
            isValid = test_for_bit(input);
            if (!isValid) throw ErrorType("Invalid bit value ");
            break;

        case All_types::TINYINT:
            isValid = test_for_tinyint(input);
            if (!isValid) throw ErrorType("TINYINT out of range (-128 to 127)");
            break;

        case All_types::SMALLINT:
            isValid = test_for_smallint(input);
            if (!isValid) throw ErrorType("SMALLINT out of range (-32768 to 32767)");
            break;

        case All_types::INT_TYPE:
            isValid = test_for_int(input);
            if (!isValid) throw ErrorType("INT out of range (-2147483648 to 2147483647)");
            break;

        case All_types::BIGINT:
            isValid = test_for_bigint(input);
            if (!isValid) throw ErrorType("BIGINT value out of range");
            break;

        case All_types::FLOAT_TYPE:
            isValid = test_for_float(input);
            if (!isValid) throw ErrorType("Invalid FLOAT value");
            break;

        case All_types::REAL:
            isValid = test_for_real(input);
            if (!isValid) throw ErrorType("Invalid REAL value");
            break;

        case All_types::DATETIME:
            isValid = test_for_datetime(input);
            if (!isValid) throw ErrorType("Invalid DATETIME format. Expected: YYYY-MM-DD HH:MM:SS");
            break;

        case All_types::SMALLDATETIME:
            isValid = test_for_smalldatetime(input);
            if (!isValid) throw ErrorType("Invalid SMALLDATETIME format");
            break;

        case All_types::DATE_TYPE:
            isValid = test_for_date(input);
            if (!isValid) throw ErrorType("Invalid DATE format. Expected: YYYY-MM-DD");
            break;

        case All_types::TIME:
            isValid = test_for_time(input);
            if (!isValid) throw ErrorType("Invalid TIME format. Expected: HH:MM:SS");
            break;

        case All_types::CHAR_TYPE:
            isValid = test_for_char(input, max_length);
            if (!isValid) throw ErrorType("CHAR value exceeds maximum length of " + std::to_string(max_length));
            break;

        case All_types::VARCHAR:
            isValid = test_for_varchar(input, max_length);
            if (!isValid) throw ErrorType("VARCHAR value exceeds maximum length of " + std::to_string(max_length));
            break;

        case All_types::TEXT:
            isValid = test_for_text(input);
            if (!isValid) throw ErrorType("Invalid TEXT value");
            break;

        default:
            throw ErrorType("Unknown data type");
        }

        return true;
    }
private:
    static bool test_for_bit(const char* input) {
        return strcmp(input, "true") == 0 ||
            strcmp(input, "false") == 0 ||
            strcmp(input, "1") == 0 ||
            strcmp(input, "0") == 0 ||
            strcmp(input, "TRUE") == 0 ||
            strcmp(input, "FALSE") == 0 ||
            strcmp(input, "b'0'") == 0 ||
            strcmp(input, "b'1'") == 0;
    }

    static bool test_for_tinyint(const char* input) {
        char* endptr;
        errno = 0;

        long value = strtol(input, &endptr, 10);

        if (errno != 0 || *endptr != '\0' || endptr == input) {
            return false;
        }

        return value >= 0 && value <= 255;
    }

    static bool test_for_smallint(const char* input) {
        char* endptr;
        errno = 0;

        long value = strtol(input, &endptr, 10);

        if (errno != 0 || *endptr != '\0' || endptr == input) {
            return false;
        }

        return value >= -32768 && value <= 32767;
    }

    static bool test_for_int(const char* input) {
        char* endptr;
        errno = 0;

        long value = strtol(input, &endptr, 10);

        if (errno != 0 || *endptr != '\0' || endptr == input) {
            return false;
        }

        return value >= -2147483648L && value <= 2147483647L;
    }

    static bool test_for_bigint(const char* input) {
        char* endptr;
        errno = 0;

        long long value = strtoll(input, &endptr, 10);

        if (errno != 0 || *endptr != '\0' || endptr == input) {
            return false;
        }

        return true;
    }

    static bool test_for_float(const char* input) {
        char* endptr;
        errno = 0;

        float value = strtof(input, &endptr);

        if (errno != 0 || *endptr != '\0' || endptr == input) {
            return false;
        }

        if (value == HUGE_VALF || value == -HUGE_VALF) {
            return false;
        }

        return has_valid_float_precision(input, 7);
    }

    static bool test_for_real(const char* input) {
        char* endptr;
        errno = 0;

        double value = strtod(input, &endptr);

        return errno == 0 && *endptr == '\0' && endptr != input && has_valid_float_precision(input, 15);
    }

    static bool is_valid_sql_date(int year, int month, int day) {
        if (year < 1000 || year > 9999) {
            return false;
        }

        if (month < 1 || month > 12) {
            return false;
        }

        if (day < 1) {
            return false;
        }

        static const int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

        int max_days = days_in_month[month - 1];

        if (month == 2 && is_leap_year(year)) {
            max_days = 29;
        }

        return day <= max_days;
    }

    static bool is_valid_sql_time(int hour, int minute, int second) {
        return (hour >= 0 && hour <= 23) &&
            (minute >= 0 && minute <= 59) &&
            (second >= 0 && second <= 59);
    }

    static bool test_for_date(const char* input) {
        if (strlen(input) != 10 || input[4] != '-' || input[7] != '-') {
            return false;
        }

        for (int i = 0; i < 10; i++) {
            if (i != 4 && i != 7 && !isdigit(input[i])) {
                return false;
            }
        }

        int year = (input[0] - '0') * 1000 + (input[1] - '0') * 100 +
            (input[2] - '0') * 10 + (input[3] - '0');
        int month = (input[5] - '0') * 10 + (input[6] - '0');
        int day = (input[8] - '0') * 10 + (input[9] - '0');

        return is_valid_sql_date(year, month, day);
    }

    static bool is_leap_year(int year) {
        return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
    }

    static bool test_for_datetime(const char* input) {
        int length = strlen(input);

        if (length < 19) {
            return false;
        }

        if (input[4] != '-' || input[7] != '-' ||
            input[10] != ' ' ||
            input[13] != ':' || input[16] != ':') {
            return false;
        }

        for (int i = 0; i < 19; i++) {
            if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16) {
                continue;
            }
            if (!isdigit(input[i])) {
                return false;
            }
        }

        int year = atoi(input);
        int month = atoi(input + 5);
        int day = atoi(input + 8);
        int hour = atoi(input + 11);
        int minute = atoi(input + 14);
        int second = atoi(input + 17);

        if (!is_valid_sql_date(year, month, day) ||
            !is_valid_sql_time(hour, minute, second)) {
            return false;
        }

        if (length > 19) {
            if (input[19] != '.') {
                return false;
            }

            for (int i = 20; i < length; i++) {
                if (!isdigit(input[i])) {
                    return false;
                }
            }

            int microsecond_digits = length - 20;
            if (microsecond_digits > 6) {
                return false;
            }
        }

        return true;
    }

    static bool test_for_smalldatetime(const char* input) {
        if (!test_for_datetime(input)) {
            return false;
        }

        int year = atoi(input);
        int hour = atoi(input + 11);
        int minute = atoi(input + 14);
        int second = atoi(input + 17);

        if (year < 1900 || year > 2079) {
            return false;
        }

        if (second != 0) {
            return false;
        }

        return true;
    }

    static bool test_for_time(const char* input) {
        int length = strlen(input);

        if (length < 8) {
            return false;
        }

        if (input[2] != ':' || input[5] != ':') {
            return false;
        }

        for (int i = 0; i < 8; i++) {
            if (i == 2 || i == 5) continue;
            if (!isdigit(input[i])) {
                return false;
            }
        }

        int hour = atoi(input);
        int minute = atoi(input + 3);
        int second = atoi(input + 6);

        if (!is_valid_sql_time(hour, minute, second)) {
            return false;
        }

        if (length > 8) {
            if (input[8] != '.') {
                return false;
            }

            for (int i = 9; i < length; i++) {
                if (!isdigit(input[i])) {
                    return false;
                }
            }

            int microsecond_digits = length - 9;
            if (microsecond_digits > 6) {
                return false;
            }
        }

        return true;
    }

    static bool test_for_char(const char* input, int max_length) {
        return max_length <= 0 || strlen(input) <= max_length;
    }

    static bool test_for_varchar(const char* input, int max_length) {
        return max_length <= 0 || strlen(input) <= max_length;
    }

    static bool test_for_text(const char* input) {
        return input != nullptr && input[0] != '\0';
    }

    static bool has_valid_float_precision(const char* input, int max_decimal_places) {
        const char* dot_pos = strchr(input, '.');
        if (dot_pos == nullptr) {
            return true;
        }
        const char* exp_pos = strchr(input, 'e');
        const char* exp_pos2 = strchr(input, 'E');
        if (exp_pos2 && (!exp_pos || exp_pos2 < exp_pos)) {
            exp_pos = exp_pos2;
        }

        const char* fractional_start = dot_pos + 1;
        const char* fractional_end = exp_pos ? exp_pos : (input + strlen(input));

        int decimal_places = fractional_end - fractional_start;

        return decimal_places <= max_decimal_places;
    }
};