#ifndef DATABASE_TYPES_H
#define DATABASE_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

// Типы данных
enum class DataType {
    INT,
    VARCHAR,
    TEXT,
    DATE,
    FLOAT,
    DOUBLE,
    BOOL
};

// Структура колонки таблицы
struct Column {
    std::string name;
    DataType type;
    int size;              // Для VARCHAR
    bool is_primary_key;
    bool is_not_null;
    bool is_unique;
    std::string default_value;
    int offset;            // Смещение в записи (для бинарного формата)
    
    Column() : type(DataType::INT), size(-1), is_primary_key(false), 
               is_not_null(false), is_unique(false), offset(0) {}
};

// Метаданные таблицы
struct TableMetadata {
    std::string name;
    std::vector<Column> columns;
    int record_size;       // Размер одной записи в байтах
    int64_t record_count;  // Количество записей
    int64_t primary_key_counter;  // Счетчик для автоинкремента
    
    TableMetadata() : record_size(0), record_count(0), primary_key_counter(0) {}
};

// Ограничение UNIQUE
struct UniqueConstraint {
    std::string table_name;
    std::string column_name;
    std::string constraint_name;
    
    UniqueConstraint() {}
};

// Внешний ключ (FOREIGN KEY)
struct ForeignKey {
    std::string table_name;        // Таблица с внешним ключом
    std::string column_name;       // Колонка с внешним ключом
    std::string ref_table_name;    // Ссылающаяся таблица
    std::string ref_column_name;   // Ссылающаяся колонка
    std::string constraint_name;
    
    ForeignKey() {}
};

// Значение ячейки
struct CellValue {
    DataType type;
    std::string string_value;
    int64_t int_value;
    double double_value;
    bool bool_value;
    bool is_null;
    
    CellValue() : type(DataType::INT), int_value(0), double_value(0.0), 
                  bool_value(false), is_null(true) {}
};

// Запись (строка) таблицы
struct Record {
    std::map<std::string, CellValue> values;  // колонка -> значение
    
    Record() {}
};

#endif // DATABASE_TYPES_H

