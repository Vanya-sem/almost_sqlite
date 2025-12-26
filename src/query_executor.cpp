#include "../include/query_executor.h"
#include "../include/database_types.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cctype>

QueryExecutor::QueryExecutor(StorageManager* storage) : storage_(storage) {}

QueryResult QueryExecutor::execute(const ParsedQuery& query) {
    QueryResult result;
    
    switch (query.type) {
        case QueryType::SELECT:
            if (query.select) {
                result = executeSelect(*query.select);
            } else {
                result.success = false;
                result.error_message = "Ошибка: SELECT запрос пуст";
            }
            break;
            
        case QueryType::INSERT:
            if (query.insert) {
                result = executeInsert(*query.insert);
            } else {
                result.success = false;
                result.error_message = "Ошибка: INSERT запрос пуст";
            }
            break;
            
        case QueryType::UPDATE:
            if (query.update) {
                result = executeUpdate(*query.update);
            } else {
                result.success = false;
                result.error_message = "Ошибка: UPDATE запрос пуст";
            }
            break;
            
        case QueryType::DELETE:
            if (query.delete_) {
                result = executeDelete(*query.delete_);
            } else {
                result.success = false;
                result.error_message = "Ошибка: DELETE запрос пуст";
            }
            break;
            
        case QueryType::CREATE_TABLE:
            if (query.create_table) {
                result = executeCreateTable(*query.create_table);
            } else {
                result.success = false;
                result.error_message = "Ошибка: CREATE TABLE запрос пуст";
            }
            break;
            
        case QueryType::DROP_TABLE:
            if (query.drop_table) {
                result = executeDropTable(*query.drop_table);
            } else {
                result.success = false;
                result.error_message = "Ошибка: DROP TABLE запрос пуст";
            }
            break;
            
        default:
            result.success = false;
            result.error_message = "Неизвестный тип запроса";
            break;
    }
    
    return result;
}

QueryResult QueryExecutor::executeSelect(const SelectQuery& query) {
    QueryResult result;
    
    if (!storage_->tableExists(query.table)) {
        result.success = false;
        result.error_message = "Таблица '" + query.table + "' не существует";
        return result;
    }
    
    // Получаем все записи
    std::vector<Record> all_records = storage_->selectRecords(query.table);
    TableMetadata metadata = storage_->getTableMetadata(query.table);
    
    // Применяем WHERE условия
    std::vector<Record> filtered_records;
    for (const auto& record : all_records) {
        bool matches = true;
        for (const auto& condition : query.where_conditions) {
            if (!evaluateWhereCondition(record, condition, metadata)) {
                matches = false;
                break;
            }
        }
        if (matches) {
            filtered_records.push_back(record);
        }
    }
    
    // Применяем ORDER BY
    if (!query.order_by.empty()) {
        std::sort(filtered_records.begin(), filtered_records.end(), 
            [&](const Record& r1, const Record& r2) {
                auto it1 = r1.values.find(query.order_by);
                auto it2 = r2.values.find(query.order_by);
                
                if (it1 == r1.values.end() || it2 == r2.values.end()) {
                    return false;
                }
                
                bool less = compareValues(it1->second, it2->second, "<");
                return query.order_asc ? less : !less;
            });
    }
    
    // Применяем LIMIT и OFFSET
    size_t start = query.offset > 0 ? query.offset : 0;
    size_t end = query.limit > 0 ? start + query.limit : filtered_records.size();
    if (end > filtered_records.size()) {
        end = filtered_records.size();
    }
    
    for (size_t i = start; i < end; ++i) {
        result.records.push_back(filtered_records[i]);
    }
    
    result.success = true;
    return result;
}

QueryResult QueryExecutor::executeInsert(const InsertQuery& query) {
    QueryResult result;
    
    if (!storage_->tableExists(query.table)) {
        result.success = false;
        result.error_message = "Таблица '" + query.table + "' не существует";
        return result;
    }
    
    TableMetadata metadata = storage_->getTableMetadata(query.table);
    
    int inserted = 0;
    for (const auto& values : query.values) {
        Record record = convertToRecord(values, query.columns, metadata);
        
        if (storage_->insertRecord(query.table, record)) {
            inserted++;
        } else {
            result.success = false;
            result.error_message = "Ошибка при вставке записи (возможно, нарушение ограничений)";
            result.affected_rows = inserted;
            return result;
        }
    }
    
    result.success = true;
    result.affected_rows = inserted;
    return result;
}

QueryResult QueryExecutor::executeUpdate(const UpdateQuery& query) {
    QueryResult result;
    
    if (!storage_->tableExists(query.table)) {
        result.success = false;
        result.error_message = "Таблица '" + query.table + "' не существует";
        return result;
    }
    
    // Упрощенная реализация UPDATE
    result.success = false;
    result.error_message = "UPDATE пока не полностью реализован";
    return result;
}

QueryResult QueryExecutor::executeDelete(const DeleteQuery& query) {
    QueryResult result;

    // Проверяем, существует ли таблица
    if (!storage_->tableExists(query.table)) {
        result.success = false;
        result.error_message = "Таблица '" + query.table + "' не существует";
        return result;
    }

    // Удаляем записи по условиям WHERE
    bool success = storage_->deleteRecords(query.table, query.where_conditions);

    if (success) {
        result.success = true;
        result.affected_rows = query.where_conditions.size(); 
    }
    else {
        result.success = false;
        result.error_message = "Ошибка при удалении записей из таблицы '" + query.table + "'";
    }

    return result;
}

QueryResult QueryExecutor::executeCreateTable(const CreateTableQuery& query) {
    QueryResult result;
    
    if (storage_->tableExists(query.table_name)) {
        result.success = false;
        result.error_message = "Таблица '" + query.table_name + "' уже существует";
        return result;
    }
    
    TableMetadata metadata;
    metadata.name = query.table_name;
    
    for (const auto& col_def : query.columns) {
        Column col;
        col.name = col_def.name;
        col.type = parseDataType(col_def.type);
        col.size = col_def.size;
        col.is_primary_key = col_def.is_primary_key;
        col.is_not_null = col_def.is_not_null || col_def.is_primary_key;  // PRIMARY KEY автоматически NOT NULL
        col.is_unique = false;  // Будет обрабатываться отдельно через UNIQUE constraint
        col.default_value = col_def.default_value;
        
        metadata.columns.push_back(col);
    }
    
    // Проверяем, что есть только один PRIMARY KEY
    int pk_count = 0;
    for (const auto& col : metadata.columns) {
        if (col.is_primary_key) {
            pk_count++;
        }
    }
    if (pk_count > 1) {
        result.success = false;
        result.error_message = "Может быть только один PRIMARY KEY";
        return result;
    }
    
    if (storage_->createTable(metadata)) {
        result.success = true;
        result.affected_rows = 0;
    } else {
        result.success = false;
        result.error_message = "Ошибка при создании таблицы";
    }
    
    return result;
}

QueryResult QueryExecutor::executeDropTable(const DropTableQuery& query) {
    QueryResult result;
    
    if (!storage_->tableExists(query.table_name)) {
        if (query.if_exists) {
            result.success = true;
            result.affected_rows = 0;
            return result;
        } else {
            result.success = false;
            result.error_message = "Таблица '" + query.table_name + "' не существует";
            return result;
        }
    }
    
    if (storage_->dropTable(query.table_name)) {
        result.success = true;
        result.affected_rows = 0;
    } else {
        result.success = false;
        result.error_message = "Ошибка при удалении таблицы";
    }
    
    return result;
}

DataType QueryExecutor::parseDataType(const std::string& type_str) const {
    std::string upper = type_str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    
    if (upper == "INT" || upper == "INTEGER") {
        return DataType::INT;
    } else if (upper == "VARCHAR") {
        return DataType::VARCHAR;
    } else if (upper == "TEXT") {
        return DataType::TEXT;
    } else if (upper == "DATE") {
        return DataType::DATE;
    } else if (upper == "FLOAT") {
        return DataType::FLOAT;
    } else if (upper == "DOUBLE") {
        return DataType::DOUBLE;
    } else if (upper == "BOOL" || upper == "BOOLEAN") {
        return DataType::BOOL;
    }
    
    return DataType::INT;  // По умолчанию
}

CellValue QueryExecutor::parseValue(const std::string& value_str, DataType type) const {
    CellValue value;
    value.type = type;
    value.is_null = false;
    
    // Убираем кавычки если есть
    std::string clean_value = value_str;
    if (clean_value.length() >= 2 && 
        ((clean_value.front() == '\'' && clean_value.back() == '\'') ||
         (clean_value.front() == '"' && clean_value.back() == '"'))) {
        clean_value = clean_value.substr(1, clean_value.length() - 2);
    }
    
    if (clean_value == "NULL" || clean_value.empty()) {
        value.is_null = true;
        return value;
    }
    
    try {
        switch (type) {
            case DataType::INT:
                value.int_value = std::stoll(clean_value);
                break;
            case DataType::FLOAT:
            case DataType::DOUBLE:
                value.double_value = std::stod(clean_value);
                break;
            case DataType::BOOL:
                value.bool_value = (clean_value == "true" || clean_value == "1");
                break;
            case DataType::VARCHAR:
            case DataType::TEXT:
            case DataType::DATE:
                value.string_value = clean_value;
                break;
        }
    } catch (...) {
        value.is_null = true;
    }
    
    return value;
}

bool QueryExecutor::evaluateWhereCondition(const Record& record, const WhereCondition& condition, const TableMetadata& metadata) const {
    auto it = record.values.find(condition.column);
    if (it == record.values.end()) {
        return false;
    }
    
    // Находим тип колонки
    DataType col_type = DataType::INT;
    for (const auto& col : metadata.columns) {
        if (col.name == condition.column) {
            col_type = col.type;
            break;
        }
    }
    
    CellValue record_value = it->second;
    CellValue condition_value = parseValue(condition.value, col_type);
    
    return compareValues(record_value, condition_value, condition.operator_);
}

bool QueryExecutor::compareValues(const CellValue& v1, const CellValue& v2, const std::string& op) const {
    if (v1.type != v2.type) {
        return false;
    }
    
    if (v1.is_null || v2.is_null) {
        return op == "=" && v1.is_null && v2.is_null;
    }
    
    switch (v1.type) {
        case DataType::INT:
        case DataType::DATE:
            if (op == "=") return v1.int_value == v2.int_value;
            if (op == "!=" || op == "<>") return v1.int_value != v2.int_value;
            if (op == "<") return v1.int_value < v2.int_value;
            if (op == ">") return v1.int_value > v2.int_value;
            if (op == "<=") return v1.int_value <= v2.int_value;
            if (op == ">=") return v1.int_value >= v2.int_value;
            break;
            
        case DataType::FLOAT:
        case DataType::DOUBLE:
            if (op == "=") return v1.double_value == v2.double_value;
            if (op == "!=" || op == "<>") return v1.double_value != v2.double_value;
            if (op == "<") return v1.double_value < v2.double_value;
            if (op == ">") return v1.double_value > v2.double_value;
            if (op == "<=") return v1.double_value <= v2.double_value;
            if (op == ">=") return v1.double_value >= v2.double_value;
            break;
            
        case DataType::BOOL:
            if (op == "=") return v1.bool_value == v2.bool_value;
            if (op == "!=" || op == "<>") return v1.bool_value != v2.bool_value;
            break;
            
        case DataType::VARCHAR:
        case DataType::TEXT:
        //case DataType::DATE:
            if (op == "=") return v1.string_value == v2.string_value;
            if (op == "!=" || op == "<>") return v1.string_value != v2.string_value;
            if (op == "LIKE") {
                // Простая реализация LIKE (поддержка только % в конце)
                if (v2.string_value.back() == '%') {
                    std::string prefix = v2.string_value.substr(0, v2.string_value.length() - 1);
                    return v1.string_value.find(prefix) == 0;
                }
                return v1.string_value == v2.string_value;
            }
            break;
    }
    
    return false;
}

Record QueryExecutor::convertToRecord(const std::vector<std::string>& values, const std::vector<std::string>& columns, const TableMetadata& metadata) const {
    Record record;
    
    // Если колонки не указаны, используем все колонки таблицы
    std::vector<std::string> target_columns = columns.empty() ? 
        [&metadata]() {
            std::vector<std::string> cols;
            for (const auto& col : metadata.columns) {
                cols.push_back(col.name);
            }
            return cols;
        }() : columns;
    
    for (size_t i = 0; i < values.size() && i < target_columns.size(); ++i) {
        // Находим тип колонки
        DataType col_type = DataType::INT;
        for (const auto& col : metadata.columns) {
            if (col.name == target_columns[i]) {
                col_type = col.type;
                break;
            }
        }
        
        CellValue value = parseValue(values[i], col_type);
        record.values[target_columns[i]] = value;
    }
    
    // Заполняем значения по умолчанию для не указанных колонок
    // PRIMARY KEY обрабатывается в StorageManager (автоинкремент)
    for (const auto& col : metadata.columns) {
        if (record.values.find(col.name) == record.values.end()) {
            // PRIMARY KEY с типом INT будет автоматически заполнен
            if (col.is_primary_key && col.type == DataType::INT) {
                // Оставляем пустым - StorageManager заполнит автоматически
                continue;
            }
            
            if (!col.default_value.empty()) {
                CellValue default_val = parseValue(col.default_value, col.type);
                record.values[col.name] = default_val;
            } else if (!col.is_not_null) {
                CellValue null_val;
                null_val.is_null = true;
                null_val.type = col.type;
                record.values[col.name] = null_val;
            }
            // Если is_not_null и нет default_value, оставляем пустым для проверки в StorageManager
        }
    }
    
    return record;
}

