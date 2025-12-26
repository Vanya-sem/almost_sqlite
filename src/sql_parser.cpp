#include "../include/sql_parser.h"
#include <sstream>
#include <algorithm>
#include <cctype>

// Триминг строки
std::string SQLParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

// Преобразование в верхний регистр
std::string SQLParser::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// Токенизация SQL запроса
std::vector<std::string> SQLParser::tokenize(const std::string& sql) {
    std::vector<std::string> tokens;
    std::string current_token;
    bool in_quotes = false;
    bool in_single_quotes = false;
    char quote_char = 0;
    
    for (size_t i = 0; i < sql.length(); ++i) {
        char c = sql[i];
        
        // Обработка кавычек
        if ((c == '"' || c == '\'') && (i == 0 || sql[i-1] != '\\')) {
            if (!in_quotes && !in_single_quotes) {
                in_quotes = (c == '"');
                in_single_quotes = (c == '\'');
                quote_char = c;
                current_token += c;
            } else if ((in_quotes && c == '"') || (in_single_quotes && c == '\'')) {
                in_quotes = false;
                in_single_quotes = false;
                current_token += c;
            } else {
                current_token += c;
            }
            continue;
        }
        
        // Если внутри кавычек, добавляем символ как есть
        if (in_quotes || in_single_quotes) {
            current_token += c;
            continue;
        }
        
        // Разделители
        if (std::isspace(c) || c == ',' || c == '(' || c == ')' || c == ';') {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
            if (c == '(' || c == ')' || c == ';' || c == ',') {
                tokens.push_back(std::string(1, c));
            }
        } else {
            current_token += c;
        }
    }
    
    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }
    
    return tokens;
}

// Основной метод парсинга
ParsedQuery SQLParser::parse(const std::string& sql) {
    ParsedQuery query;
    std::vector<std::string> tokens = tokenize(sql);
    
    if (tokens.empty()) {
        return query;
    }
    
    size_t pos = 0;
    std::string first_token = toUpper(trim(tokens[0]));
    
    if (first_token == "SELECT") {
        query = parseSelect(tokens, pos);
    } else if (first_token == "INSERT") {
        query = parseInsert(tokens, pos);
    } else if (first_token == "UPDATE") {
        query = parseUpdate(tokens, pos);
    } else if (first_token == "DELETE") {
        query = parseDelete(tokens, pos);
    } else if (first_token == "CREATE") {
        if (pos + 1 < tokens.size() && toUpper(tokens[pos + 1]) == "TABLE") {
            pos += 2;
            query = parseCreateTable(tokens, pos);
        }
    } else if (first_token == "DROP") {
        if (pos + 1 < tokens.size() && toUpper(tokens[pos + 1]) == "TABLE") {
            pos += 2;
            query = parseDropTable(tokens, pos);
        }
    }
    
    return query;
}

// Парсинг SELECT запроса
ParsedQuery SQLParser::parseSelect(const std::vector<std::string>& tokens, size_t& pos) {
    ParsedQuery query;
    query.type = QueryType::SELECT;
    query.select = new SelectQuery();
    
    pos++; // Пропускаем SELECT
    
    // Парсинг колонок
    while (pos < tokens.size() && toUpper(tokens[pos]) != "FROM") {
        std::string token = trim(tokens[pos]);
        if (token != "," && token != "(" && token != ")") {
            query.select->columns.push_back(token);
        }
        pos++;
    }
    
    if (query.select->columns.empty()) {
        query.select->columns.push_back("*");
    }
    
    // Парсинг FROM
    if (pos < tokens.size() && toUpper(tokens[pos]) == "FROM") {
        pos++;
        if (pos < tokens.size()) {
            query.select->table = tokens[pos];
            pos++;
        }
    }
    
    // Парсинг JOIN
    while (pos < tokens.size()) {
        std::string token = toUpper(trim(tokens[pos]));
        if (token == "INNER" || token == "LEFT" || token == "RIGHT" || token == "FULL") {
            JoinClause join;
            join.join_type = token;
            pos++;
            if (pos < tokens.size() && toUpper(tokens[pos]) == "JOIN") {
                pos++;
                if (pos < tokens.size()) {
                    join.table = tokens[pos];
                    pos++;
                }
                if (pos < tokens.size() && toUpper(tokens[pos]) == "ON") {
                    pos++;
                    // Собираем условие ON
                    std::string condition;
                    while (pos < tokens.size() && toUpper(tokens[pos]) != "WHERE" && 
                           toUpper(tokens[pos]) != "ORDER" && toUpper(tokens[pos]) != "LIMIT") {
                        condition += tokens[pos] + " ";
                        pos++;
                    }
                    join.condition = trim(condition);
                }
                query.select->joins.push_back(join);
            }
        } else {
            break;
        }
    }
    
    // Парсинг WHERE
    if (pos < tokens.size() && toUpper(tokens[pos]) == "WHERE") {
        pos++;
        while (pos < tokens.size()) {
            std::string token = toUpper(trim(tokens[pos]));
            if (token == "ORDER" || token == "LIMIT") break;
            
            if (token == "AND" || token == "OR") {
                pos++;
                continue;
            }
            
            WhereCondition condition = parseWhereCondition(tokens, pos);
            query.select->where_conditions.push_back(condition);
        }
    }
    
    // Парсинг ORDER BY
    if (pos < tokens.size() && toUpper(tokens[pos]) == "ORDER") {
        pos++;
        if (pos < tokens.size() && toUpper(tokens[pos]) == "BY") {
            pos++;
            if (pos < tokens.size()) {
                query.select->order_by = tokens[pos];
                pos++;
                if (pos < tokens.size()) {
                    std::string order_dir = toUpper(tokens[pos]);
                    if (order_dir == "ASC") {
                        query.select->order_asc = true;
                        pos++;
                    } else if (order_dir == "DESC") {
                        query.select->order_asc = false;
                        pos++;
                    }
                }
            }
        }
    }
    
    // Парсинг LIMIT
    if (pos < tokens.size() && toUpper(tokens[pos]) == "LIMIT") {
        pos++;
        if (pos < tokens.size()) {
            query.select->limit = std::stoi(tokens[pos]);
            pos++;
            if (pos < tokens.size() && toUpper(tokens[pos]) == "OFFSET") {
                pos++;
                if (pos < tokens.size()) {
                    query.select->offset = std::stoi(tokens[pos]);
                }
            }
        }
    }
    
    return query;
}

// Парсинг INSERT запроса
ParsedQuery SQLParser::parseInsert(const std::vector<std::string>& tokens, size_t& pos) {
    ParsedQuery query;
    query.type = QueryType::INSERT;
    query.insert = new InsertQuery();
    
    pos++; // Пропускаем INSERT
    
    // Пропускаем INTO если есть
    if (pos < tokens.size() && toUpper(tokens[pos]) == "INTO") {
        pos++;
    }
    
    // Имя таблицы
    if (pos < tokens.size()) {
        query.insert->table = tokens[pos];
        pos++;
    }
    
    // Парсинг колонок
    if (pos < tokens.size() && tokens[pos] == "(") {
        pos++; // Пропускаем (
        while (pos < tokens.size() && tokens[pos] != ")") {
            std::string token = trim(tokens[pos]);
            if (token != "," && token != "(") {
                query.insert->columns.push_back(token);
            }
            pos++;
        }
        if (pos < tokens.size() && tokens[pos] == ")") {
            pos++;
        }
    }
    
    // Парсинг VALUES
    if (pos < tokens.size() && toUpper(tokens[pos]) == "VALUES") {
        pos++;
        while (pos < tokens.size()) {
            if (tokens[pos] == "(") {
                pos++;
                std::vector<std::string> row_values;
                while (pos < tokens.size() && tokens[pos] != ")") {
                    std::string token = trim(tokens[pos]);
                    if (token != "," && token != "(") {
                        row_values.push_back(token);
                    }
                    pos++;
                }
                if (pos < tokens.size() && tokens[pos] == ")") {
                    pos++;
                }
                if (!row_values.empty()) {
                    query.insert->values.push_back(row_values);
                }
            } else if (tokens[pos] == ",") {
                pos++;
            } else {
                break;
            }
        }
    }
    
    return query;
}

// Парсинг UPDATE запроса
ParsedQuery SQLParser::parseUpdate(const std::vector<std::string>& tokens, size_t& pos) {
    ParsedQuery query;
    query.type = QueryType::UPDATE;
    query.update = new UpdateQuery();
    
    pos++; // Пропускаем UPDATE
    
    // Имя таблицы
    if (pos < tokens.size()) {
        query.update->table = tokens[pos];
        pos++;
    }
    
    // Парсинг SET
    if (pos < tokens.size() && toUpper(tokens[pos]) == "SET") {
        pos++;
        while (pos < tokens.size() && toUpper(tokens[pos]) != "WHERE") {
            std::string column = tokens[pos];
            pos++;
            if (pos < tokens.size() && tokens[pos] == "=") {
                pos++;
                std::string value = parseValue(tokens, pos);
                query.update->set_values[column] = value;
            }
            if (pos < tokens.size() && tokens[pos] == ",") {
                pos++;
            }
        }
    }
    
    // Парсинг WHERE
    if (pos < tokens.size() && toUpper(tokens[pos]) == "WHERE") {
        pos++;
        while (pos < tokens.size()) {
            std::string token = toUpper(trim(tokens[pos]));
            if (token == "AND" || token == "OR") {
                pos++;
                continue;
            }
            WhereCondition condition = parseWhereCondition(tokens, pos);
            query.update->where_conditions.push_back(condition);
        }
    }
    
    return query;
}

// Парсинг DELETE запроса
ParsedQuery SQLParser::parseDelete(const std::vector<std::string>& tokens, size_t& pos) {
    ParsedQuery query;
    query.type = QueryType::DELETE;
    query.delete_ = new DeleteQuery();

    // 1. Читаем DELETE
    if (pos < tokens.size()) {
        std::string token = toUpper(trim(tokens[pos]));
        if (token == "DELETE") {
            pos++; //  читаем DELETE
        }
        else {
            // Ошибка: ожидался DELETE
            return query;
        }
    }
    else {
        // Ошибка: ожидался DELETE
        return query;
    }

    // 2. Читаем FROM если есть
    if (pos < tokens.size()) {
        std::string token = toUpper(trim(tokens[pos]));
        if (token == "FROM") {
            pos++; //  читаем FROM
        }
    }

    // 3. Читаем имя таблицы
    if (pos < tokens.size()) {
        std::string table_name = trim(tokens[pos]);
        query.delete_->table = table_name; // сохраняем имя таблицы
        pos++; // читаем имя таблицы
    }
    else {
        // Ошибка: ожидалось имя таблицы
        return query;
    }

    // 4. Парсинг WHERE
    if (pos < tokens.size()) {
        std::string token = toUpper(trim(tokens[pos]));
        if (token == "WHERE") {
            pos++; // читаем WHERE

            while (pos < tokens.size()) {
                std::string current_token = toUpper(trim(tokens[pos]));

                if (current_token == "AND") {
                    pos++; 
                    continue; 
                }
                else if (current_token == "OR") {
                    pos++; 
                    continue; 
                }

                if (current_token == "ORDER" || current_token == "LIMIT") {
                    break; 
                }

                WhereCondition condition = parseWhereCondition(tokens, pos);
                query.delete_->where_conditions.push_back(condition); 
            }
        }
    }

    return query; 
}

// Парсинг CREATE TABLE запроса
ParsedQuery SQLParser::parseCreateTable(const std::vector<std::string>& tokens, size_t& pos) {
    ParsedQuery query;
    query.type = QueryType::CREATE_TABLE;
    query.create_table = new CreateTableQuery();
    
    // Имя таблицы
    if (pos < tokens.size()) {
        query.create_table->table_name = tokens[pos];
        pos++;
    }
    
    // Парсинг определения колонок
    if (pos < tokens.size() && tokens[pos] == "(") {
        pos++; // Пропускаем (
        while (pos < tokens.size() && tokens[pos] != ")") {
            std::string token = trim(tokens[pos]);
            if (token == "," || token == "(") {
                pos++;
                continue;
            }
            
            ColumnDefinition col = parseColumnDefinition(tokens, pos);
            query.create_table->columns.push_back(col);
            
            if (pos < tokens.size() && tokens[pos] == ",") {
                pos++;
            }
        }
        if (pos < tokens.size() && tokens[pos] == ")") {
            pos++;
        }
    }
    
    return query;
}

// Парсинг DROP TABLE запроса
ParsedQuery SQLParser::parseDropTable(const std::vector<std::string>& tokens, size_t& pos) {
    ParsedQuery query;
    query.type = QueryType::DROP_TABLE;
    query.drop_table = new DropTableQuery();
    
    // Проверка на IF EXISTS
    if (pos < tokens.size() && toUpper(tokens[pos]) == "IF") {
        pos++;
        if (pos < tokens.size() && toUpper(tokens[pos]) == "EXISTS") {
            query.drop_table->if_exists = true;
            pos++;
        }
    }
    
    // Имя таблицы
    if (pos < tokens.size()) {
        query.drop_table->table_name = tokens[pos];
    }
    
    return query;
}

// Парсинг условия WHERE
WhereCondition SQLParser::parseWhereCondition(const std::vector<std::string>& tokens, size_t& pos) {
    WhereCondition condition;
    
    if (pos < tokens.size()) {
        condition.column = tokens[pos];
        pos++;
    }
    
    if (pos < tokens.size()) {
        std::string op = tokens[pos];
        if (op == "=" || op == "<" || op == ">" || op == "<=" || op == ">=" || op == "!=" || op == "<>") {
            condition.operator_ = op;
            pos++;
        } else if (toUpper(op) == "LIKE") {
            condition.operator_ = "LIKE";
            pos++;
        }
    }
    
    if (pos < tokens.size()) {
        std::string value = tokens[pos];
        condition.value = value;
        // Проверяем, является ли значение строкой
        if ((value.front() == '"' && value.back() == '"') || 
            (value.front() == '\'' && value.back() == '\'')) {
            condition.is_string_value = true;
            // Убираем кавычки
            condition.value = value.substr(1, value.length() - 2);
        } else {
            condition.is_string_value = false;
        }
        pos++;
    }
    
    return condition;
}

// Парсинг значения
std::string SQLParser::parseValue(const std::vector<std::string>& tokens, size_t& pos) {
    if (pos < tokens.size()) {
        std::string value = tokens[pos];
        pos++;
        return value;
    }
    return "";
}

// Парсинг определения колонки
ColumnDefinition SQLParser::parseColumnDefinition(const std::vector<std::string>& tokens, size_t& pos) {
    ColumnDefinition col;
    
    // Имя колонки
    if (pos < tokens.size()) {
        col.name = tokens[pos];
        pos++;
    }
    
    // Тип данных
    if (pos < tokens.size()) {
        std::string type = toUpper(tokens[pos]);
        pos++;
        
        // Проверка на размер (VARCHAR(255))
        if (pos < tokens.size() && tokens[pos] == "(") {
            pos++;
            if (pos < tokens.size()) {
                col.size = std::stoi(tokens[pos]);
                pos++;
            }
            if (pos < tokens.size() && tokens[pos] == ")") {
                pos++;
            }
        }
        
        col.type = type;
    }
    
    // Дополнительные атрибуты
    while (pos < tokens.size()) {
        std::string token = toUpper(trim(tokens[pos]));
        if (token == "," || token == ")") {
            break;
        }
        
        if (token == "PRIMARY" && pos + 1 < tokens.size() && toUpper(tokens[pos + 1]) == "KEY") {
            col.is_primary_key = true;
            pos += 2;
        } else if (token == "NOT" && pos + 1 < tokens.size() && toUpper(tokens[pos + 1]) == "NULL") {
            col.is_not_null = true;
            pos += 2;
        } else if (token == "DEFAULT" && pos + 1 < tokens.size()) {
            pos++;
            col.default_value = tokens[pos];
            pos++;
        } else {
            pos++;
        }
    }
    
    return col;
}

