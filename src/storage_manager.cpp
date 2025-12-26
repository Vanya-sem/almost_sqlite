#include "../include/storage_manager.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <map>
#include <algorithm>

#ifdef _WIN32
#include <direct.h>
#define mkdir _mkdir
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

StorageManager::StorageManager(const std::string& db_path) 
    : db_path_(db_path), constraints_loaded_(false) {
    data_dir_ = db_path_ + "/data";
    metadata_file_ = db_path_ + "/metadata.db";
    
    // Создаем директорию если не существует
    #ifdef _WIN32
    _mkdir(db_path_.c_str());
    _mkdir(data_dir_.c_str());
    #else
    mkdir(db_path_.c_str(), 0755);
    mkdir(data_dir_.c_str(), 0755);
    #endif
    
    initializeDatabase();
}

StorageManager::~StorageManager() {
    // Сохраняем ограничения при закрытии
    if (constraints_loaded_) {
        saveUniqueConstraints();
        saveForeignKeys();
    }
}

bool StorageManager::initializeDatabase() {
    // Загружаем метаданные таблиц
    std::ifstream in(metadata_file_, std::ios::binary);
    if (in.is_open()) {
        int table_count;
        in.read(reinterpret_cast<char*>(&table_count), sizeof(int));
        
        for (int i = 0; i < table_count; ++i) {
            TableMetadata metadata = deserializeMetadata(in);
            tables_cache_[metadata.name] = metadata;
        }
        in.close();
    }
    
    // Загружаем ограничения
    loadUniqueConstraints();
    loadForeignKeys();
    
    return true;
}

std::string StorageManager::getTableDataFile(const std::string& table_name) const {
    return data_dir_ + "/" + table_name + ".dat";
}

std::string StorageManager::getTableMetadataFile(const std::string& table_name) const {
    return data_dir_ + "/" + table_name + ".meta";
}

std::string StorageManager::getConstraintsFile() const {
    return db_path_ + "/constraints.db";
}

std::string StorageManager::getForeignKeysFile() const {
    return db_path_ + "/foreign_keys.db";
}

bool StorageManager::createTable(const TableMetadata& metadata) {
    if (tableExists(metadata.name)) {
        return false;
    }
    
    // Вычисляем размер записи
    TableMetadata meta = metadata;
    meta.record_size = 0;
    
    for (auto& col : meta.columns) {
        col.offset = meta.record_size;
        switch (col.type) {
            case DataType::INT:
                meta.record_size += sizeof(int64_t);
                break;
            case DataType::FLOAT:
                meta.record_size += sizeof(float);
                break;
            case DataType::DOUBLE:
                meta.record_size += sizeof(double);
                break;
            case DataType::BOOL:
                meta.record_size += sizeof(bool);
                break;
            case DataType::VARCHAR:
            case DataType::TEXT:
                // Для VARCHAR/TEXT храним: int (длина) + данные
                meta.record_size += sizeof(int);  // Длина строки
                if (col.size > 0) {
                    meta.record_size += col.size;  // Фиксированный размер для VARCHAR
                } else {
                    meta.record_size += 256;  // По умолчанию для TEXT
                }
                break;
            case DataType::DATE:
                meta.record_size += sizeof(int64_t);  // Храним как timestamp
                break;
        }
    }
    
    // Сохраняем метаданные
    tables_cache_[meta.name] = meta;
    
    // Сохраняем в файл
    // Сначала читаем существующие метаданные
    std::ifstream in(metadata_file_, std::ios::binary);
    std::vector<TableMetadata> all_metadata;
    if (in.is_open()) {
        int count;
        in.read(reinterpret_cast<char*>(&count), sizeof(int));
        for (int i = 0; i < count; ++i) {
            TableMetadata existing = deserializeMetadata(in);
            if (existing.name != meta.name) {  // Пропускаем текущую таблицу если она уже есть
                all_metadata.push_back(existing);
            }
        }
        in.close();
    }
    
    // Добавляем новую таблицу
    all_metadata.push_back(meta);
    
    // Перезаписываем файл
    std::ofstream out(metadata_file_, std::ios::binary);
    int count = all_metadata.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(int));
    for (const auto& m : all_metadata) {
        serializeMetadata(m, out);
    }
    out.close();
    
    // Создаем файл данных
    std::ofstream data_file(getTableDataFile(meta.name), std::ios::binary);
    data_file.close();
    
    return true;
}

bool StorageManager::dropTable(const std::string& table_name) {
    if (!tableExists(table_name)) {
        return false;
    }
    
    // Удаляем из кэша
    tables_cache_.erase(table_name);
    
    // Удаляем файлы
    std::remove(getTableDataFile(table_name).c_str());
    std::remove(getTableMetadataFile(table_name).c_str());
    
    // Пересохраняем метаданные
    std::ofstream out(metadata_file_, std::ios::binary);
    int count = tables_cache_.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(int));
    for (const auto& pair : tables_cache_) {
        serializeMetadata(pair.second, out);
    }
    out.close();
    
    return true;
}

bool StorageManager::tableExists(const std::string& table_name) const {
    return tables_cache_.find(table_name) != tables_cache_.end();
}

TableMetadata StorageManager::getTableMetadata(const std::string& table_name) const {
    auto it = tables_cache_.find(table_name);
    if (it != tables_cache_.end()) {
        return it->second;
    }
    return TableMetadata();
}

std::vector<std::string> StorageManager::getAllTableNames() const {
    std::vector<std::string> names;
    for (const auto& pair : tables_cache_) {
        names.push_back(pair.first);
    }
    return names;
}

void StorageManager::serializeMetadata(const TableMetadata& metadata, std::ofstream& out) const {
    // Имя таблицы
    int name_len = metadata.name.length();
    out.write(reinterpret_cast<const char*>(&name_len), sizeof(int));
    out.write(metadata.name.c_str(), name_len);
    
    // Количество колонок
    int col_count = metadata.columns.size();
    out.write(reinterpret_cast<const char*>(&col_count), sizeof(int));
    
    // Колонки
    for (const auto& col : metadata.columns) {
        int col_name_len = col.name.length();
        out.write(reinterpret_cast<const char*>(&col_name_len), sizeof(int));
        out.write(col.name.c_str(), col_name_len);
        
        int type = static_cast<int>(col.type);
        out.write(reinterpret_cast<const char*>(&type), sizeof(int));
        out.write(reinterpret_cast<const char*>(&col.size), sizeof(int));
        out.write(reinterpret_cast<const char*>(&col.is_primary_key), sizeof(bool));
        out.write(reinterpret_cast<const char*>(&col.is_not_null), sizeof(bool));
        out.write(reinterpret_cast<const char*>(&col.is_unique), sizeof(bool));
        out.write(reinterpret_cast<const char*>(&col.offset), sizeof(int));
        
        int default_len = col.default_value.length();
        out.write(reinterpret_cast<const char*>(&default_len), sizeof(int));
        if (default_len > 0) {
            out.write(col.default_value.c_str(), default_len);
        }
    }
    
    // Размер записи и счетчики
    out.write(reinterpret_cast<const char*>(&metadata.record_size), sizeof(int));
    out.write(reinterpret_cast<const char*>(&metadata.record_count), sizeof(int64_t));
    out.write(reinterpret_cast<const char*>(&metadata.primary_key_counter), sizeof(int64_t));
}

TableMetadata StorageManager::deserializeMetadata(std::ifstream& in) const {
    TableMetadata metadata;
    
    // Имя таблицы
    int name_len;
    in.read(reinterpret_cast<char*>(&name_len), sizeof(int));
    char* name_buf = new char[name_len + 1];
    in.read(name_buf, name_len);
    name_buf[name_len] = '\0';
    metadata.name = name_buf;
    delete[] name_buf;
    
    // Количество колонок
    int col_count;
    in.read(reinterpret_cast<char*>(&col_count), sizeof(int));
    
    // Колонки
    for (int i = 0; i < col_count; ++i) {
        Column col;
        
        int col_name_len;
        in.read(reinterpret_cast<char*>(&col_name_len), sizeof(int));
        char* col_name_buf = new char[col_name_len + 1];
        in.read(col_name_buf, col_name_len);
        col_name_buf[col_name_len] = '\0';
        col.name = col_name_buf;
        delete[] col_name_buf;
        
        int type;
        in.read(reinterpret_cast<char*>(&type), sizeof(int));
        col.type = static_cast<DataType>(type);
        in.read(reinterpret_cast<char*>(&col.size), sizeof(int));
        in.read(reinterpret_cast<char*>(&col.is_primary_key), sizeof(bool));
        in.read(reinterpret_cast<char*>(&col.is_not_null), sizeof(bool));
        in.read(reinterpret_cast<char*>(&col.is_unique), sizeof(bool));
        in.read(reinterpret_cast<char*>(&col.offset), sizeof(int));
        
        int default_len;
        in.read(reinterpret_cast<char*>(&default_len), sizeof(int));
        if (default_len > 0) {
            char* default_buf = new char[default_len + 1];
            in.read(default_buf, default_len);
            default_buf[default_len] = '\0';
            col.default_value = default_buf;
            delete[] default_buf;
        }
        
        metadata.columns.push_back(col);
    }
    
    // Размер записи и счетчики
    in.read(reinterpret_cast<char*>(&metadata.record_size), sizeof(int));
    in.read(reinterpret_cast<char*>(&metadata.record_count), sizeof(int64_t));
    in.read(reinterpret_cast<char*>(&metadata.primary_key_counter), sizeof(int64_t));
    
    return metadata;
}

void StorageManager::serializeRecord(const Record& record, const TableMetadata& metadata, std::ofstream& out) const {
    for (const auto& col : metadata.columns) {
        auto it = record.values.find(col.name);
        if (it != record.values.end()) {
            serializeCellValue(it->second, col.type, col.size, out);
        } else {
            // Записываем значение по умолчанию или NULL
            CellValue default_val;
            if (!col.default_value.empty()) {
                default_val = parseValue(col.default_value, col.type);
            } else {
                default_val.is_null = true;
            }
            serializeCellValue(default_val, col.type, col.size, out);
        }
    }
}

Record StorageManager::deserializeRecord(std::ifstream& in, const TableMetadata& metadata) const {
    Record record;
    for (const auto& col : metadata.columns) {
        CellValue value = deserializeCellValue(in, col.type, col.size);
        record.values[col.name] = value;
    }
    return record;
}

void StorageManager::serializeCellValue(const CellValue& value, DataType type, int col_size, std::ofstream& out) const {
    out.write(reinterpret_cast<const char*>(&value.is_null), sizeof(bool));
    
    if (value.is_null) {
        // Для NULL записываем нули в зависимости от типа
        switch (type) {
            case DataType::INT:
            case DataType::DATE:
                {
                    int64_t zero = 0;
                    out.write(reinterpret_cast<const char*>(&zero), sizeof(int64_t));
                }
                break;
            case DataType::FLOAT:
                {
                    float zero = 0.0f;
                    out.write(reinterpret_cast<const char*>(&zero), sizeof(float));
                }
                break;
            case DataType::DOUBLE:
                {
                    double zero = 0.0;
                    out.write(reinterpret_cast<const char*>(&zero), sizeof(double));
                }
                break;
            case DataType::BOOL:
                {
                    bool zero = false;
                    out.write(reinterpret_cast<const char*>(&zero), sizeof(bool));
                }
                break;
            case DataType::VARCHAR:
            case DataType::TEXT:
                {
                    // Записываем длину 0
                    int len = 0;
                    out.write(reinterpret_cast<const char*>(&len), sizeof(int));
                    // Дополняем нулями до фиксированного размера
                    int fixed_size = (col_size > 0) ? col_size : 256;
                    char* zeros = new char[fixed_size];
                    memset(zeros, 0, fixed_size);
                    out.write(zeros, fixed_size);
                    delete[] zeros;
                }
                break;
        }
        return;
    }
    
    switch (type) {
        case DataType::INT:
        case DataType::DATE:
            out.write(reinterpret_cast<const char*>(&value.int_value), sizeof(int64_t));
            break;
        case DataType::FLOAT:
            {
                float f = static_cast<float>(value.double_value);
                out.write(reinterpret_cast<const char*>(&f), sizeof(float));
            }
            break;
        case DataType::DOUBLE:
            out.write(reinterpret_cast<const char*>(&value.double_value), sizeof(double));
            break;
        case DataType::BOOL:
            out.write(reinterpret_cast<const char*>(&value.bool_value), sizeof(bool));
            break;
        case DataType::VARCHAR:
        case DataType::TEXT:
            {
                int len = value.string_value.length();
                out.write(reinterpret_cast<const char*>(&len), sizeof(int));
                int fixed_size = (col_size > 0) ? col_size : 256;
                // Записываем строку
                out.write(value.string_value.c_str(), std::min(len, fixed_size));
                // Дополняем нулями до фиксированного размера
                int padding = fixed_size - std::min(len, fixed_size);
                if (padding > 0) {
                    char* zeros = new char[padding];
                    memset(zeros, 0, padding);
                    out.write(zeros, padding);
                    delete[] zeros;
                }
            }
            break;
    }
}

CellValue StorageManager::deserializeCellValue(std::ifstream& in, DataType type, int col_size) const {
    CellValue value;
    value.type = type;
    
    in.read(reinterpret_cast<char*>(&value.is_null), sizeof(bool));
    
    if (value.is_null) {
        // Пропускаем данные для NULL
        switch (type) {
            case DataType::INT:
            case DataType::DATE:
                {
                    int64_t dummy;
                    in.read(reinterpret_cast<char*>(&dummy), sizeof(int64_t));
                }
                break;
            case DataType::FLOAT:
                {
                    float dummy;
                    in.read(reinterpret_cast<char*>(&dummy), sizeof(float));
                }
                break;
            case DataType::DOUBLE:
                {
                    double dummy;
                    in.read(reinterpret_cast<char*>(&dummy), sizeof(double));
                }
                break;
            case DataType::BOOL:
                {
                    bool dummy;
                    in.read(reinterpret_cast<char*>(&dummy), sizeof(bool));
                }
                break;
            case DataType::VARCHAR:
            case DataType::TEXT:
                {
                    int len;
                    in.read(reinterpret_cast<char*>(&len), sizeof(int));
                    int fixed_size = (col_size > 0) ? col_size : 256;
                    // Пропускаем фиксированный размер данных
                    char* dummy_buf = new char[fixed_size];
                    in.read(dummy_buf, fixed_size);
                    delete[] dummy_buf;
                }
                break;
        }
        return value;
    }
    
    switch (type) {
        case DataType::INT:
        case DataType::DATE:
            in.read(reinterpret_cast<char*>(&value.int_value), sizeof(int64_t));
            break;
        case DataType::FLOAT:
            {
                float f;
                in.read(reinterpret_cast<char*>(&f), sizeof(float));
                value.double_value = f;
            }
            break;
        case DataType::DOUBLE:
            in.read(reinterpret_cast<char*>(&value.double_value), sizeof(double));
            break;
        case DataType::BOOL:
            in.read(reinterpret_cast<char*>(&value.bool_value), sizeof(bool));
            break;
        case DataType::VARCHAR:
        case DataType::TEXT:
            {
                int len;
                in.read(reinterpret_cast<char*>(&len), sizeof(int));
                int fixed_size = (col_size > 0) ? col_size : 256;
                // Читаем строку (максимум fixed_size байт)
                char* buf = new char[fixed_size + 1];
                int read_size = std::min(len, fixed_size);
                in.read(buf, read_size);
                buf[read_size] = '\0';
                value.string_value = buf;
                // Пропускаем padding если есть
                int padding = fixed_size - read_size;
                if (padding > 0) {
                    char* dummy = new char[padding];
                    in.read(dummy, padding);
                    delete[] dummy;
                }
                delete[] buf;
            }
            break;
    }
    
    return value;
}

CellValue StorageManager::parseValue(const std::string& str, DataType type) const {
    CellValue value;
    value.type = type;
    value.is_null = false;
    
    switch (type) {
        case DataType::INT:
            value.int_value = std::stoll(str);
            break;
        case DataType::FLOAT:
        case DataType::DOUBLE:
            value.double_value = std::stod(str);
            break;
        case DataType::BOOL:
            value.bool_value = (str == "true" || str == "1");
            break;
        case DataType::VARCHAR:
        case DataType::TEXT:
        case DataType::DATE:
            value.string_value = str;
            break;
    }
    
    return value;
}

bool StorageManager::insertRecord(const std::string& table_name, const Record& record) {
    if (!tableExists(table_name)) {
        return false;
    }
    
    TableMetadata metadata = getTableMetadata(table_name);
    Record final_record = record;  // Копия для модификации
    
    // Обработка PRIMARY KEY
    for (auto& col : metadata.columns) {
        if (col.is_primary_key) {
            auto it = final_record.values.find(col.name);
            
            // Если значение не указано или NULL, генерируем автоинкремент
            if (it == final_record.values.end() || it->second.is_null) {
                if (col.type == DataType::INT) {
                    metadata.primary_key_counter++;
                    CellValue pk_value;
                    pk_value.type = DataType::INT;
                    pk_value.is_null = false;
                    pk_value.int_value = metadata.primary_key_counter;
                    final_record.values[col.name] = pk_value;
                } else {
                    return false;  // PRIMARY KEY должен быть указан для не-INT типов
                }
            } else {
                // Проверяем, что PRIMARY KEY не NULL
                if (it->second.is_null) {
                    return false;  // PRIMARY KEY не может быть NULL
                }
                
                // Проверяем уникальность PRIMARY KEY
                if (!checkUniqueConstraint(table_name, col.name, it->second)) {
                    return false;  // Нарушение уникальности PRIMARY KEY
                }
            }
        }
    }
    
    // Проверяем ограничения NOT NULL для PRIMARY KEY
    for (const auto& col : metadata.columns) {
        if (col.is_primary_key || col.is_not_null) {
            auto it = final_record.values.find(col.name);
            if (it == final_record.values.end() || it->second.is_null) {
                if (col.is_primary_key && col.type == DataType::INT) {
                    // Уже обработано выше (автоинкремент)
                } else {
                    return false;  // PRIMARY KEY или NOT NULL колонка не может быть NULL
                }
            }
        }
    }
    
    // Проверяем ограничения UNIQUE
    for (const auto& col : metadata.columns) {
        if (col.is_unique) {
            auto it = final_record.values.find(col.name);
            if (it != final_record.values.end() && !it->second.is_null) {
                if (!checkUniqueConstraint(table_name, col.name, it->second)) {
                    return false;  // Нарушение UNIQUE
                }
            }
        }
    }
    
    // Проверяем внешние ключи
    for (const auto& fk : foreign_keys_) {
        if (fk.table_name == table_name) {
            auto it = final_record.values.find(fk.column_name);
            if (it != final_record.values.end() && !it->second.is_null) {
                if (!checkForeignKey(table_name, fk.column_name, it->second)) {
                    return false;  // Нарушение FOREIGN KEY
                }
            }
        }
    }
    
    // Записываем в файл
    std::ofstream out(getTableDataFile(table_name), std::ios::binary | std::ios::app);
    if (!out.is_open()) {
        return false;
    }
    
    serializeRecord(final_record, metadata, out);
    out.close();
    
    // Обновляем счетчик записей и primary_key_counter
    metadata.record_count++;
    tables_cache_[table_name] = metadata;
    
    // Обновляем метаданные в файле
    std::ofstream meta_out(metadata_file_, std::ios::binary);
    int count = tables_cache_.size();
    meta_out.write(reinterpret_cast<const char*>(&count), sizeof(int));
    for (const auto& pair : tables_cache_) {
        serializeMetadata(pair.second, meta_out);
    }
    meta_out.close();
    
    return true;
}

std::vector<Record> StorageManager::selectRecords(const std::string& table_name) const {
    std::vector<Record> records;
    
    if (!tableExists(table_name)) {
        return records;
    }
    
    TableMetadata metadata = getTableMetadata(table_name);
    std::ifstream in(getTableDataFile(table_name), std::ios::binary);
    
    if (!in.is_open()) {
        return records;
    }
    
    for (int64_t i = 0; i < metadata.record_count; ++i) {
        Record record = deserializeRecord(in, metadata);
        records.push_back(record);
    }
    
    in.close();
    return records;
}

bool StorageManager::addUniqueConstraint(const UniqueConstraint& constraint) {
    unique_constraints_.push_back(constraint);
    saveUniqueConstraints();
    return true;
}

bool StorageManager::addForeignKey(const ForeignKey& fk) {
    foreign_keys_.push_back(fk);
    saveForeignKeys();
    return true;
}

bool StorageManager::checkUniqueConstraint(const std::string& table_name, const std::string& column_name, const CellValue& value) const {
    // Проверяем все записи в таблице
    std::vector<Record> records = selectRecords(table_name);
    for (const auto& record : records) {
        auto it = record.values.find(column_name);
        if (it != record.values.end()) {
            if (compareCellValues(it->second, value)) {
                return false;  // Дубликат найден
            }
        }
    }
    return true;
}

bool StorageManager::checkPrimaryKeyConstraint(const std::string& table_name, const std::string& column_name, const CellValue& value) const {
    // PRIMARY KEY должен быть уникальным и не NULL
    if (value.is_null) {
        return false;
    }
    return checkUniqueConstraint(table_name, column_name, value);
}

bool StorageManager::checkForeignKey(const std::string& table_name, const std::string& column_name, const CellValue& value) const {
    // Находим соответствующий внешний ключ
    for (const auto& fk : foreign_keys_) {
        if (fk.table_name == table_name && fk.column_name == column_name) {
            // Проверяем существование значения в ссылающейся таблице
            std::vector<Record> ref_records = selectRecords(fk.ref_table_name);
            for (const auto& record : ref_records) {
                auto it = record.values.find(fk.ref_column_name);
                if (it != record.values.end()) {
                    if (compareCellValues(it->second, value)) {
                        return true;  // Значение найдено
                    }
                }
            }
            return false;  // Значение не найдено
        }
    }
    return true;  // Нет ограничения
}

bool StorageManager::compareCellValues(const CellValue& v1, const CellValue& v2) const {
    if (v1.type != v2.type) return false;
    if (v1.is_null && v2.is_null) return true;
    if (v1.is_null || v2.is_null) return false;
    
    switch (v1.type) {
        case DataType::INT:
        case DataType::DATE:
            return v1.int_value == v2.int_value;
        case DataType::FLOAT:
        case DataType::DOUBLE:
            return v1.double_value == v2.double_value;
        case DataType::BOOL:
            return v1.bool_value == v2.bool_value;
        case DataType::VARCHAR:
        case DataType::TEXT:
            return v1.string_value == v2.string_value;
    }
    return false;
}

void StorageManager::saveUniqueConstraints() const {
    std::ofstream out(getConstraintsFile(), std::ios::binary);
    if (!out.is_open()) return;
    
    int count = unique_constraints_.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(int));
    
    for (const auto& constraint : unique_constraints_) {
        int len = constraint.table_name.length();
        out.write(reinterpret_cast<const char*>(&len), sizeof(int));
        out.write(constraint.table_name.c_str(), len);
        
        len = constraint.column_name.length();
        out.write(reinterpret_cast<const char*>(&len), sizeof(int));
        out.write(constraint.column_name.c_str(), len);
        
        len = constraint.constraint_name.length();
        out.write(reinterpret_cast<const char*>(&len), sizeof(int));
        out.write(constraint.constraint_name.c_str(), len);
    }
    
    out.close();
}

void StorageManager::loadUniqueConstraints() const {
    unique_constraints_.clear();
    std::ifstream in(getConstraintsFile(), std::ios::binary);
    if (!in.is_open()) {
        constraints_loaded_ = true;
        return;
    }
    
    int count;
    in.read(reinterpret_cast<char*>(&count), sizeof(int));
    
    for (int i = 0; i < count; ++i) {
        UniqueConstraint constraint;
        
        int len;
        in.read(reinterpret_cast<char*>(&len), sizeof(int));
        char* buf = new char[len + 1];
        in.read(buf, len);
        buf[len] = '\0';
        constraint.table_name = buf;
        delete[] buf;
        
        in.read(reinterpret_cast<char*>(&len), sizeof(int));
        buf = new char[len + 1];
        in.read(buf, len);
        buf[len] = '\0';
        constraint.column_name = buf;
        delete[] buf;
        
        in.read(reinterpret_cast<char*>(&len), sizeof(int));
        buf = new char[len + 1];
        in.read(buf, len);
        buf[len] = '\0';
        constraint.constraint_name = buf;
        delete[] buf;
        
        unique_constraints_.push_back(constraint);
    }
    
    in.close();
    constraints_loaded_ = true;
}

void StorageManager::saveForeignKeys() const {
    std::ofstream out(getForeignKeysFile(), std::ios::binary);
    if (!out.is_open()) return;
    
    int count = foreign_keys_.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(int));
    
    for (const auto& fk : foreign_keys_) {
        int len = fk.table_name.length();
        out.write(reinterpret_cast<const char*>(&len), sizeof(int));
        out.write(fk.table_name.c_str(), len);
        
        len = fk.column_name.length();
        out.write(reinterpret_cast<const char*>(&len), sizeof(int));
        out.write(fk.column_name.c_str(), len);
        
        len = fk.ref_table_name.length();
        out.write(reinterpret_cast<const char*>(&len), sizeof(int));
        out.write(fk.ref_table_name.c_str(), len);
        
        len = fk.ref_column_name.length();
        out.write(reinterpret_cast<const char*>(&len), sizeof(int));
        out.write(fk.ref_column_name.c_str(), len);
        
        len = fk.constraint_name.length();
        out.write(reinterpret_cast<const char*>(&len), sizeof(int));
        out.write(fk.constraint_name.c_str(), len);
    }
    
    out.close();
}

void StorageManager::loadForeignKeys() const {
    foreign_keys_.clear();
    std::ifstream in(getForeignKeysFile(), std::ios::binary);
    if (!in.is_open()) {
        return;
    }
    
    int count;
    in.read(reinterpret_cast<char*>(&count), sizeof(int));
    
    for (int i = 0; i < count; ++i) {
        ForeignKey fk;
        
        int len;
        in.read(reinterpret_cast<char*>(&len), sizeof(int));
        char* buf = new char[len + 1];
        in.read(buf, len);
        buf[len] = '\0';
        fk.table_name = buf;
        delete[] buf;
        
        in.read(reinterpret_cast<char*>(&len), sizeof(int));
        buf = new char[len + 1];
        in.read(buf, len);
        buf[len] = '\0';
        fk.column_name = buf;
        delete[] buf;
        
        in.read(reinterpret_cast<char*>(&len), sizeof(int));
        buf = new char[len + 1];
        in.read(buf, len);
        buf[len] = '\0';
        fk.ref_table_name = buf;
        delete[] buf;
        
        in.read(reinterpret_cast<char*>(&len), sizeof(int));
        buf = new char[len + 1];
        in.read(buf, len);
        buf[len] = '\0';
        fk.ref_column_name = buf;
        delete[] buf;
        
        in.read(reinterpret_cast<char*>(&len), sizeof(int));
        buf = new char[len + 1];
        in.read(buf, len);
        buf[len] = '\0';
        fk.constraint_name = buf;
        delete[] buf;
        
        foreign_keys_.push_back(fk);
    }
    
    in.close();
}

std::vector<UniqueConstraint> StorageManager::getUniqueConstraints(const std::string& table_name) const {
    std::vector<UniqueConstraint> result;
    for (const auto& constraint : unique_constraints_) {
        if (constraint.table_name == table_name) {
            result.push_back(constraint);
        }
    }
    return result;
}

std::vector<ForeignKey> StorageManager::getForeignKeys(const std::string& table_name) const {
    std::vector<ForeignKey> result;
    for (const auto& fk : foreign_keys_) {
        if (fk.table_name == table_name) {
            result.push_back(fk);
        }
    }
    return result;
}

bool StorageManager::updateRecord(const std::string& table_name, int64_t record_id, const Record& record) {
    // Упрощенная версия - в реальности нужно перезаписать запись по ID
    return false;
}

bool StorageManager::deleteRecord(const std::string& table_name, int64_t record_id) {
    // Упрощенная версия - в реальности нужно удалить запись по ID
    return false;
}

bool StorageManager::deleteRecords(const std::string& table_name, const std::vector<WhereCondition>& conditions) {
    if (!tableExists(table_name)) {
        return false;
    }

    TableMetadata metadata = getTableMetadata(table_name);
    std::string data_file = getTableDataFile(table_name);

    // Читаем ВЕСЬ файл целиком (без зависимости от record_count)
    std::ifstream in(data_file, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        return false;
    }

    std::streamsize file_size = in.tellg();
    in.seekg(0, std::ios::beg);

    std::vector<Record> all_records;

    while (in.tellg() < file_size) {
        Record record = deserializeRecord(in, metadata);
        all_records.push_back(record);
    }
    in.close();

    // Фильтруем: оставляем только те, которые НЕ соответствуют условиям
    std::vector<Record> remaining_records;
    for (const auto& record : all_records) {
        bool matches_all_conditions = true;

        for (const auto& cond : conditions) {
            auto it = record.values.find(cond.column);
            if (it == record.values.end()) {
                matches_all_conditions = false;
                break;
            }

            if (!evaluateCondition(it->second, cond)) {
                matches_all_conditions = false;
                break;
            }
        }

        if (!matches_all_conditions) {
            remaining_records.push_back(record); // оставляем, если НЕ подходит под условия
        }
    }

    if (remaining_records.size() == all_records.size()) {
        return true; // ничего не удалили
    }

    // Перезаписываем файл с оставшимися записями
    std::ofstream out(data_file, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }

    for (const auto& rec : remaining_records) {
        serializeRecord(rec, metadata, out);
    }
    out.close();

    // Обновляем счетчик записей
    metadata.record_count = remaining_records.size();
    tables_cache_[table_name] = metadata;

    // Обновляем метаданные в файле
    std::ofstream meta_out(metadata_file_, std::ios::binary);
    int count = tables_cache_.size();
    meta_out.write(reinterpret_cast<const char*>(&count), sizeof(int));
    for (const auto& pair : tables_cache_) {
        serializeMetadata(pair.second, meta_out);
    }
    meta_out.close();

    std::cout << "Deleted " << (all_records.size() - remaining_records.size()) << " records from " << table_name << ".\n";
    return true;
}

bool StorageManager::evaluateCondition(const CellValue& value, const WhereCondition& cond) const {
    if (value.is_null) {
        return false; // NULL не соответствует условиям
    }

    if (cond.is_string_value) {
        std::string val_str;
        switch (value.type) {
            case DataType::INT: val_str = std::to_string(value.int_value); break;
            case DataType::FLOAT:
            case DataType::DOUBLE: val_str = std::to_string(value.double_value); break;
            case DataType::BOOL: val_str = value.bool_value ? "true" : "false"; break;
            case DataType::VARCHAR:
            case DataType::TEXT:
            case DataType::DATE: val_str = value.string_value; break;
        }

        if (cond.operator_ == "=") return val_str == cond.value;
        if (cond.operator_ == "!=" || cond.operator_ == "<>") return val_str != cond.value;
        if (cond.operator_ == "LIKE") {
            // Простая поддержка LIKE: % в начале/конце
            if (cond.value.front() == '%' && cond.value.back() == '%') {
                return val_str.find(cond.value.substr(1, cond.value.length() - 2)) != std::string::npos;
            } else if (cond.value.back() == '%') {
                return val_str.substr(0, cond.value.length() - 1) == cond.value.substr(0, cond.value.length() - 1);
            } else if (cond.value.front() == '%') {
                return val_str.substr(val_str.length() - cond.value.length() + 1) == cond.value.substr(1);
            }
        }
    } else {
        // Сравнение чисел
        double val_dbl;
        switch (value.type) {
            case DataType::INT: val_dbl = static_cast<double>(value.int_value); break;
            case DataType::FLOAT:
            case DataType::DOUBLE: val_dbl = value.double_value; break;
            default: return false; // Не числовое значение
        }

        double cond_dbl = std::stod(cond.value);

        if (cond.operator_ == "=") return val_dbl == cond_dbl;
        if (cond.operator_ == "!=" || cond.operator_ == "<>") return val_dbl != cond_dbl;
        if (cond.operator_ == "<") return val_dbl < cond_dbl;
        if (cond.operator_ == ">") return val_dbl > cond_dbl;
        if (cond.operator_ == "<=") return val_dbl <= cond_dbl;
        if (cond.operator_ == ">=") return val_dbl >= cond_dbl;
    }

    return false;
}

Record StorageManager::getRecordById(const std::string& table_name, int64_t record_id) const {
    Record record;
    // Упрощенная версия
    return record;
}

