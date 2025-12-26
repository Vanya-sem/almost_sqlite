#include "../include/database.h"
#include "../include/database_types.h"
#include "../include/query_executor.h"
#include "../include/sql_parser.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>

// Функция для вывода результата SELECT запроса
void printSelectResult(const QueryResult& result, const std::vector<std::string>& columns) {
    if (result.records.empty()) {
        std::cout << "Записей не найдено.\n";
        return;
    }
    
    // Выводим заголовки
    for (const auto& col : columns) {
        std::cout << std::setw(15) << std::left << col;
    }
    std::cout << "\n";
    std::cout << std::string(columns.size() * 15, '-') << "\n";
    
    // Выводим данные
    for (const auto& record : result.records) {
        for (const auto& col : columns) {
            auto it = record.values.find(col);
            if (it != record.values.end()) {
                const CellValue& val = it->second;
                if (val.is_null) {
                    std::cout << std::setw(15) << std::left << "NULL";
                } else {
                    switch (val.type) {
                        case DataType::INT:
                        case DataType::DATE:
                            std::cout << std::setw(15) << std::left << val.int_value;
                            break;
                        case DataType::FLOAT:
                        case DataType::DOUBLE:
                            std::cout << std::setw(15) << std::left << std::fixed << std::setprecision(2) << val.double_value;
                            break;
                        case DataType::BOOL:
                            std::cout << std::setw(15) << std::left << (val.bool_value ? "true" : "false");
                            break;
                        case DataType::VARCHAR:
                        case DataType::TEXT:
                            std::cout << std::setw(15) << std::left << val.string_value;
                            break;
                    }
                }
            } else {
                std::cout << std::setw(15) << std::left << "NULL";
            }
        }
        std::cout << "\n";
    }
    
    std::cout << "\nНайдено записей: " << result.records.size() << "\n";
}

int main() {
    std::cout << "=== Консольная СУБД ===\n";
    std::cout << "Введите SQL запросы (или 'exit' для выхода)\n\n";
    
    Database db("mydatabase");
    
    std::string input;
    while (true) {
        std::cout << "SQL> ";
        std::getline(std::cin, input);
        
        if (input == "exit" || input == "quit") {
            break;
        }
        
        if (input.empty()) {
            continue;
        }
        
        // Выполняем запрос
        QueryResult result = db.execute(input);
        
        if (!result.success) {
            std::cout << "ОШИБКА: " << result.error_message << "\n\n";
            continue;
        }
        
        // Определяем тип запроса для вывода
        ParsedQuery query = SQLParser::parse(input);
        
        if (query.type == QueryType::SELECT && query.select) {
            // Определяем колонки для вывода
            std::vector<std::string> columns;
            if (query.select->columns.size() == 1 && query.select->columns[0] == "*") {
                // Получаем все колонки из первой записи
                if (!result.records.empty()) {
                    for (const auto& pair : result.records[0].values) {
                        columns.push_back(pair.first);
                    }
                }
            } else {
                columns = query.select->columns;
            }
            
            printSelectResult(result, columns);
        } else if (query.type == QueryType::INSERT || 
                   query.type == QueryType::UPDATE || 
                   query.type == QueryType::DELETE) {
            std::cout << "Запрос выполнен успешно. Затронуто строк: " << result.affected_rows << "\n\n";
        } else if (query.type == QueryType::CREATE_TABLE || 
                   query.type == QueryType::DROP_TABLE) {
            std::cout << "Запрос выполнен успешно.\n\n";
        } else {
            std::cout << "Запрос выполнен успешно.\n\n";
        }
    }
    
    std::cout << "До свидания!\n";
    return 0;
}

