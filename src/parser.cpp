#include <cstring> //c����� (strlen � ��)
#include <sstream> //��������� ������ (getline � ��)
#include <vector> //������� ����
#include <algorithm> //������ � ������������ (find, transform � ��)
#include <cctype> //������������� �������� (::toupper, ::isdigit � ��)
#include "types.h"
#include <iostream>
#include <fstream>

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

//��������� ������� (������������� ������ � �������)
struct Column {
    string name;
    All_types type;
    int size; //������ (VARCHAR)
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
                    catch (const exception& e) {
                        cout << "Error: Invalid size for column " << column_name << ": " << size_str << endl;
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

        //�������� ������� ��������
        if (query.columns.empty()) {
            cout << "Error: No valid columns defined" << endl;
        }

        //��������� ����������
    }
    catch (const exception& e) {
        cout << "Error parsing CREATE TABLE: " << e.what() << endl;
    }

    return query;
}

//������� ���������� ���������� �������
void save_table_metadata(const CreateTableQuery& query) {
    //��������� ���������� � ��������� ����
    ofstream meta_file(query.table_name + ".meta", ios::binary);
    //�������� �������� �����
    if (meta_file.is_open()) {
        int column_count = query.columns.size();//���������� ��� ��������
        meta_file.write(reinterpret_cast<const char*>(&column_count), sizeof(column_count));

        //���� �� ���� ��������
        for (const auto& column : query.columns) {
            //��������� ��� �������
            int name_size = column.name.size();
            meta_file.write(reinterpret_cast<const char*>(&name_size), sizeof(name_size));//������
            meta_file.write(column.name.c_str(), name_size);//������

            //��������� ��� � ������
            meta_file.write(reinterpret_cast<const char*>(&column.type), sizeof(column.type));
            meta_file.write(reinterpret_cast<const char*>(&column.size), sizeof(column.size));
        }
        meta_file.close();
    }
}

//������� �������� ���������� �������
bool load_table_metadata(const string& table_name, vector<Column>& columns) {
    ifstream meta_file(table_name + ".meta", ios::binary);
    if (!meta_file.is_open()) { //�������� �������� �����
        return false;
    }

    //������ ��� ��������
    int column_count;
    meta_file.read(reinterpret_cast<char*>(&column_count), sizeof(column_count));

    //���������� ������� ��������
    columns.clear();
    //������ �������
    for (int i = 0; i < column_count; i++) {
        Column column;

        //������ ��� �������
        int name_size;
        meta_file.read(reinterpret_cast<char*>(&name_size), sizeof(name_size));
        char* name_buffer = new char[name_size + 1];//�������� ������ (��� ������ +1 ����)
        meta_file.read(name_buffer, name_size);
        name_buffer[name_size] = '\0';//��������� ����-���������� � ����� ������
        column.name = string(name_buffer);
        delete[] name_buffer;

        //������ ��� � ������ �������
        meta_file.read(reinterpret_cast<char*>(&column.type), sizeof(column.type));
        meta_file.read(reinterpret_cast<char*>(&column.size), sizeof(column.size));

        columns.push_back(column);//���������� � ������
    }

    meta_file.close();
    return true;
}

//������� ���������� ������ �� ����� 
void read_data_from_file(const string& filename, const vector<Column>& columns, vector<DynamicRecord>& records) { //��������� ���������� ������ records
    ifstream file(filename, ios::binary);//�������� �����

    if (file.is_open()) { //�������� �������� �����
        int record_count;
        file.read(reinterpret_cast<char*>(&record_count), sizeof(record_count));

        records.clear();//���������� ������ �������

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
                //�������������������� �����
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
        int record_count = records.size();
        file.write(reinterpret_cast<const char*>(&record_count), sizeof(record_count));

        //���� �� ���� �������
        for (const auto& record : records) {
            //���� �� ���� �������� ������
            for (int i = 0; i < columns.size(); i++) {
                const auto& column = columns[i];
                const auto& cell = record.values[i];

                //��������� ������������� �����
                if (column.type == INT || column.type == TINYINT || column.type == SMALLINT || column.type == BIGINT) {
                    int int_value = record.get_int_value(i);
                    file.write(reinterpret_cast<const char*>(&int_value), sizeof(int_value));
                }
                //�������
                else if (column.type == FLOAT || column.type == REAL) {
                    float float_value = stof(cell.data);
                    file.write(reinterpret_cast<const char*>(&float_value), sizeof(float_value));
                }
                //���������
                else if (column.type == VARCHAR || column.type == CHAR || column.type == TEXT) {
                    int string_size = cell.data.size();
                    file.write(reinterpret_cast<const char*>(&string_size), sizeof(string_size));
                    file.write(cell.data.c_str(), string_size);
                }
                //�����������
                else {
                    int data_size = cell.data.size();
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

//������� ��������� ������� UPDATE
void process_update_command(const string& command) {
    string upper_command = command;
    transform(upper_command.begin(), upper_command.end(), upper_command.begin(), ::toupper);

    //����� �������� �������
    size_t update_pos = upper_command.find("UPDATE");
    size_t set_pos = upper_command.find("SET");
    size_t where_pos = upper_command.find("WHERE");

    //�������� ����������
    if (update_pos == string::npos || set_pos == string::npos) {
        cout << "Error: Invalid UPDATE syntax" << endl;
        return;
    }

    string table_name = command.substr(update_pos + 6, set_pos - (update_pos + 6));//���������� ����� �������
    table_name.erase(0, table_name.find_first_not_of(" "));//�������
    table_name.erase(table_name.find_last_not_of(" ") + 1);

    if (table_name.empty()) { //�������� ����� �������
        cout << "Error: Table name is empty" << endl;
        return;
    }

    cout << "Updating table: " << table_name << endl;

    //��������� ���������� �������
    vector<Column> table_columns;
    if (!load_table_metadata(table_name, table_columns)) {
        cout << "Error: Cannot load metadata for table '" << table_name << "'" << endl;
        return;
    }

    //������ ������ �������
    vector<DynamicRecord> records;
    read_data_from_file(table_name + ".dat", table_columns, records);

    //����� ������� set wehere , ���� ���
    if (!records.empty()) {
        //��������� ������ ������ (��������� ����������)
        if (records[0].values.size() > 1) {
            records[0].values[1].data = "UpdatedName";
        }
        write_data_to_file(table_name + ".dat", table_columns, records);
        cout << "Table updated successfully!" << endl;
    }
}

//������� ��������� ������� DELETE
void process_delete_command(const string& command) {
    string upper_command = command;
    transform(upper_command.begin(), upper_command.end(), upper_command.begin(), ::toupper);

    size_t from_pos = upper_command.find("FROM");
    size_t where_pos = upper_command.find("WHERE");

    //�������� ����������
    if (from_pos == string::npos) {
        cout << "Error: Invalid DELETE syntax" << endl;
        return;
    }

    //���������� ����� �������
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

    cout << "Deleting from table: " << table_name << endl;

    // ��������� ���������� �������
    vector<Column> table_columns;
    if (!load_table_metadata(table_name, table_columns)) {
        cout << "Error: Cannot load metadata for table '" << table_name << "'" << endl;
        return;
    }

    vector<DynamicRecord> records;
    read_data_from_file(table_name + ".dat", table_columns, records);

    if (!records.empty()) {
        records.clear(); // � �������� ���������� ����� ������ ���� �������� ������
        write_data_to_file(table_name + ".dat", table_columns, records);
        cout << "All records deleted from table!" << endl;
    }
    else {
        cout << "Table is already empty" << endl;
    }
}

int parse() {
    string command; //������ SQL ������� ��������� �������������

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

        //������� ����� � ������� � ����� ���� ����
        if (!command.empty() && command.back() == ';') { //��������� ��� ������� �� ������ � ������������� �� ;
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