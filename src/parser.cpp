#include <cstring>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include "types.h"
#include <iostream>
#include <fstream>
#include "metadata.h"
#include "Command.h"
#include "ValidationError.h"

using namespace std;

//функция возвращает тип команды
int parse_command(const string& command) {
    if (command.empty()) return -1;

    string upper_command = command;
    transform(upper_command.begin(), upper_command.end(), upper_command.begin(), ::toupper);

    size_t first_char = upper_command.find_first_not_of(" ");
    if (first_char == string::npos) return -1;

    if (upper_command.compare(first_char, 6, "CREATE") == 0) {
        return 0;
    }
    else if (upper_command.compare(first_char, 6, "SELECT") == 0) {
        return 1;
    }
    else if (upper_command.compare(first_char, 6, "UPDATE") == 0) {
        return 2;
    }
    else if (upper_command.compare(first_char, 6, "DELETE") == 0) {
        return 3;
    }
    else if (upper_command.compare(first_char, 6, "INSERT") == 0) {
        return 4;
    }
    else if (upper_command.compare(first_char, 5, "ALTER") == 0) {
        return 5;
    }

    return -1;
}

// Функция для создания CreateCommand из парсера
unique_ptr<CreateCommand> parse_create_table_query(const string& command) {
    string table_name;
    vector<string> column_names;
    vector<string> data_types;
    vector<bool> is_nullable;

    try {
        size_t table_start = command.find("TABLE") + 5;
        size_t paren_start = command.find("(");

        if (table_start == string::npos || paren_start == string::npos) {
            throw ValidationError(INVALID_SYNTAX, "Неверный синтаксис CREATE TABLE");
        }

        table_name = command.substr(table_start, paren_start - table_start);
        table_name.erase(0, table_name.find_first_not_of(" "));
        table_name.erase(table_name.find_last_not_of(" ") + 1);

        if (table_name.empty()) {
            throw ValidationError(INVALID_SYNTAX, "Имя таблицы не может быть пустым");
        }

        string columns_part = command.substr(paren_start + 1, command.find_last_of(")") - paren_start - 1);
        columns_part.erase(0, columns_part.find_first_not_of(" "));
        columns_part.erase(columns_part.find_last_not_of(" ") + 1);

        stringstream ss(columns_part);
        string column_definition;
        vector<string> parsed_column_names;

        while (getline(ss, column_definition, ',')) {
            column_definition.erase(0, column_definition.find_first_not_of(" "));
            column_definition.erase(column_definition.find_last_not_of(" ") + 1);

            if (column_definition.empty()) continue;

            size_t first_space = column_definition.find(" ");
            if (first_space == string::npos) {
                throw ValidationError(INVALID_SYNTAX, "Неверное определение колонки: " + column_definition);
            }

            string column_name = column_definition.substr(0, first_space);
            string column_type_str = column_definition.substr(first_space + 1);

            column_type_str.erase(0, column_type_str.find_first_not_of(" "));
            column_type_str.erase(column_type_str.find_last_not_of(" ") + 1);

            if (find(parsed_column_names.begin(), parsed_column_names.end(), column_name) != parsed_column_names.end()) {
                throw ValidationError(INVALID_SYNTAX, "Дублирующееся имя колонки: " + column_name);
            }
            parsed_column_names.push_back(column_name);

            string base_type = column_type_str;
            int size = 0;

            size_t paren_open = column_type_str.find("(");
            if (paren_open != string::npos) {
                base_type = column_type_str.substr(0, paren_open);
                size_t paren_close = column_type_str.find(")");
                if (paren_close != string::npos) {
                    string size_str = column_type_str.substr(paren_open + 1, paren_close - paren_open - 1);
                    try {
                        size = stoi(size_str);
                    }
                    catch (const exception&) {
                        throw ValidationError(INVALID_SYNTAX, "Неверный размер для колонки: " + column_name);
                    }
                }
            }

            All_types column_type_id = get_type_from_string(base_type);

            if (column_type_id == All_types::VARCHAR && size <= 0) {
                throw ValidationError(INVALID_SYNTAX, "VARCHAR должен иметь размер для колонки " + column_name);
            }

            if (column_type_id == All_types::CHAR_TYPE && size <= 0) {
                size = 1;
            }

            column_names.push_back(column_name);
            data_types.push_back(column_type_str);
            is_nullable.push_back(true); 
        }

        if (column_names.empty()) {
            throw ValidationError(INVALID_SYNTAX, "Таблица должна иметь хотя бы одну колонку");
        }

    }
    catch (const ValidationError&) {
        throw;
    }
    catch (const exception& e) {
        throw ValidationError(INVALID_SYNTAX, "Ошибка парсинга CREATE TABLE: " + string(e.what()));
    }

    return make_unique<CreateCommand>(table_name, column_names, data_types, is_nullable);
}

// Функция для создания SelectCommand
unique_ptr<SelectCommand> parse_select_query(const string& command) {
    string upper_command = command;
    transform(upper_command.begin(), upper_command.end(), upper_command.begin(), ::toupper);

    size_t select_pos = upper_command.find("SELECT");
    if (select_pos == string::npos) {
        throw ValidationError(INVALID_SYNTAX, "Отсутствует SELECT в запросе");
    }

    size_t from_pos = upper_command.find("FROM");
    if (from_pos == string::npos) {
        throw ValidationError(INVALID_SYNTAX, "Отсутствует FROM в SELECT запросе");
    }

    // Извлекаем список колонок
    string columns_str = command.substr(select_pos + 6, from_pos - (select_pos + 6));
    columns_str.erase(0, columns_str.find_first_not_of(" \t"));
    columns_str.erase(columns_str.find_last_not_of(" \t") + 1);

    // Проверяем, что есть колонки
    if (columns_str.empty()) {
        throw ValidationError(INVALID_SYNTAX, "SELECT должен содержать список колонок или *");
    }

    vector<string> columns;
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

    string table_name = command.substr(from_pos + 4);
    table_name.erase(0, table_name.find_first_not_of(" "));

    // Убираем возможные условия WHERE, ORDER BY и тд
    size_t where_pos_global = upper_command.find("WHERE", from_pos);
    size_t order_pos = upper_command.find("ORDER BY", from_pos);
    size_t limit_pos = upper_command.find("LIMIT", from_pos);

    size_t end_pos = string::npos;
    if (where_pos_global != string::npos) end_pos = min(end_pos, where_pos_global);
    if (order_pos != string::npos) end_pos = min(end_pos, order_pos);
    if (limit_pos != string::npos) end_pos = min(end_pos, limit_pos);

    if (end_pos != string::npos) {
        table_name = command.substr(from_pos + 4, end_pos - (from_pos + 4));
    }

    table_name.erase(0, table_name.find_first_not_of(" "));
    table_name.erase(table_name.find_last_not_of(" \t;") + 1);

    if (table_name.empty()) {
        throw ValidationError(INVALID_SYNTAX, "Имя таблицы не может быть пустым");
    }

    vector<string> where_conditions;
    vector<string> order_by;
    int limit = -1;

    // Парсим WHERE если есть
    if (where_pos_global != string::npos) {
        size_t where_start = where_pos_global + 5;
        string where_clause = command.substr(where_start);

        // Убираем ORDER BY и LIMIT из WHERE условия
        size_t where_end = where_clause.length();
        size_t order_in_where = where_clause.find("ORDER BY");
        size_t limit_in_where = where_clause.find("LIMIT");

        if (order_in_where != string::npos) where_end = min(where_end, order_in_where);
        if (limit_in_where != string::npos) where_end = min(where_end, limit_in_where);

        where_clause = where_clause.substr(0, where_end);
        where_clause.erase(0, where_clause.find_first_not_of(" \t"));
        where_clause.erase(where_clause.find_last_not_of(" \t") + 1);

        if (!where_clause.empty()) {
            where_conditions.push_back(where_clause);
        }
    }

    return make_unique<SelectCommand>(columns, table_name, where_conditions, order_by, limit);
}

// Функция для создания InsertCommand
unique_ptr<InsertCommand> parse_insert_query(const string& command) {
    string upper_command = command;
    transform(upper_command.begin(), upper_command.end(), upper_command.begin(), ::toupper);

    size_t into_pos = upper_command.find("INTO");
    if (into_pos == string::npos) {
        throw ValidationError(INVALID_SYNTAX, "Отсутствует INTO в INSERT запросе");
    }

    size_t values_pos = upper_command.find("VALUES");
    if (values_pos == string::npos) {
        throw ValidationError(INVALID_SYNTAX, "Отсутствует VALUES в INSERT запросе");
    }

    string table_part = command.substr(into_pos + 4, values_pos - (into_pos + 4));
    table_part.erase(0, table_part.find_first_not_of(" "));
    table_part.erase(table_part.find_last_not_of(" ") + 1);

    string table_name;
    vector<string> column_names;

    // Проверяем есть ли список колонок
    size_t paren_open = table_part.find("(");
    if (paren_open != string::npos) {
        table_name = table_part.substr(0, paren_open);
        table_name.erase(0, table_name.find_first_not_of(" "));
        table_name.erase(table_name.find_last_not_of(" ") + 1);

        size_t paren_close = table_part.find(")");
        if (paren_close == string::npos) {
            throw ValidationError(INVALID_SYNTAX, "Незакрытые скобки в списке колонок INSERT");
        }

        string columns_str = table_part.substr(paren_open + 1, paren_close - paren_open - 1);
        stringstream ss(columns_str);
        string column;
        while (getline(ss, column, ',')) {
            column.erase(0, column.find_first_not_of(" "));
            column.erase(column.find_last_not_of(" ") + 1);
            if (!column.empty()) {
                column_names.push_back(column);
            }
        }
    }
    else {
        table_name = table_part;
    }

    if (table_name.empty()) {
        throw ValidationError(INVALID_SYNTAX, "Имя таблицы не может быть пустым");
    }

    // Парсим значения
    string values_part = command.substr(values_pos + 6);
    values_part.erase(0, values_part.find_first_not_of(" "));

    size_t values_start = values_part.find("(");
    size_t values_end = values_part.find(")");
    if (values_start == string::npos || values_end == string::npos) {
        throw ValidationError(INVALID_SYNTAX, "Неверный формат значений в INSERT");
    }

    string values_str = values_part.substr(values_start + 1, values_end - values_start - 1);
    vector<string> values;
    stringstream ss_values(values_str);
    string value;

    while (getline(ss_values, value, ',')) {
        value.erase(0, value.find_first_not_of(" "));
        value.erase(value.find_last_not_of(" ") + 1);

        // Убираем кавычки если есть
        if (!value.empty() && value[0] == '\'' && value.back() == '\'') {
            value = value.substr(1, value.length() - 2);
        }

        values.push_back(value);
    }

    vector<vector<string>> values_vector = { values };

    return make_unique<InsertCommand>(table_name, column_names, values_vector);
}

// Функция для создания UpdateCommand
unique_ptr<UpdateCommand> parse_update_query(const string& command) {
    string upper_command = command;
    transform(upper_command.begin(), upper_command.end(), upper_command.begin(), ::toupper);

    size_t set_pos = upper_command.find("SET");
    if (set_pos == string::npos) {
        throw ValidationError(INVALID_SYNTAX, "Отсутствует SET в UPDATE запросе");
    }

    string table_name = command.substr(6, set_pos - 6); 
    table_name.erase(0, table_name.find_first_not_of(" "));
    table_name.erase(table_name.find_last_not_of(" ") + 1);

    if (table_name.empty()) {
        throw ValidationError(INVALID_SYNTAX, "Имя таблицы не может быть пустым");
    }

    string set_part;
    size_t where_pos = upper_command.find("WHERE");

    if (where_pos != string::npos) {
        set_part = command.substr(set_pos + 3, where_pos - (set_pos + 3));
    }
    else {
        set_part = command.substr(set_pos + 3);
    }

    vector<pair<string, string>> set_clauses;
    vector<string> where_conditions;

    // Парсим SET clauses
    stringstream ss_set(set_part);
    string assignment;
    while (getline(ss_set, assignment, ',')) {
        size_t eq_pos = assignment.find('=');
        if (eq_pos != string::npos) {
            string column = assignment.substr(0, eq_pos);
            string value = assignment.substr(eq_pos + 1);

            column.erase(0, column.find_first_not_of(" "));
            column.erase(column.find_last_not_of(" ") + 1);
            value.erase(0, value.find_first_not_of(" "));
            value.erase(value.find_last_not_of(" ") + 1);

            set_clauses.push_back({ column, value });
        }
    }

    // Парсим WHERE если есть
    if (where_pos != string::npos) {
        string where_part = command.substr(where_pos + 5);
        where_part.erase(0, where_part.find_first_not_of(" "));
        where_conditions.push_back(where_part);
    }

    return make_unique<UpdateCommand>(table_name, set_clauses, where_conditions);
}

// Функция для создания DeleteCommand
unique_ptr<DeleteCommand> parse_delete_query(const string& command) {
    string upper_command = command;
    transform(upper_command.begin(), upper_command.end(), upper_command.begin(), ::toupper);

    size_t from_pos = upper_command.find("FROM");
    if (from_pos == string::npos) {
        throw ValidationError(INVALID_SYNTAX, "Отсутствует FROM в DELETE запросе");
    }

    // Извлекаем часть после FROM
    string after_from = command.substr(from_pos + 4);
    after_from.erase(0, after_from.find_first_not_of(" "));

    string table_name;
    vector<string> where_conditions;

    size_t where_pos = upper_command.find("WHERE", from_pos + 4);

    if (where_pos != string::npos) {
        // Извлекаем имя таблицы 
        table_name = command.substr(from_pos + 4, where_pos - (from_pos + 4));
        table_name.erase(0, table_name.find_first_not_of(" "));
        table_name.erase(table_name.find_last_not_of(" \t") + 1);

        // Извлекаем условие WHERE
        string where_part = command.substr(where_pos + 5);
        where_part.erase(0, where_part.find_first_not_of(" "));
        where_part.erase(where_part.find_last_not_of(" \t;") + 1);
        where_conditions.push_back(where_part);
    }
    else {
        // Нет WHERE
        table_name = after_from;
        table_name.erase(table_name.find_last_not_of(" \t;") + 1);
    }

    // Очищаем имя таблицы от пробелов
    table_name.erase(0, table_name.find_first_not_of(" "));
    table_name.erase(table_name.find_last_not_of(" \t") + 1);

    if (table_name.empty()) {
        throw ValidationError(INVALID_SYNTAX, "Имя таблицы не может быть пустым");
    }

    return make_unique<DeleteCommand>(table_name, where_conditions);
}

// Функция для создания AlterCommand
unique_ptr<AlterCommand> parse_alter_query(const string& command) {
    string upper_command = command;
    transform(upper_command.begin(), upper_command.end(), upper_command.begin(), ::toupper);

    size_t pos = 0;
    while (pos < upper_command.length() && isspace(upper_command[pos])) ++pos;

    if (upper_command.substr(pos, 5) != "ALTER") {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'ALTER' в запросе: " + command);
    }
    pos += 5;

    while (pos < upper_command.length() && isspace(upper_command[pos])) ++pos;

    if (upper_command.substr(pos, 5) != "TABLE") {
        throw ValidationError(INVALID_SYNTAX, "Ожидается 'TABLE' в ALTER запросе: " + command);
    }
    pos += 5;

    while (pos < upper_command.length() && isspace(upper_command[pos])) ++pos;

    // Извлекаем имя таблицы
    size_t table_start = pos;
    while (pos < upper_command.length() && !isspace(upper_command[pos]) && upper_command[pos] != ';') {
        ++pos;
    }

    if (table_start == pos) {
        throw ValidationError(INVALID_SYNTAX, "Отсутствует имя таблицы в ALTER запросе: " + command);
    }

    string table_name = command.substr(table_start, pos - table_start);

    while (pos < upper_command.length() && isspace(upper_command[pos])) ++pos;

    // Определяем тип операции
    AlterCommand::OperationType operation_type;
    string column_name, data_type;

    if (upper_command.substr(pos, 3) == "ADD") {
        operation_type = AlterCommand::ADD_COLUMN;
        pos += 3;
        while (pos < upper_command.length() && isspace(upper_command[pos])) ++pos;

        // Пропускаем ключевое слово COLUMN если есть
        if (upper_command.substr(pos, 6) == "COLUMN") {
            pos += 6;
            while (pos < upper_command.length() && isspace(upper_command[pos])) ++pos;
        }

        // Извлекаем имя колонки
        size_t column_start = pos;
        while (pos < upper_command.length() && !isspace(upper_command[pos]) && upper_command[pos] != ';') {
            ++pos;
        }
        column_name = command.substr(column_start, pos - column_start);

        // Оставшаяся часть - тип данных
        while (pos < upper_command.length() && isspace(upper_command[pos])) ++pos;
        data_type = command.substr(pos);
    }
    else if (upper_command.substr(pos, 4) == "DROP") {
        operation_type = AlterCommand::DROP_COLUMN;
        pos += 4;
        while (pos < upper_command.length() && isspace(upper_command[pos])) ++pos;

        // Пропускаем ключевое слово COLUMN если есть
        if (upper_command.substr(pos, 6) == "COLUMN") {
            pos += 6;
            while (pos < upper_command.length() && isspace(upper_command[pos])) ++pos;
        }

        // Извлекаем имя колонки
        size_t column_start = pos;
        while (pos < upper_command.length() && !isspace(upper_command[pos]) && upper_command[pos] != ';') {
            ++pos;
        }
        column_name = command.substr(column_start, pos - column_start);
    }
    else {
        throw ValidationError(INVALID_SYNTAX, "Неизвестная операция в ALTER запросе: " + command);
    }

    return make_unique<AlterCommand>(table_name, operation_type, column_name, data_type, "", true);
}

// Главная функция парсера которая возвращает Command
unique_ptr<Command> parse_sql_command(const string& command) {
    int command_type = parse_command(command);

    switch (command_type) {
    case 0: 
        return parse_create_table_query(command);
    case 1: 
        return parse_select_query(command);
    case 2: 
        return parse_update_query(command);
    case 3: 
        return parse_delete_query(command);
    case 4: 
        return parse_insert_query(command);
    case 5: 
        return parse_alter_query(command);
    default:
        throw ValidationError(INVALID_SYNTAX, "Неизвестная или неподдерживаемая команда: " + command);
    }
}

// Старая функция main для тестирования
int main3() {
    string command;

    while (true) {
        cout << "\n=== Almost SQLite ===" << endl;
        cout << "Введите SQL команду (или 'EXIT' для выхода): ";
        getline(cin, command);

        if (command == "EXIT" || command == "exit") {
            cout << "Выход..." << endl;
            break;
        }

        if (command.empty()) continue;

        try {
            auto sql_command = parse_sql_command(command);
            cout << "Успешно распарсена команда: " << sql_command->getCommandType() << endl;

            // Здесь можно вызвать 

        }
        catch (const ValidationError& e) {
            cout << "Ошибка валидации: " << e.what() << endl;
        }
        catch (const exception& e) {
            cout << "Ошибка: " << e.what() << endl;
        }
    }

    return 0;
}