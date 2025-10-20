#include <cstring> //c����� (strlen � ��)
#include <sstream> //��������� ������ (getline � ��)
#include <vector> //������� ����
#include <algorithm> //������ � ������������ (find, transform � ��)
#include <cctype> //������������� �������� (::toupper, ::isdigit � ��)
#include "types.h"
#include <iostream>
#include <fstream>
#include "metadata.h"

using namespace std;

//������� ������� SQL
int parse_command(const string& command) {
    if (command.empty()) return -1;

    string upper_command = command;//������� 
    transform(upper_command.begin(), upper_command.end(), upper_command.begin(), ::toupper);

    size_t first_char = upper_command.find_first_not_of(" ");//�������� ��� �������� 
    if (first_char == string::npos) return -1;

    if (upper_command.compare(first_char, 6, "CREATE") == 0) { //�������� ��� ���������� � ��������� �����
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

    return -1;
}

//��������� ��� �������� �������� ������
struct CellValue {
    All_types type;
    string data;  //������ ��� ������

    CellValue(All_types t = INT, const string& d = "") : type(t), data(d) {}
};//������������� �����

//��������� ������������ ������ (����� ���������� �������� � ������� ������ ������)
struct DynamicRecord {
    vector<CellValue> values;  //������ �� ����� �������

    //����������� ��� ��������
    DynamicRecord(const vector<CellValue>& vals = {}) : values(vals) {}

    //��������� �������� � ����� ������
    void add_value(All_types type, const string& data) {
        values.push_back(CellValue(type, data));
    }

    //��������� �������� ������� ��� ������ (��� ������)
    string get_string_value(int column_index) const {
        if (column_index < values.size()) { //�������� ������
            return values[column_index].data;
        }
        return "";
    }

    //��������� �������� ������� ��� ������ ����� (��� �������� ��������)
    int get_int_value(int column_index) const {
        if (column_index < values.size()) {
            try {
                return stoi(values[column_index].data);//tring to integer
            }
            catch (...) {
                return 0;
            }
        }
        return 0;
    }
};

//�������� �������
struct CreateTableQuery {
    string table_name;
    vector<Column> columns;   //������ ��������
};

//������� �������� �� ��� ������� �������� ������
bool is_keyword(const string& table_name) {
    vector<string> sql_keywords = {
        "SELECT", "UPDATE", "DELETE", "INSERT", "WHERE", "FROM", "CREATE", "DROP",
        "ALTER", "JOIN", "GROUP", "ORDER", "BY", "HAVING", "TABLE", "SET"
    };

    string upper_name = table_name;
    transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);

    return find(sql_keywords.begin(), sql_keywords.end(), upper_name) != sql_keywords.end(); //������ � �������
}

//������� �������� ������� CREATE TABLE
CreateTableQuery parse_create_table_query(const string& command) {
    CreateTableQuery query;//������� query ��� ���������

    try {
        //��������� ��� �������
        size_t table_start = command.find("TABLE") + 5;
        size_t paren_start = command.find("(");

        if (table_start == string::npos || paren_start == string::npos) { //�������� �� ���� �������� 
            cout << "Error: Invalid CREATE TABLE syntax" << endl;
            return query;
        }

        //��������� � ������� ��� �������
        string table_name = command.substr(table_start, paren_start - table_start);

        //������ ������� �������
        table_name.erase(0, table_name.find_first_not_of(" "));
        table_name.erase(table_name.find_last_not_of(" ") + 1);

        if (table_name.empty()) { //�������� ����� �������
            cout << "Error: Table name is empty" << endl;
            return query;
        }

        query.table_name = table_name; //���������� ����� ������� � ��������� ����������

        //��������� ����� �� ��������� (�� ��� � �������)
        string columns_part = command.substr(paren_start + 1, command.find_last_of(")") - paren_start - 1);

        //������� ������� � ������ � �����
        columns_part.erase(0, columns_part.find_first_not_of(" "));
        columns_part.erase(columns_part.find_last_not_of(" ") + 1);

        //��������� ������
        stringstream ss(columns_part);//�������� ������ �� �����������
        string column_definition;//����� ��� ������ ����������� �������
        vector<string> column_names; //��������� ��������� ��� �������� ������������

        //���� �������� ��������
        while (getline(ss, column_definition, ',')) { //����������� ��������
            column_definition.erase(0, column_definition.find_first_not_of(" "));
            column_definition.erase(column_definition.find_last_not_of(" ") + 1);

            if (column_definition.empty()) continue;//������� ������ �����������

            //��������� ��� � ��� �������
            size_t first_space = column_definition.find(" ");
            if (first_space == string::npos) {
                cout << "Error: Invalid column definition: " << column_definition << endl;
                continue;
            }

            string column_name = column_definition.substr(0, first_space);//���������� ����� �����
            string column_type_str = column_definition.substr(first_space + 1);//���������� ����

            //������� ������� � ����
            column_type_str.erase(0, column_type_str.find_first_not_of(" "));
            column_type_str.erase(column_type_str.find_last_not_of(" ") + 1);

            //�������� ������������ ����� �������
            if (find(column_names.begin(), column_names.end(), column_name) != column_names.end()) {
                cout << "Error: Duplicate column name: " << column_name << endl;//���������� �������
                continue;
            }
            column_names.push_back(column_name);//��������� � ������

            //������������ ������� ��� (��� �������)
            string base_type = column_type_str;
            int size = 0;

            //��������� ���� �� ������ � �������
            size_t paren_open = column_type_str.find("(");
            if (paren_open != string::npos) {
                base_type = column_type_str.substr(0, paren_open);
                size_t paren_close = column_type_str.find(")");//���� ���� ������ ��������� ��� ��� � ������
                if (paren_close != string::npos) {
                    string size_str = column_type_str.substr(paren_open + 1, paren_close - paren_open - 1);
                    try {
                        size = stoi(size_str);
                    }
                    catch (const exception&) {
                        cout << "Error parsing CREATE TABLE" << endl;
                        continue;
                    }
                }
            }

            //����������� ������ ���� � enum
            All_types column_type_id = get_type_from_string(base_type);

            // �������� ��� VARCHAR ����� ������������� ������
            if (column_type_id == VARCHAR && size <= 0) {
                cout << "Error: VARCHAR must have a size for column " << column_name << endl;
                continue;
            }

            //��� CHAR
            if (column_type_id == CHAR && size <= 0) {
                size = 1;
            }

            //�������� � ���������� �������
            Column column = { column_name, column_type_id, size };
            query.columns.push_back(column);//� ������
            cout << "Column: " << column_name << " Type: " << base_type << " Size: " << size << endl;
        }

        if (query.columns.empty()) {
            cout << "Error: No valid columns defined" << endl;
        }

    }
    catch (const exception& e) {
        cout << "Error parsing CREATE TABLE: " << e.what() << endl;
    }

    return query;
}

//������� ���������� ���������� �������
void save_table_metadata(const CreateTableQuery& query) {
    serialize_metadata(query.table_name, query.columns, 0);
}

//������� �������� ���������� �������
bool load_table_metadata(const string& table_name, vector<Column>& columns) {
    uint64_t record_count;
    return deserialize_metadata(table_name, columns, record_count);
}

//������� ���������� ������ �� ����� 
void read_data_from_file(const string& filename, const vector<Column>& columns, vector<DynamicRecord>& records) {
    ifstream file(filename, ios::binary);//�������� �����

    if (file.is_open()) { 
        int record_count;
        file.read(reinterpret_cast<char*>(&record_count), sizeof(record_count));

        records.clear();

        for (int i = 0; i < record_count; ++i) {
            DynamicRecord record;

            //������ �������� ��� ������� ������� �������� ��� ����
            for (const auto& column : columns) {
                //��������� ������������� �����
                if (column.type == INT || column.type == TINYINT || column.type == SMALLINT || column.type == BIGINT) {
                    int int_value;
                    file.read(reinterpret_cast<char*>(&int_value), sizeof(int_value));
                    record.add_value(column.type, to_string(int_value));
                }
                //��������� ������� �����
                else if (column.type == FLOAT || column.type == REAL) {
                    float float_value;
                    file.read(reinterpret_cast<char*>(&float_value), sizeof(float_value));
                    record.add_value(column.type, to_string(float_value));
                }
                //��������� ��������� �����
                else if (column.type == VARCHAR || column.type == CHAR || column.type == TEXT) {
                    int string_size;
                    file.read(reinterpret_cast<char*>(&string_size), sizeof(string_size));

                    char* string_buffer = new char[string_size + 1];
                    file.read(string_buffer, string_size);
                    string_buffer[string_size] = '\0';
                    record.add_value(column.type, string(string_buffer));
                    delete[] string_buffer;
                }
                //��������� ���� � �������
                else if (column.type == DATETIME || column.type == SMALLDATETIME ||
                    column.type == DATE || column.type == TIME) {
                    // ��� ��������� ����� ������ ��� ������ (���������)
                    int data_size;
                    file.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));

                    char* buffer = new char[data_size + 1];
                    file.read(buffer, data_size);
                    buffer[data_size] = '\0';
                    record.add_value(column.type, string(buffer));
                    delete[] buffer;
                }
                //��������� BIT
                else if (column.type == BIT) {
                    char bit_value;
                    file.read(&bit_value, sizeof(bit_value));
                    record.add_value(column.type, to_string(bit_value));
                }
                //��������� ����������� �����
                else {
                    //��� ���������������� ������ ��� ������
                    int data_size;
                    file.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));

                    char* buffer = new char[data_size + 1];
                    file.read(buffer, data_size);
                    buffer[data_size] = '\0';
                    record.add_value(column.type, string(buffer));
                    delete[] buffer;
                }
            }

            records.push_back(record);
        }

        file.close();
    }
    else {
        cerr << "Error: unable to open file for reading." << endl;
    }
}

//������� ������ ������ � ����
void write_data_to_file(const string& filename, const vector<Column>& columns, vector<DynamicRecord>& records) {
    ofstream file(filename, ios::binary | ios::trunc);

    if (file.is_open()) {
        //������ ���������� �������
        int record_count = static_cast<int>(records.size());  
        file.write(reinterpret_cast<const char*>(&record_count), sizeof(record_count));

        //���� �� ���� �������
        for (const auto& record : records) {  
            //���� �� ���� �������� ������
            for (size_t i = 0; i < columns.size(); i++) { 
                const auto& column = columns[i];  
                const auto& cell = record.values[i];  

                //��������� ������������� �����
                if (column.type == INT || column.type == TINYINT || column.type == SMALLINT || column.type == BIGINT) {  
                    int int_value = record.get_int_value(static_cast<int>(i)); 
                    file.write(reinterpret_cast<const char*>(&int_value), sizeof(int_value));
                }
                //�������
                else if (column.type == FLOAT || column.type == REAL) {
                    float float_value = stof(cell.data);
                    file.write(reinterpret_cast<const char*>(&float_value), sizeof(float_value));
                }
                //���������
                else if (column.type == VARCHAR || column.type == CHAR || column.type == TEXT) {
                    int string_size = static_cast<int>(cell.data.size());  // ����� ����������
                    file.write(reinterpret_cast<const char*>(&string_size), sizeof(string_size));
                    file.write(cell.data.c_str(), string_size);
                }
                else if (column.type == BIT) {
                    char bit_value = static_cast<char>(stoi(cell.data));
                    file.write(&bit_value, sizeof(bit_value));
                }
                //�����������
                else {
                    int data_size = static_cast<int>(cell.data.size());  
                    file.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
                    file.write(cell.data.c_str(), data_size);
                }
            }
        }

        file.close();
    }
    else {
        cerr << "Error: unable to open file for writing." << endl;
    }
}

//������� ��������� ������� CREATE
void process_create_command(const string& command) {
    CreateTableQuery query = parse_create_table_query(command);//�������

    //��������� ���������� ��������
    if (query.table_name.empty() || query.columns.empty()) {
        cout << "Error: Invalid table definition" << endl;//��������� ��� ������ ������� ������ ��� ������� � ���� �� ���� �������
        return;
    }

    //�������� �� �������� �����
    if (is_keyword(query.table_name)) {
        cout << "Error: Table name '" << query.table_name << "' is a reserved SQL keyword." << endl;
        return;
    }

    //��������� �� ���������� �� ��� �������
    ifstream test_file(query.table_name + ".dat");
    if (test_file.good()) {
        cout << "Error: Table '" << query.table_name << "' already exists." << endl;
        test_file.close();
        return;
    }

    //������������� ��������
    cout << "Creating table: " << query.table_name << " with " << query.columns.size() << " columns" << endl;

    //������� ������ ���� ������ ��� �������
    vector<DynamicRecord> records;
    write_data_to_file(query.table_name + ".dat", query.columns, records);

    //��������� ����������
    save_table_metadata(query);

    cout << "Table created successfully!" << endl;
}

//������� ��������� ������� SELECT 
void process_select_command(const string& command) {
    string upper_command = command;
    transform(upper_command.begin(), upper_command.end(), upper_command.begin(), ::toupper);

    //��������� ��� �������
    string table_name;
    size_t from_pos = upper_command.find("FROM");//����� �������� ����� from
    if (from_pos != string::npos) {
        table_name = command.substr(from_pos + 4);//���������� ����� �������
        table_name.erase(0, table_name.find_first_not_of(" "));//������� ����� �������
        table_name.erase(table_name.find_last_not_of(" ;") + 1);
    }
    else {
        cout << "Error: Invalid SELECT syntax - missing FROM" << endl;//�������� ������� from
        return;
    }

    //�������� ����� �������
    if (table_name.empty()) {
        cout << "Error: Table name is empty" << endl;
        return;
    }

    cout << "Selecting from table: " << table_name << endl;

    //��������� ���������� �������
    vector<Column> table_columns;
    if (!load_table_metadata(table_name, table_columns)) { //��������� ��������� ������� �� .meta �����
        cout << "Error: Cannot load metadata for table '" << table_name << "'" << endl;
        cout << "Table might not exist or metadata is corrupted" << endl;
        return;
    }

    //������ ������ �������
    vector<DynamicRecord> records;
    read_data_from_file(table_name + ".dat", table_columns, records);

    if (records.empty()) {
        cout << "Table is empty" << endl;
        return;
    }

    //������� ��������� �������
    cout << "\n=== " << table_name << " ===" << endl;

    //������� �������� ��������
    for (const auto& column : table_columns) {
        cout << column.name << "\t| ";
    }
    cout << endl;

    //������� �����������
    for (int i = 0; i < table_columns.size(); i++) {
        cout << "--------\t| ";
    }
    cout << endl;

    //������� ������ �������
    for (const auto& record : records) {
        for (int i = 0; i < table_columns.size(); i++) {
            cout << record.get_string_value(i) << "\t| ";
        }
        cout << endl;
    }

    cout << "Total records: " << records.size() << endl;//����� ����������
}

struct WhereCondition {
    string column_name;
    string operator_; // "=", ">", "<", ">=", "<=", "!="
    string value;
};


bool check_where_condition(const DynamicRecord& record,
    const vector<Column>& columns,
    const WhereCondition& condition) {
    int column_index = -1;
    for (int i = 0; i < columns.size(); i++) {
        if (columns[i].name == condition.column_name) {
            column_index = i;
            break;
        }
    }

    if (column_index == -1) return false;

    string record_value = record.get_string_value(column_index);

    if (condition.operator_ == "=") {
        return record_value == condition.value;
    }
    else if (condition.operator_ == "!=") {
        return record_value != condition.value;
    }
    else if (condition.operator_ == ">") {
        return record.get_int_value(column_index) > stoi(condition.value);
    }
    else if (condition.operator_ == "<") {
        return record.get_int_value(column_index) < stoi(condition.value);
    }

    return false;
}

vector<pair<string, string>> parse_set_clause(const string& set_part) {
    vector<pair<string, string>> updates;
    stringstream ss(set_part);
    string assignment;

    while (getline(ss, assignment, ',')) {
        size_t eq_pos = assignment.find('=');
        if (eq_pos != string::npos) {
            string column = assignment.substr(0, eq_pos);
            string value = assignment.substr(eq_pos + 1);

            column.erase(0, column.find_first_not_of(" "));
            column.erase(column.find_last_not_of(" ") + 1);
            value.erase(0, value.find_first_not_of(" "));
            value.erase(value.find_last_not_of(" ") + 1);

            if (!value.empty() && value[0] == '\'' && value.back() == '\'') {
                value = value.substr(1, value.size() - 2);
            }

            updates.push_back({ column, value });
        }
    }
    return updates;
}

WhereCondition parse_where_clause(const string& where_part) {
    WhereCondition condition;
    vector<string> operators = { "=", "!=", ">", "<", ">=", "<=" };

    for (const auto& op : operators) {
        size_t op_pos = where_part.find(op);
        if (op_pos != string::npos) {
            condition.column_name = where_part.substr(0, op_pos);
            condition.value = where_part.substr(op_pos + op.length());
            condition.operator_ = op;

            condition.column_name.erase(0, condition.column_name.find_first_not_of(" "));
            condition.column_name.erase(condition.column_name.find_last_not_of(" ") + 1);
            condition.value.erase(0, condition.value.find_first_not_of(" "));
            condition.value.erase(condition.value.find_last_not_of(" ") + 1);

            if (!condition.value.empty() && condition.value[0] == '\'' && condition.value.back() == '\'') {
                condition.value = condition.value.substr(1, condition.value.size() - 2);
            }

            break;
        }
    }

    return condition;
}

void process_update_command(const string& command) {
    string upper_command = command;
    transform(upper_command.begin(), upper_command.end(), upper_command.begin(), ::toupper);

    size_t update_pos = upper_command.find("UPDATE");
    size_t set_pos = upper_command.find("SET");
    size_t where_pos = upper_command.find("WHERE");

    if (update_pos == string::npos || set_pos == string::npos) {
        cout << "Error: Invalid UPDATE syntax" << endl;
        return;
    }

    string table_name = command.substr(update_pos + 6, set_pos - (update_pos + 6));
    table_name.erase(0, table_name.find_first_not_of(" "));
    table_name.erase(table_name.find_last_not_of(" ") + 1);

    if (table_name.empty()) {
        cout << "Error: Table name is empty" << endl;
        return;
    }

    string set_part;
    if (where_pos != string::npos) {
        set_part = command.substr(set_pos + 3, where_pos - (set_pos + 3));
    }
    else {
        set_part = command.substr(set_pos + 3);
    }

    auto updates = parse_set_clause(set_part);

    WhereCondition where_condition;
    bool has_where = (where_pos != string::npos);
    if (has_where) {
        string where_part = command.substr(where_pos + 5);
        where_condition = parse_where_clause(where_part);
    }

    cout << "Updating table: " << table_name << endl;

    vector<Column> table_columns;
    if (!load_table_metadata(table_name, table_columns)) {
        cout << "Error: Cannot load metadata for table '" << table_name << "'" << endl;
        return;
    }

    vector<DynamicRecord> records;
    read_data_from_file(table_name + ".dat", table_columns, records);

    int updated_count = 0;

    for (auto& record : records) {
        if (has_where && !check_where_condition(record, table_columns, where_condition)) {
            continue;
        }

        for (const auto& update : updates) {
            int column_index = -1;
            for (int i = 0; i < table_columns.size(); i++) {
                if (table_columns[i].name == update.first) {
                    column_index = i;
                    break;
                }
            }

            if (column_index != -1 && column_index < record.values.size()) {
                record.values[column_index].data = update.second;
                updated_count++;
            }
        }
    }

    write_data_to_file(table_name + ".dat", table_columns, records);
    cout << "Updated " << updated_count << " records successfully!" << endl;
}


void process_delete_command(const string& command) {
    string upper_command = command;
    transform(upper_command.begin(), upper_command.end(), upper_command.begin(), ::toupper);

    size_t from_pos = upper_command.find("FROM");
    size_t where_pos = upper_command.find("WHERE");

    if (from_pos == string::npos) {
        cout << "Error: Invalid DELETE syntax" << endl;
        return;
    }

    string table_name;
    if (where_pos != string::npos) {
        table_name = command.substr(from_pos + 4, where_pos - (from_pos + 4));
    }
    else {
        table_name = command.substr(from_pos + 4);
    }

    table_name.erase(0, table_name.find_first_not_of(" "));
    table_name.erase(table_name.find_last_not_of(" ;") + 1);

    if (table_name.empty()) {
        cout << "Error: Table name is empty" << endl;
        return;
    }

    WhereCondition where_condition;
    bool has_where = (where_pos != string::npos);
    if (has_where) {
        string where_part = command.substr(where_pos + 5);
        where_condition = parse_where_clause(where_part);
    }

    cout << "Deleting from table: " << table_name << endl;

    vector<Column> table_columns;
    if (!load_table_metadata(table_name, table_columns)) {
        cout << "Error: Cannot load metadata for table '" << table_name << "'" << endl;
        return;
    }

    vector<DynamicRecord> records;
    read_data_from_file(table_name + ".dat", table_columns, records);

    vector<DynamicRecord> remaining_records;
    int deleted_count = 0;

    for (const auto& record : records) {
        if (has_where && check_where_condition(record, table_columns, where_condition)) {
            deleted_count++;
        }
        else {
            remaining_records.push_back(record);
        }
    }

    write_data_to_file(table_name + ".dat", table_columns, remaining_records);
    cout << "Deleted " << deleted_count << " records successfully!" << endl;
}

int parse() {
    string command; 

    while (true) {
        cout << "\n=== Almost SQLite ===" << endl;
        cout << "Available commands:" << endl;
        cout << "1. CREATE TABLE table_name (col1 TYPE, col2 TYPE, ...)" << endl;
        cout << "2. SELECT * FROM table_name" << endl;
        cout << "3. UPDATE table_name SET col = value WHERE condition" << endl;
        cout << "4. DELETE FROM table_name WHERE condition" << endl;
        cout << "5. EXIT" << endl;
        cout << "Enter your SQL command (or 'EXIT' to quit): ";

        getline(cin, command);

        if (!command.empty() && command.back() == ';') { 
            command.pop_back();
        }

        //�����
        if (command == "EXIT" || command == "exit") {
            cout << "Goodbye!" << endl;
            break;
        }

        //������� ������ ������
        if (command.empty()) continue;

        //����������� ���� �������
        int command_type = parse_command(command);

        //��������� ������ � ����� try-catch
        try {
            if (command_type == 0) {
                process_create_command(command);
            }
            else if (command_type == 1) {
                process_select_command(command);
            }
            else if (command_type == 2) {
                process_update_command(command);
            }
            else if (command_type == 3) {
                process_delete_command(command);
            }
            else {
                cout << "Error: Invalid or unsupported command!" << endl;
                cout << "Supported commands: CREATE, SELECT, UPDATE, DELETE" << endl;
            }
        }
        catch (const exception& e) {
            cout << "Error executing command: " << e.what() << endl;
        }
    }

    return 0;
}
int main() {
    return parse();
}