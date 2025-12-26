#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "database_types.h"
#include "sql_query.h"
#include <string>
#include <vector>
#include <fstream>

// Менеджер хранения данных в бинарных файлах
class StorageManager {
public:
    StorageManager(const std::string& db_path);
    ~StorageManager();
    
    // Работа с метаданными таблиц
    bool createTable(const TableMetadata& metadata);
    bool dropTable(const std::string& table_name);
    bool tableExists(const std::string& table_name) const;
    TableMetadata getTableMetadata(const std::string& table_name) const;
    std::vector<std::string> getAllTableNames() const;
    
    // Работа с данными
    bool insertRecord(const std::string& table_name, const Record& record);
    bool updateRecord(const std::string& table_name, int64_t record_id, const Record& record);
    bool deleteRecord(const std::string& table_name, int64_t record_id);
    bool deleteRecords(const std::string& table_name, const std::vector<WhereCondition>& conditions);
    std::vector<Record> selectRecords(const std::string& table_name) const;
    Record getRecordById(const std::string& table_name, int64_t record_id) const;
    
    // Работа с ограничениями
    bool addUniqueConstraint(const UniqueConstraint& constraint);
    bool addForeignKey(const ForeignKey& fk);
    bool checkUniqueConstraint(const std::string& table_name, const std::string& column_name, const CellValue& value) const;
    bool checkPrimaryKeyConstraint(const std::string& table_name, const std::string& column_name, const CellValue& value) const;
    bool checkForeignKey(const std::string& table_name, const std::string& column_name, const CellValue& value) const;
    std::vector<UniqueConstraint> getUniqueConstraints(const std::string& table_name) const;
    std::vector<ForeignKey> getForeignKeys(const std::string& table_name) const;
    
    // Инициализация БД
    bool initializeDatabase();
    
private:
    std::string db_path_;
    std::string metadata_file_;
    std::string data_dir_;
    
    // Внутренние методы
    std::string getTableDataFile(const std::string& table_name) const;
    std::string getTableMetadataFile(const std::string& table_name) const;
    std::string getConstraintsFile() const;
    std::string getForeignKeysFile() const;
    
    // Сериализация/десериализация
    void serializeMetadata(const TableMetadata& metadata, std::ofstream& out) const;
    TableMetadata deserializeMetadata(std::ifstream& in) const;
    void serializeRecord(const Record& record, const TableMetadata& metadata, std::ofstream& out) const;
    Record deserializeRecord(std::ifstream& in, const TableMetadata& metadata) const;
    void serializeCellValue(const CellValue& value, DataType type, int col_size, std::ofstream& out) const;
    CellValue deserializeCellValue(std::ifstream& in, DataType type, int col_size) const;
    
    // Работа с ограничениями
    void saveUniqueConstraints() const;
    void loadUniqueConstraints() const;
    void saveForeignKeys() const;
    void loadForeignKeys() const;
    
    // Вспомогательные методы
    CellValue parseValue(const std::string& str, DataType type) const;
    bool compareCellValues(const CellValue& v1, const CellValue& v2) const;
    bool evaluateCondition(const CellValue& value, const WhereCondition& cond) const;
    
    // Внутренние хранилища
    mutable std::map<std::string, TableMetadata> tables_cache_;
    mutable std::vector<UniqueConstraint> unique_constraints_;
    mutable std::vector<ForeignKey> foreign_keys_;
    mutable bool constraints_loaded_;
};

#endif // STORAGE_MANAGER_H

