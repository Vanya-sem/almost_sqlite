#ifndef QUERY_EXECUTOR_H
#define QUERY_EXECUTOR_H

#include "sql_parser.h"
#include "storage_manager.h"
#include "database_types.h"
#include <vector>
#include <string>

// Результат выполнения запроса
struct QueryResult {
    bool success;
    std::string error_message;
    std::vector<Record> records;  // Для SELECT
    int affected_rows;            // Для INSERT, UPDATE, DELETE
    
    QueryResult() : success(false), affected_rows(0) {}
};

// Исполнитель SQL запросов
class QueryExecutor {
public:
    QueryExecutor(StorageManager* storage);
    
    // Выполнение запроса
    QueryResult execute(const ParsedQuery& query);
    
    // Выполнение конкретных типов запросов
    QueryResult executeSelect(const SelectQuery& query);
    QueryResult executeInsert(const InsertQuery& query);
    QueryResult executeUpdate(const UpdateQuery& query);
    QueryResult executeDelete(const DeleteQuery& query);
    QueryResult executeCreateTable(const CreateTableQuery& query);
    QueryResult executeDropTable(const DropTableQuery& query);
    
private:
    StorageManager* storage_;
    
    // Вспомогательные методы
    DataType parseDataType(const std::string& type_str) const;
    CellValue parseValue(const std::string& value_str, DataType type) const;
    bool evaluateWhereCondition(const Record& record, const WhereCondition& condition, const TableMetadata& metadata) const;
    bool compareValues(const CellValue& v1, const CellValue& v2, const std::string& op) const;
    Record convertToRecord(const std::vector<std::string>& values, const std::vector<std::string>& columns, const TableMetadata& metadata) const;
};

#endif // QUERY_EXECUTOR_H

