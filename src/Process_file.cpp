#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <memory>
#include "types.h"
#include "metadata.h"
#include "Command.h"
#include "ValidationError.h"

using namespace std;

class Metadata {
};

unique_ptr<Command> parse_create(const string& query, Metadata& metadata) {
    string query_upper = query;
    transform(query_upper.begin(), query_upper.end(), query_upper.begin(), ::toupper);

    size_t pos = 0;
    while (pos < query_upper.length() && isspace(query_upper[pos])) {
        ++pos;
    }

    if (query_upper.substr(pos, 11) != "CREATE TABLE") {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'CREATE TABLE' в запросе: " + query);
    }
    pos += 11;

    while (pos < query_upper.length() && isspace(query_upper[pos])) {
        ++pos;
    }

    size_t start_table = pos;
    while (pos < query_upper.length() && !isspace(query_upper[pos]) && query_upper[pos] != '(') {
        ++pos;
    }
    if (start_table == pos) {
        throw ValidationError(INVALID_SYNTAX, "Отсутствует имя таблицы в CREATE TABLE запросе: " + query);
    }
    string table_name = query.substr(start_table, pos - start_table);

    while (pos < query_upper.length() && isspace(query_upper[pos])) {
        ++pos;
    }

    if (pos >= query_upper.length() || query_upper[pos] != '(') {
        throw ValidationError(INVALID_SYNTAX, "Ожидается '(' после имени таблицы в CREATE TABLE запросе: " + query);
    }
    ++pos;

    vector<string> column_names;
    vector<string> data_types;
    vector<bool> is_nullable;

    string current_token = "";
    int paren_count = 1;
    bool in_quotes = false;
    char quote_char = 0;

    auto add_column_def = [&column_names, &data_types, &is_nullable](const string& token) {
        istringstream iss(token);
        string col_name, data_type;
        string word;
        bool nullable = true;
        bool name_found = false;
        bool type_found = false;

        while (iss >> word) {
            if (!name_found) {
                col_name = word;
                name_found = true;
                continue;
            }
            if (!type_found) {
                data_type = word;
                type_found = true;
                continue;
            }
            if (word == "NOT") {
                string next_word;
                if (iss >> next_word && next_word == "NULL") {
                    nullable = false;
                }
            }
            else if (word == "NULL") {
            }
        }

        if (!name_found || !type_found) {
            throw ValidationError(INVALID_SYNTAX, "Неверное определение колонки в CREATE TABLE: " + token);
        }

        column_names.push_back(col_name);
        data_types.push_back(data_type);
        is_nullable.push_back(nullable);
        };

    while (pos < query.length() && paren_count > 0) {
        char c = query[pos];

        if (in_quotes) {
            if (c == quote_char) {
                in_quotes = false;
                quote_char = 0;
            }
            current_token += c;
            ++pos;
            continue;
        }

        if (c == '\'' || c == '"') {
            in_quotes = true;
            quote_char = c;
        }
        else if (c == '(') {
            paren_count++;
            current_token += c;
        }
        else if (c == ')') {
            paren_count--;
            if (paren_count == 0) {
                if (!current_token.empty()) {
                    size_t start = current_token.find_first_not_of(" \t\n\r");
                    size_t end = current_token.find_last_not_of(" \t\n\r");
                    if (start != string::npos && end != string::npos) {
                        add_column_def(current_token.substr(start, end - start + 1));
                    }
                    else if (!current_token.empty()) {
                        add_column_def(current_token.substr(start));
                    }
                }
                break;
            }
            else {
                current_token += c;
            }
        }
        else if (c == ',' && paren_count == 1) {
            if (!current_token.empty()) {
                size_t start = current_token.find_first_not_of(" \t\n\r");
                size_t end = current_token.find_last_not_of(" \t\n\r");
                if (start != string::npos && end != string::npos) {
                    add_column_def(current_token.substr(start, end - start + 1));
                }
                else if (!current_token.empty()) {
                    add_column_def(current_token.substr(start));
                }
            }
            current_token.clear();
        }
        else {
            current_token += c;
        }
        ++pos;
    }

    if (paren_count != 0) {
        throw ValidationError(INVALID_SYNTAX, "Непарные скобки в списке определений колонок CREATE TABLE запроса: " + query);
    }

    while (pos < query.length() && isspace(query[pos])) {
        ++pos;
    }

    return make_unique<CreateCommand>(table_name, column_names, data_types, is_nullable);
}

unique_ptr<Command> parse_select(const string& query, Metadata& metadata) {
    string query_upper = query;
    transform(query_upper.begin(), query_upper.end(), query_upper.begin(), ::toupper);

    size_t pos = 0;
    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    if (query_upper.substr(pos, 6) != "SELECT") {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'SELECT' в запросе: " + query);
    }
    pos += 6;

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    vector<string> columns;
    size_t from_pos = query_upper.find("FROM", pos);
    if (from_pos == string::npos) {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'FROM' в SELECT запросе: " + query);
    }

    string columns_str = query.substr(pos, from_pos - pos);
    stringstream ss_columns(columns_str);
    string column;
    while (getline(ss_columns, column, ',')) {
        column.erase(0, column.find_first_not_of(" \t"));
        column.erase(column.find_last_not_of(" \t") + 1);
        if (!column.empty()) {
            columns.push_back(column);
        }
    }

    if (columns.empty()) {
        columns.push_back("*");
    }

    pos = from_pos + 4;
    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    size_t table_end = query_upper.find(" ", pos);
    if (table_end == string::npos) {
        table_end = query_upper.length();
    }

    string table_name = query.substr(pos, table_end - pos);
    pos = table_end;

    vector<string> where_conditions;
    vector<string> order_by;
    int limit = -1;

    size_t where_pos = query_upper.find("WHERE", pos);
    if (where_pos != string::npos) {
        pos = where_pos + 5;
        while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

        size_t where_end = query_upper.length();
        size_t order_pos = query_upper.find("ORDER BY", pos);
        size_t limit_pos = query_upper.find("LIMIT", pos);

        if (order_pos != string::npos) where_end = min(where_end, order_pos);
        if (limit_pos != string::npos) where_end = min(where_end, limit_pos);

        string where_clause = query.substr(pos, where_end - pos);
        where_conditions.push_back(where_clause);
        pos = where_end;
    }

    size_t order_pos = query_upper.find("ORDER BY", pos);
    if (order_pos != string::npos) {
        pos = order_pos + 8;
        while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

        size_t order_end = query_upper.length();
        size_t limit_pos = query_upper.find("LIMIT", pos);
        if (limit_pos != string::npos) order_end = min(order_end, limit_pos);

        string order_clause = query.substr(pos, order_end - pos);
        order_by.push_back(order_clause);
        pos = order_end;
    }

    size_t limit_pos = query_upper.find("LIMIT", pos);
    if (limit_pos != string::npos) {
        pos = limit_pos + 5;
        while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

        size_t limit_end = query_upper.find(" ", pos);
        if (limit_end == string::npos) limit_end = query_upper.length();

        string limit_str = query.substr(pos, limit_end - pos);
        try {
            limit = stoi(limit_str);
        }
        catch (const exception&) {
            throw ValidationError(INVALID_SYNTAX, "Неверный формат LIMIT в SELECT запросе: " + query);
        }
    }

    return make_unique<SelectCommand>(columns, table_name, where_conditions, order_by, limit);
}

unique_ptr<Command> parse_insert(const string& query, Metadata& metadata) {
    string query_upper = query;
    transform(query_upper.begin(), query_upper.end(), query_upper.begin(), ::toupper);

    size_t pos = 0;
    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    if (query_upper.substr(pos, 6) != "INSERT") {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'INSERT' в запросе: " + query);
    }
    pos += 6;

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    if (query_upper.substr(pos, 4) == "INTO") {
        pos += 4;
        while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;
    }

    size_t table_start = pos;
    while (pos < query_upper.length() && !isspace(query_upper[pos]) && query_upper[pos] != '(') {
        ++pos;
    }

    if (table_start == pos) {
        throw ValidationError(INVALID_SYNTAX, "Отсутствует имя таблицы в INSERT запросе: " + query);
    }

    string table_name = query.substr(table_start, pos - table_start);

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    vector<string> column_names;
    if (pos < query.length() && query[pos] == '(') {
        ++pos;
        size_t column_start = pos;

        while (pos < query.length() && query[pos] != ')') {
            ++pos;
        }

        if (pos >= query.length()) {
            throw ValidationError(INVALID_SYNTAX, "Незакрытый список колонок в INSERT запросе: " + query);
        }

        string columns_str = query.substr(column_start, pos - column_start);
        stringstream ss(columns_str);
        string column;

        while (getline(ss, column, ',')) {
            column.erase(0, column.find_first_not_of(" \t"));
            column.erase(column.find_last_not_of(" \t") + 1);
            if (!column.empty()) {
                column_names.push_back(column);
            }
        }
        ++pos;
    }

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    if (query_upper.substr(pos, 6) != "VALUES") {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'VALUES' в INSERT запросе: " + query);
    }
    pos += 6;

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    vector<vector<string>> values;
    if (pos < query.length() && query[pos] == '(') {
        ++pos;
        size_t values_start = pos;

        while (pos < query.length() && query[pos] != ')') {
            ++pos;
        }

        if (pos >= query.length()) {
            throw ValidationError(INVALID_SYNTAX, "Незакрытый список значений в INSERT запросе: " + query);
        }

        string values_str = query.substr(values_start, pos - values_start);
        vector<string> row_values;
        string current_value;
        bool in_quotes = false;
        char quote_char = 0;

        for (char c : values_str) {
            if (in_quotes) {
                if (c == quote_char) {
                    in_quotes = false;
                }
                current_value += c;
            }
            else {
                if (c == '\'' || c == '"') {
                    in_quotes = true;
                    quote_char = c;
                    current_value += c;
                }
                else if (c == ',') {
                    current_value.erase(0, current_value.find_first_not_of(" \t"));
                    current_value.erase(current_value.find_last_not_of(" \t") + 1);
                    row_values.push_back(current_value);
                    current_value.clear();
                }
                else {
                    current_value += c;
                }
            }
        }

        if (!current_value.empty()) {
            current_value.erase(0, current_value.find_first_not_of(" \t"));
            current_value.erase(current_value.find_last_not_of(" \t") + 1);
            row_values.push_back(current_value);
        }

        values.push_back(row_values);
    }
    else {
        throw ValidationError(INVALID_SYNTAX, "Ожидается список значений в INSERT запросе: " + query);
    }

    return make_unique<InsertCommand>(table_name, column_names, values);
}

unique_ptr<Command> parse_update(const string& query, Metadata& metadata) {
    string query_upper = query;
    transform(query_upper.begin(), query_upper.end(), query_upper.begin(), ::toupper);

    size_t pos = 0;
    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    if (query_upper.substr(pos, 6) != "UPDATE") {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'UPDATE' в запросе: " + query);
    }
    pos += 6;

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    size_t table_start = pos;
    while (pos < query_upper.length() && !isspace(query_upper[pos])) {
        ++pos;
    }

    if (table_start == pos) {
        throw ValidationError(INVALID_SYNTAX, "Отсутствует имя таблицы в UPDATE запросе: " + query);
    }

    string table_name = query.substr(table_start, pos - table_start);

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    if (query_upper.substr(pos, 3) != "SET") {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'SET' в UPDATE запросе: " + query);
    }
    pos += 3;

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    vector<pair<string, string>> set_clauses;
    size_t where_pos = query_upper.find("WHERE", pos);
    string set_str;

    if (where_pos != string::npos) {
        set_str = query.substr(pos, where_pos - pos);
        pos = where_pos;
    }
    else {
        set_str = query.substr(pos);
    }

    stringstream ss_set(set_str);
    string assignment;
    while (getline(ss_set, assignment, ',')) {
        size_t eq_pos = assignment.find('=');
        if (eq_pos != string::npos) {
            string column = assignment.substr(0, eq_pos);
            string value = assignment.substr(eq_pos + 1);

            column.erase(0, column.find_first_not_of(" \t"));
            column.erase(column.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (!value.empty() && value[0] == '\'' && value.back() == '\'') {
                value = value.substr(1, value.size() - 2);
            }

            set_clauses.push_back({ column, value });
        }
    }

    vector<string> where_conditions;
    if (where_pos != string::npos) {
        pos = where_pos + 5;
        while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

        string where_clause = query.substr(pos);
        where_conditions.push_back(where_clause);
    }

    return make_unique<UpdateCommand>(table_name, set_clauses, where_conditions);
}

unique_ptr<Command> parse_delete(const string& query, Metadata& metadata) {
    string query_upper = query;
    transform(query_upper.begin(), query_upper.end(), query_upper.begin(), ::toupper);

    size_t pos = 0;
    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    if (query_upper.substr(pos, 6) != "DELETE") {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'DELETE' в запросе: " + query);
    }
    pos += 6;

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    if (query_upper.substr(pos, 4) != "FROM") {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'FROM' в DELETE запросе: " + query);
    }
    pos += 4;

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    size_t table_start = pos;
    while (pos < query_upper.length() && !isspace(query_upper[pos])) {
        ++pos;
    }

    if (table_start == pos) {
        throw ValidationError(INVALID_SYNTAX, "Отсутствует имя таблицы в DELETE запросе: " + query);
    }

    string table_name = query.substr(table_start, pos - table_start);

    vector<string> where_conditions;
    size_t where_pos = query_upper.find("WHERE", pos);
    if (where_pos != string::npos) {
        pos = where_pos + 5;
        while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

        string where_clause = query.substr(pos);
        where_conditions.push_back(where_clause);
    }

    return make_unique<DeleteCommand>(table_name, where_conditions);
}

unique_ptr<Command> parse_alter(const string& query, Metadata& metadata) {
    string query_upper = query;
    transform(query_upper.begin(), query_upper.end(), query_upper.begin(), ::toupper);

    size_t pos = 0;
    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    if (query_upper.substr(pos, 5) != "ALTER") {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'ALTER' в запросе: " + query);
    }
    pos += 5;

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    if (query_upper.substr(pos, 5) != "TABLE") {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'TABLE' в ALTER запросе: " + query);
    }
    pos += 5;

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    size_t table_start = pos;
    while (pos < query_upper.length() && !isspace(query_upper[pos])) {
        ++pos;
    }

    if (table_start == pos) {
        throw ValidationError(INVALID_SYNTAX, "Отсутствует имя таблицы в ALTER запросе: " + query);
    }

    string table_name = query.substr(table_start, pos - table_start);

    while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

    AlterCommand::OperationType operation_type;
    string column_name, data_type, constraint;
    bool nullable = true;

    if (query_upper.substr(pos, 3) == "ADD") {
        operation_type = AlterCommand::ADD_COLUMN;
        pos += 3;
        while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

        size_t column_start = pos;
        while (pos < query_upper.length() && !isspace(query_upper[pos])) {
            ++pos;
        }
        column_name = query.substr(column_start, pos - column_start);

        while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;
        data_type = query.substr(pos);
    }
    else if (query_upper.substr(pos, 4) == "DROP") {
        operation_type = AlterCommand::DROP_COLUMN;
        pos += 4;
        while (pos < query_upper.length() && isspace(query_upper[pos])) ++pos;

        size_t column_start = pos;
        while (pos < query_upper.length() && !isspace(query_upper[pos])) {
            ++pos;
        }
        column_name = query.substr(column_start, pos - column_start);
    }
    else {
        throw ValidationError(INVALID_SYNTAX, "Неизвестная операция в ALTER запросе: " + query);
    }

    return make_unique<AlterCommand>(table_name, operation_type, column_name, data_type, constraint, nullable);
}