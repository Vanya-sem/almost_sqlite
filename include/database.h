#ifndef DATABASE_H
#define DATABASE_H

#include "sql_parser.h"
#include "storage_manager.h"
#include "query_executor.h"
#include <string>
#include <vector>

// Главный класс базы данных
class Database {
public:
    Database(const std::string& db_path);
    ~Database();
    
    // Выполнение SQL запроса
    QueryResult execute(const std::string& sql);
    
    // Удобные методы для выполнения запросов
    QueryResult executeQuery(const std::string& sql);
    
    // Получение информации о БД
    std::vector<std::string> getTableNames() const;
    bool tableExists(const std::string& table_name) const;
    
private:
    StorageManager* storage_;
    QueryExecutor* executor_;
    std::string db_path_;
};

#endif // DATABASE_H

