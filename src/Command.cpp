#include "ValidationError.h"
#include "Command.h"
#include <iostream>
#include "db.h"

using namespace std;

CreateCommand::CreateCommand(const string& table_name,
    const vector<string>& column_names,
    const vector<string>& data_types,
    const vector<bool>& is_nullable)
    : table_name_(table_name), column_names_(column_names),
    data_types_(data_types), is_nullable_(is_nullable) {
}

void CreateCommand::execute(DB* db) {
    if (!validate()) {
        throw ValidationError(INVALID_SYNTAX, "Ошибка валидации CREATE команды");
    }

    // Проверяем, что таблица не существует
    if (db->tableExists(table_name_)) {
        throw ValidationError(TABLE_ALREADY_EXISTS, "Таблица '" + table_name_ + "' уже существует");
    }

    cout << "Создаем таблицу: " << table_name_ << endl;
    cout << "Структура таблицы:" << endl;
    for (size_t i = 0; i < column_names_.size(); i++) {
        cout << "  " << column_names_[i] << " " << data_types_[i]
            << (is_nullable_[i] ? " NULL" : " NOT NULL") << endl;
    }

    // Регистрируем таблицу в БД
    db->createTable(table_name_);
}

string CreateCommand::getCommandType() const {
    return "CREATE";
}

bool CreateCommand::validate() const {
    if (table_name_.empty()) {
        throw ValidationError(INVALID_SYNTAX, "Имя таблицы не может быть пустым");
    }
    if (column_names_.size() != data_types_.size() ||
        column_names_.size() != is_nullable_.size()) {
        throw ValidationError(INVALID_SYNTAX, "Количество имен колонок, типов данных и флагов nullable не совпадает");
    }
    if (column_names_.empty()) {
        throw ValidationError(INVALID_SYNTAX, "Таблица должна содержать хотя бы одну колонку");
    }

    for (const auto& column_name : column_names_) {
        if (column_name.empty()) {
            throw ValidationError(INVALID_SYNTAX, "Имя колонки не может быть пустым");
        }
    }

    return true;
}

// SelectCommand implementation
SelectCommand::SelectCommand(const vector<string>& columns,
    const string& table_name,
    const vector<string>& where_conditions,
    const vector<string>& order_by,
    int limit)
    : columns_(columns), table_name_(table_name),
    where_conditions_(where_conditions), order_by_(order_by), limit_(limit) {
}

void SelectCommand::execute(DB* db) {
    if (!validate()) {
        throw ValidationError(INVALID_SYNTAX, "Ошибка валидации SELECT команды");
    }

    // Проверяем существование таблицы
    if (!db->tableExists(table_name_)) {
        throw ValidationError(TABLE_NOT_EXISTS, "Таблица '" + table_name_ + "' не существует");
    }

    cout << "Выполняем SELECT из таблицы: " << table_name_ << endl;
    cout << "Колонки: ";
    for (const auto& col : columns_) {
        cout << col << " ";
    }
    cout << endl;

    if (!where_conditions_.empty()) {
        cout << "Условия WHERE: ";
        for (const auto& cond : where_conditions_) {
            cout << cond << " ";
        }
        cout << endl;
    }

    if (!order_by_.empty()) {
        cout << "Сортировка: ";
        for (const auto& order : order_by_) {
            cout << order << " ";
        }
        cout << endl;
    }

    if (limit_ > 0) {
        cout << "Лимит: " << limit_ << endl;
    }
}

string SelectCommand::getCommandType() const {
    return "SELECT";
}

bool SelectCommand::validate() const {
    if (table_name_.empty()) {
        throw ValidationError(INVALID_SYNTAX, "Имя таблицы не может быть пустым");
    }
    return true;
}

UpdateCommand::UpdateCommand(const string& table_name,
    const vector<pair<string, string>>& set_clauses,
    const vector<string>& where_conditions)
    : table_name_(table_name), set_clauses_(set_clauses), where_conditions_(where_conditions) {
}

void UpdateCommand::execute(DB* db) {
    if (!validate()) {
        throw ValidationError(INVALID_SYNTAX, "Ошибка валидации UPDATE команды");
    }

    // Проверяем существование таблицы
    if (!db->tableExists(table_name_)) {
        throw ValidationError(TABLE_NOT_EXISTS, "Таблица '" + table_name_ + "' не существует");
    }

    cout << "Обновляем таблицу: " << table_name_ << endl;
    cout << "SET операции:" << endl;
    for (const auto& set_clause : set_clauses_) {
        cout << "  " << set_clause.first << " = " << set_clause.second << endl;
    }

    if (!where_conditions_.empty()) {
        cout << "Условия WHERE: ";
        for (const auto& cond : where_conditions_) {
            cout << cond << " ";
        }
        cout << endl;
    }
}

string UpdateCommand::getCommandType() const {
    return "UPDATE";
}

bool UpdateCommand::validate() const {
    if (table_name_.empty()) {
        throw ValidationError(INVALID_SYNTAX, "Имя таблицы не может быть пустым");
    }
    if (set_clauses_.empty()) {
        throw ValidationError(INVALID_SYNTAX, "UPDATE должен содержать хотя бы одну SET операцию");
    }

    for (const auto& set_clause : set_clauses_) {
        if (set_clause.first.empty()) {
            throw ValidationError(INVALID_SYNTAX, "Имя колонки в SET операции не может быть пустым");
        }
    }

    return true;
}

DeleteCommand::DeleteCommand(const string& table_name,
    const vector<string>& where_conditions)
    : table_name_(table_name), where_conditions_(where_conditions) {
}

void DeleteCommand::execute(DB* db) {
    if (!validate()) {
        throw ValidationError(INVALID_SYNTAX, "Ошибка валидации DELETE команды");
    }

    if (!db->tableExists(table_name_)) {
        throw ValidationError(TABLE_NOT_EXISTS, "Таблица '" + table_name_ + "' не существует");
    }

    cout << "Удаляем из таблицы: " << table_name_ << endl;

    if (!where_conditions_.empty()) {
        cout << "Условия WHERE: ";
        for (const auto& cond : where_conditions_) {
            cout << cond << " ";
        }
        cout << endl;
    }
    else {
        cout << "ВНИМАНИЕ: Будут удалены все записи таблицы!" << endl;
    }
}

string DeleteCommand::getCommandType() const {
    return "DELETE";
}

bool DeleteCommand::validate() const {
    if (table_name_.empty()) {
        throw ValidationError(INVALID_SYNTAX, "Имя таблицы не может быть пустым");
    }
    return true;
}

InsertCommand::InsertCommand(const string& table_name,
    const vector<string>& column_names,
    const vector<vector<string>>& values)
    : table_name_(table_name), column_names_(column_names), values_(values) {
}

void InsertCommand::execute(DB* db) {
    if (!validate()) {
        throw ValidationError(INVALID_SYNTAX, "Ошибка валидации INSERT команды");
    }

    if (!db->tableExists(table_name_)) {
        throw ValidationError(TABLE_NOT_EXISTS, "Таблица '" + table_name_ + "' не существует");
    }

    cout << "Вставляем данные в таблицу: " << table_name_ << endl;

    if (!column_names_.empty()) {
        cout << "Колонки: ";
        for (const auto& col : column_names_) {
            cout << col << " ";
        }
        cout << endl;
    }

    for (size_t i = 0; i < values_.size(); i++) {
        cout << "Строка " << (i + 1) << ": (";
        for (size_t j = 0; j < values_[i].size(); j++) {
            cout << values_[i][j];
            if (j < values_[i].size() - 1) cout << ", ";
        }
        cout << ")" << endl;
    }
}

string InsertCommand::getCommandType() const {
    return "INSERT";
}

bool InsertCommand::validate() const {
    if (table_name_.empty()) {
        throw ValidationError(INVALID_SYNTAX, "Имя таблицы не может быть пустым");
    }
    if (values_.empty()) {
        throw ValidationError(INVALID_SYNTAX, "INSERT должен содержать хотя бы одну строку значений");
    }

    size_t expected_values = column_names_.empty() ? 0 : column_names_.size();
    for (const auto& row : values_) {
        if (!column_names_.empty() && row.size() != column_names_.size()) {
            throw ValidationError(INVALID_SYNTAX, "Количество значений не совпадает с количеством колонок");
        }
        if (column_names_.empty() && expected_values == 0) {
            expected_values = row.size();
        }
        else if (row.size() != expected_values) {
            throw ValidationError(INVALID_SYNTAX, "Все строки значений должны содержать одинаковое количество элементов");
        }
    }

    return true;
}

AlterCommand::AlterCommand(const string& table_name,
    OperationType operation_type,
    const string& column_name,
    const string& data_type,
    const string& constraint,
    bool nullable)
    : table_name_(table_name), operation_type_(operation_type),
    column_name_(column_name), data_type_(data_type),
    constraint_(constraint), nullable_(nullable) {
}

void AlterCommand::execute(DB* db) {
    if (!validate()) {
        throw ValidationError(INVALID_SYNTAX, "Ошибка валидации ALTER команды");
    }

    if (!db->tableExists(table_name_)) {
        throw ValidationError(TABLE_NOT_EXISTS, "Таблица '" + table_name_ + "' не существует");
    }

    cout << "Изменяем таблицу: " << table_name_ << endl;
    cout << "Тип операции: ";
    switch (operation_type_) {
    case ADD_COLUMN:
        cout << "ADD COLUMN" << endl;
        cout << "Новая колонка: " << column_name_ << " " << data_type_
            << (nullable_ ? " NULL" : " NOT NULL") << endl;
        break;
    case DROP_COLUMN:
        cout << "DROP COLUMN" << endl;
        cout << "Удаляемая колонка: " << column_name_ << endl;
        break;
    case MODIFY_COLUMN:
        cout << "MODIFY COLUMN" << endl;
        cout << "Изменяемая колонка: " << column_name_ << " -> " << data_type_
            << (nullable_ ? " NULL" : " NOT NULL") << endl;
        break;
    case ADD_CONSTRAINT:
        cout << "ADD CONSTRAINT" << endl;
        cout << "Ограничение: " << constraint_ << endl;
        break;
    }
}

string AlterCommand::getCommandType() const {
    return "ALTER";
}

bool AlterCommand::validate() const {
    if (table_name_.empty()) {
        throw ValidationError(INVALID_SYNTAX, "Имя таблицы не может быть пустым");
    }

    switch (operation_type_) {
    case ADD_COLUMN:
    case MODIFY_COLUMN:
        if (column_name_.empty() || data_type_.empty()) {
            throw ValidationError(INVALID_SYNTAX, "ADD_COLUMN и MODIFY_COLUMN требуют указания имени колонки и типа данных");
        }
        break;
    case DROP_COLUMN:
        if (column_name_.empty()) {
            throw ValidationError(INVALID_SYNTAX, "DROP_COLUMN требует указания имени колонки");
        }
        break;
    case ADD_CONSTRAINT:
        if (constraint_.empty()) {
            throw ValidationError(INVALID_SYNTAX, "ADD_CONSTRAINT требует указания ограничения");
        }
        break;
    }

    return true;
}