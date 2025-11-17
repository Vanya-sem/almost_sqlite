#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include "types.h"
#include <stdlib.h>

class Serializer {
public:
    static size_t serialize(All_types type, const char* input, uint8_t** buffer, int max_length = 0) {
        switch (type) {
        case All_types::BIT:
            return serialize_bit(input, buffer);
        case All_types::TINYINT:
            return serialize_tinyint(input, buffer);
        case All_types::SMALLINT:
            return serialize_smallint(input, buffer);
        case All_types::INT:
            return serialize_int(input, buffer);
        case All_types::BIGINT:
            return serialize_bigint(input, buffer);
        case All_types::FLOAT:
            return serialize_float(input, buffer);
        case All_types::REAL:
            return serialize_real(input, buffer);
        case All_types::DATE:
            return serialize_date(input, buffer);
        case All_types::TIME:
            return serialize_time(input, buffer);
        case All_types::DATETIME:
            return serialize_datetime(input, buffer);
        case All_types::CHAR:
            return serialize_char(input, buffer, max_length);
        case All_types::VARCHAR:
            return serialize_varchar(input, buffer, max_length);
        case All_types::TEXT:
            return serialize_text(input, buffer);
        default:
            return 0;
        }
    }

private:

    static size_t serialize_bit(const char* input, uint8_t** buffer) {
        uint8_t value = (strcmp(input, "1") == 0 || strcmp(input, "b'1'") == 0) ? 1 : 0;
        *buffer = (uint8_t*)malloc(1);
        (*buffer)[0] = value;
        return 1; 
    }

    static size_t serialize_tinyint(const char* input, uint8_t** buffer) {
        uint8_t value = static_cast<uint8_t>(atoi(input));
        *buffer = (uint8_t*)malloc(1);
        (*buffer)[0] = value;
        return 1;
    }

    static size_t serialize_smallint(const char* input, uint8_t** buffer) {
        int16_t value = static_cast<int16_t>(atoi(input));
        // Little-endian (храним младший байт по младшему адресу)
        *buffer = (uint8_t*)malloc(2);
        (*buffer)[0] = value & 0xFF;
        (*buffer)[1] = (value >> 8) & 0xFF;
        return 2; 
    }

    static size_t serialize_int(const char* input, uint8_t** buffer) {
        int32_t value = atoi(input);
        // Little-endian
        *buffer = (uint8_t*)malloc(4);
        (*buffer)[0] = value & 0xFF;
        (*buffer)[1] = (value >> 8) & 0xFF;
        (*buffer)[2] = (value >> 16) & 0xFF;
        (*buffer)[3] = (value >> 24) & 0xFF;
        return 4; 
    }

    static size_t serialize_bigint(const char* input, uint8_t** buffer) {
        int64_t value = atoll(input);
        // Little-endian
        *buffer = (uint8_t*)malloc(8);
        for (int i = 0; i < 8; i++) {
            (*buffer)[i] = (value >> (i * 8)) & 0xFF;
        }
        return 8; 
    }

    static size_t serialize_float(const char* input, uint8_t** buffer) {
        float value = strtof(input, nullptr);
        // копирование побайтно (IEEE 754)
        *buffer = (uint8_t*)malloc(8);
        memcpy(*buffer, &value, sizeof(float));
        return sizeof(float);
    }

    static size_t serialize_real(const char* input, uint8_t** buffer) {
        double value = strtod(input, nullptr);
        // IEEE 754
        *buffer = (uint8_t*)malloc(16);
        memcpy(*buffer, &value, sizeof(double));
        return sizeof(double); 
    }

    static size_t serialize_date(const char* input, uint8_t** buffer) {
        // YYYY-MM-DD
        int year = atoi(input);
        int month = atoi(input + 5);
        int day = atoi(input + 8);
        *buffer = (uint8_t*)malloc(4);
        (*buffer)[0] = year & 0xFF;
        (*buffer)[1] = (year >> 8) & 0xFF;
        (*buffer)[2] = month;
        (*buffer)[3] = day;

        return 4;
    }

    static size_t serialize_time(const char* input, uint8_t** buffer) {
        //  HH:MM:SS[.ffffff]
        int hour = atoi(input);
        int minute = atoi(input + 3);
        int second = atoi(input + 6);

        const char* dot_pos = strchr(input, '.');
        size_t size = 3;
        if (dot_pos) size_t size = 6;
        else size_t size = 3;

        *buffer = (uint8_t*)malloc(size);

        (*buffer)[0] = hour;
        (*buffer)[1] = minute;
        (*buffer)[2] = second;


        if (dot_pos) {
            uint32_t microsecond = atoi(dot_pos + 1);
            int digits = strlen(dot_pos + 1);
            for (int i = digits; i < 6; i++) {
                microsecond *= 10;
            }

            (*buffer)[3] = microsecond & 0xFF;
            (*buffer)[4] = (microsecond >> 8) & 0xFF;
            (*buffer)[5] = (microsecond >> 16) & 0xFF;
        }

        return size;
    }

    static size_t serialize_datetime(const char* input, uint8_t** buffer) {
        // YYYY-MM-DD HH:MM:SS[.ffffff]

        const char* dot_pos = strchr(input, '.');
        size_t size = 7;
        if (dot_pos) size_t size = 10;
        else size_t size = 7;

        *buffer = (uint8_t*)malloc(size);

        (*buffer)[0] = atoi(input) & 0xFF;         
        (*buffer)[1] = (atoi(input) >> 8) & 0xFF;  
        (*buffer)[2] = atoi(input + 5);            
        (*buffer)[3] = atoi(input + 8);            

        (*buffer)[4] = atoi(input + 11);           
        (*buffer)[5] = atoi(input + 14);           
        (*buffer)[6] = atoi(input + 17);         

        if (dot_pos) {
            uint32_t microsecond = atoi(dot_pos + 1);
            int digits = strlen(dot_pos + 1);
            for (int i = digits; i < 6; i++) {
                microsecond *= 10;
            }

            (*buffer)[7] = microsecond & 0xFF;
            (*buffer)[8] = (microsecond >> 8) & 0xFF;
            (*buffer)[9] = (microsecond >> 16) & 0xFF;
        }

        return size;
    }

    //ААААААААААААААААААААААААААААААААААААААААААААААААА

    static size_t serialize_char(const char* input, uint8_t** buffer, int max_length) {
        size_t input_len = strlen(input);

        if (max_length <= 0) { //При пустой заполняется хотя бы 1 байт пробелом
            max_length = 1; 
        }
        *buffer = (uint8_t*)malloc(max_length);

        size_t copy_len = (input_len > max_length) ? max_length : input_len;
        memcpy(*buffer, input, copy_len);

        for (size_t i = copy_len; i < max_length; i++) {
            (*buffer)[i] = '\0'; // 
        }

        return max_length; 
    }

    static size_t serialize_varchar(const char* input, uint8_t** buffer, int max_length) {
        size_t length = strlen(input);

        if (max_length > 0 && length > max_length) {
            length = max_length;
        }
        *buffer = (uint8_t*)malloc(length + 2);

        (*buffer)[0] = length & 0xFF;
        (*buffer)[1] = (length >> 8) & 0xFF;

        memcpy(*buffer + 2, input, length);

        return 2 + length; 
    }

    static size_t serialize_text(const char* input, uint8_t** buffer) {
        uint32_t length = strlen(input);
        *buffer = (uint8_t*)malloc(length + 4);
        (*buffer)[0] = length & 0xFF;
        (*buffer)[1] = (length >> 8) & 0xFF;
        (*buffer)[2] = (length >> 16) & 0xFF;
        (*buffer)[3] = (length >> 24) & 0xFF;

        memcpy(*buffer + 4, input, length);

        return 4 + length;
    }
};