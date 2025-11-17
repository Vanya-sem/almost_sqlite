#include "serializer.h"
#include "validator.h"
#include <iostream>
#include <iomanip>

void test_data(All_types type, const char* input, bool not_null = false, int max_length = 0) {
    std::cout << "Тест ";
    switch (type) {
    case All_types::BIT: std::cout << "BIT"; break;
    case All_types::TINYINT: std::cout << "TINYINT"; break;
    case All_types::SMALLINT: std::cout << "SMALLINT"; break;
    case All_types::INT: std::cout << "INT"; break;
    case All_types::BIGINT: std::cout << "BIGINT"; break;
    case All_types::FLOAT: std::cout << "FLOAT"; break;
    case All_types::REAL: std::cout << "REAL"; break;
    case All_types::DATETIME: std::cout << "DATETIME"; break;
    case All_types::DATE: std::cout << "DATE"; break;
    case All_types::TIME: std::cout << "TIME"; break;
    case All_types::CHAR: std::cout << "CHAR"; break;
    case All_types::VARCHAR: std::cout << "VARCHAR"; break;
    case All_types::TEXT: std::cout << "TEXT"; break;
    }
    std::cout << "\n " << std::endl;

    std::cout << "Данные: '" << input << "'" << std::endl;

    try {
        if (!SimpleValidator::validate_before_write(input, type, not_null, max_length)) {
            std::cout << "не подходит для типа" << std::endl;
            std::cout << std::endl;
            return;
        }
    }catch (const ErrorType& e) {
            std::cout << " ошибка валидации: " << e.what() << std::endl;
            std::cout << std::endl;
            return;
    }catch (const std::exception& e) {
            std::cout << " ноу нейм ошибка: " << e.what() << std::endl;
            std::cout << std::endl;
            return;
        }

    std::cout << "СЮДА" << std::endl;

    uint8_t buffer[256];
    size_t size = Serializer::serialize(type, input, buffer, max_length);

    std::cout << "Байты (" << size << "): ";

    std::cout << "HEX: ";
    for (size_t i = 0; i < size; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(buffer[i]) << " ";
    }

    std::cout << "| DEC: ";
    for (size_t i = 0; i < size; i++) {
        std::cout << std::dec << static_cast<int>(buffer[i]) << " ";
    }
    std::cout << std::endl << std::endl;
}


int main() {

    // BIT
    test_data(BIT, "1");
    test_data(BIT, "2"); 

    // TINYINT
    test_data(TINYINT, "100");
    test_data(TINYINT, "300");  

    // SMALLINT
    test_data(SMALLINT, "25000");
    test_data(SMALLINT, "40000"); 

    // INT
    test_data(INT, "2147483647");
    test_data(INT, "3000000000");  

    // BIGINT
    test_data(BIGINT, "9223372036854775807");
    test_data(BIGINT, "9999999999999999999"); 

    // FLOAT
    test_data(FLOAT, "3.141592");
    test_data(FLOAT, "3.14159265");  

    // REAL
    test_data(REAL, "3.141592653589793");
    test_data(REAL, "3.1415926535897932"); 

    // DATE
    test_data(DATE, "2023-12-25");
    test_data(DATE, "2023-13-01");  

    // TIME
    test_data(TIME, "14:30:00.500");
    test_data(TIME, "25:00:00");  

    // DATETIME
    test_data(DATETIME, "2023-12-25 14:30:00.123456");
    test_data(DATETIME, "2023-12-25 14:30:00.1234567");  

    // CHAR
    test_data(CHAR, "Hello", true, 10);
    test_data(CHAR, "VeryLongString", true, 5);  

    // VARCHAR
    test_data(VARCHAR, "Test", true, 10);
    test_data(VARCHAR, "TooLongText", true, 5);  

    // TEXT
    test_data(TEXT, "This is a long text");
    test_data(TEXT, ""); 

    return 0;
}