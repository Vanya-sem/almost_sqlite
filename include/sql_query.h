#ifndef SQL_QUERY_H
#define SQL_QUERY_H

#include <string>
#include <vector>
#include <map>

// Типы SQL запросов
enum class QueryType {
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    CREATE_TABLE,
    DROP_TABLE,
    UNKNOWN
};

// Условие WHERE
struct WhereCondition {
    std::string column;
    std::string operator_;  // =, <, >, <=, >=, !=, LIKE
    std::string value;
    bool is_string_value;  // true если значение в кавычках
};

// JOIN условие
struct JoinClause {
    std::string table;
    std::string join_type;  // INNER, LEFT, RIGHT, FULL
    std::string condition;
};

// Структура для SELECT запроса
struct SelectQuery {
    std::vector<std::string> columns;  // "*" или список колонок
    std::string table;
    std::vector<JoinClause> joins;
    std::vector<WhereCondition> where_conditions;
    std::string order_by;
    bool order_asc;  // true для ASC, false для DESC
    int limit;
    int offset;
    
    SelectQuery() : order_asc(true), limit(-1), offset(-1) {}
};

// Структура для INSERT запроса
struct InsertQuery {
    std::string table;
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> values;  // Множественные строки для вставки
    
    InsertQuery() {}
};

// Структура для UPDATE запроса
struct UpdateQuery {
    std::string table;
    std::map<std::string, std::string> set_values;  // колонка -> значение
    std::vector<WhereCondition> where_conditions;
    
    UpdateQuery() {}
};

// Структура для DELETE запроса
struct DeleteQuery {
    std::string table;
    std::vector<WhereCondition> where_conditions;
    
    DeleteQuery() {}
};

// Структура для CREATE TABLE запроса
struct ColumnDefinition {
    std::string name;
    std::string type;  // INT, VARCHAR, TEXT, DATE, etc.
    int size;  // Для VARCHAR и подобных
    bool is_primary_key;
    bool is_not_null;
    std::string default_value;
    
    ColumnDefinition() : size(-1), is_primary_key(false), is_not_null(false) {}
};

struct CreateTableQuery {
    std::string table_name;
    std::vector<ColumnDefinition> columns;
    
    CreateTableQuery() {}
};

// Структура для DROP TABLE запроса
struct DropTableQuery {
    std::string table_name;
    bool if_exists;  // DROP TABLE IF EXISTS
    
    DropTableQuery() : if_exists(false) {}
};

// Основная структура распарсенного запроса
struct ParsedQuery {
    QueryType type;
    
    // Указатели на конкретные типы запросов (только один будет не nullptr)
    SelectQuery* select;
    InsertQuery* insert;
    UpdateQuery* update;
    DeleteQuery* delete_;
    CreateTableQuery* create_table;
    DropTableQuery* drop_table;
    
    ParsedQuery() : type(QueryType::UNKNOWN), 
                    select(nullptr), insert(nullptr), update(nullptr),
                    delete_(nullptr), create_table(nullptr), drop_table(nullptr) {}
    
    ~ParsedQuery() {
        clear();
    }
    
    // Копирующий конструктор
    ParsedQuery(const ParsedQuery& other) : type(other.type),
        select(nullptr), insert(nullptr), update(nullptr),
        delete_(nullptr), create_table(nullptr), drop_table(nullptr) {
        copyFrom(other);
    }
    
    // Оператор присваивания
    ParsedQuery& operator=(const ParsedQuery& other) {
        if (this != &other) {
            clear();
            type = other.type;
            copyFrom(other);
        }
        return *this;
    }
    
    void clear() {
        delete select;
        delete insert;
        delete update;
        delete delete_;
        delete create_table;
        delete drop_table;
        
        select = nullptr;
        insert = nullptr;
        update = nullptr;
        delete_ = nullptr;
        create_table = nullptr;
        drop_table = nullptr;
        type = QueryType::UNKNOWN;
    }
    
private:
    void copyFrom(const ParsedQuery& other) {
        switch(other.type) {
            case QueryType::SELECT:
                if (other.select) {
                    select = new SelectQuery(*other.select);
                }
                break;
            case QueryType::INSERT:
                if (other.insert) {
                    insert = new InsertQuery(*other.insert);
                }
                break;
            case QueryType::UPDATE:
                if (other.update) {
                    update = new UpdateQuery(*other.update);
                }
                break;
            case QueryType::DELETE:
                if (other.delete_) {
                    delete_ = new DeleteQuery(*other.delete_);
                }
                break;
            case QueryType::CREATE_TABLE:
                if (other.create_table) {
                    create_table = new CreateTableQuery(*other.create_table);
                }
                break;
            case QueryType::DROP_TABLE:
                if (other.drop_table) {
                    drop_table = new DropTableQuery(*other.drop_table);
                }
                break;
            default:
                break;
        }
    }
};

#endif // SQL_QUERY_H

