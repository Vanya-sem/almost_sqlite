#ifndef SQL_PARSER_H
#define SQL_PARSER_H

#include "sql_query.h"
#include <string>

class SQLParser {
public:
    // Основной метод парсинга SQL запроса
    static ParsedQuery parse(const std::string& sql);
    
private:
    // Вспомогательные методы для токенизации
    static std::vector<std::string> tokenize(const std::string& sql);
    static std::string trim(const std::string& str);
    static std::string toUpper(const std::string& str);
    
    // Методы парсинга конкретных типов запросов
    static ParsedQuery parseSelect(const std::vector<std::string>& tokens, size_t& pos);
    static ParsedQuery parseInsert(const std::vector<std::string>& tokens, size_t& pos);
    static ParsedQuery parseUpdate(const std::vector<std::string>& tokens, size_t& pos);
    static ParsedQuery parseDelete(const std::vector<std::string>& tokens, size_t& pos);
    static ParsedQuery parseCreateTable(const std::vector<std::string>& tokens, size_t& pos);
    static ParsedQuery parseDropTable(const std::vector<std::string>& tokens, size_t& pos);
    
    // Вспомогательные методы парсинга
    static WhereCondition parseWhereCondition(const std::vector<std::string>& tokens, size_t& pos);
    static std::string parseValue(const std::vector<std::string>& tokens, size_t& pos);
    static ColumnDefinition parseColumnDefinition(const std::vector<std::string>& tokens, size_t& pos);
};

#endif // SQL_PARSER_H

