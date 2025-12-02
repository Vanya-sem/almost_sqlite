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

    uint8_t* buffer;
    size_t size = Serializer::serialize(type, input, &buffer, max_length);

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
    free(buffer);
}


int main2() {

    test_data(All_types::INT, "2147483647"); 
    test_data(All_types::INT, "3000000000"); 
 
    test_data(All_types::FLOAT, "3.141592"); 
    test_data(All_types::FLOAT, "3.14159265"); 

    test_data(All_types::DATE, "2023-12-25");  
    test_data(All_types::DATE, "2023-13-01");  

    test_data(All_types::CHAR, "Hello", true, 10); 
    test_data(All_types::CHAR, "VeryLongString", true, 5);

    return 0;
}