#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <windows.h>

using namespace std;

typedef struct {
    char column_name[50];
    int data_type_id;
} AlmostSeq;

typedef struct {
    char table_name[50];
    int byte_count;
    char restriction[20];  // "unique", "primary_key", "check"
} AlmostRestr;

typedef struct {
    char table1_name[50];
    char column1_name[50];
    char table2_name[50];
    char column2_name[50];
} AlmostRelate;

void initialize_file(const char* filename) {
    fstream file(filename, ios::out | ios::app | ios::binary);
    if (file.is_open()) {
        file.close();
        cout << "Файл " << filename << " инициализирован" << endl;
    }
}

bool add_record(const char* filename, void* record, size_t record_size) {
    fstream file(filename, ios::out | ios::app | ios::binary);
    if (!file.is_open()) {
        cout << "Ошибка открытия файла " << filename << endl;
        return false;
    }
    
    file.write(static_cast<char*>(record), record_size);
    file.close();
    return true;
}

vector<char> read_all_records(const char* filename, size_t record_size, int* count) {
    fstream file(filename, ios::in | ios::binary);
    vector<char> buffer;
    
    if (!file.is_open()) {
        *count = 0;
        return buffer;
    }
    
    file.seekg(0, ios::end);
    streampos file_size = file.tellg();
    file.seekg(0, ios::beg);
    
    *count = file_size / record_size;
    
    if (*count > 0) {
        buffer.resize(file_size);
        file.read(buffer.data(), file_size);
    }
    
    file.close();
    return buffer;
}

bool seq_exists(const char* column_name) {
    int count;
    vector<char> buffer = read_all_records("almost_seq.bin", sizeof(AlmostSeq), &count);
    
    if (count == 0) return false;
    
    AlmostSeq* records = reinterpret_cast<AlmostSeq*>(buffer.data());
    for (int i = 0; i < count; i++) {
        if (strcmp(records[i].column_name, column_name) == 0) {
            return true;
        }
    }
    return false;
}

bool restr_exists(const char* table_name, const char* restriction) {
    int count;
    vector<char> buffer = read_all_records("almost_restr.bin", sizeof(AlmostRestr), &count);
    
    if (count == 0) return false;
    
    AlmostRestr* records = reinterpret_cast<AlmostRestr*>(buffer.data());
    for (int i = 0; i < count; i++) {
        if (strcmp(records[i].table_name, table_name) == 0 &&
            strcmp(records[i].restriction, restriction) == 0) {
            return true;
        }
    }
    return false;
}

bool relate_exists(const char* table1, const char* column1, const char* table2, const char* column2) {
    int count;
    vector<char> buffer = read_all_records("almost_relate.bin", sizeof(AlmostRelate), &count);
    
    if (count == 0) return false;
    
    AlmostRelate* records = reinterpret_cast<AlmostRelate*>(buffer.data());
    for (int i = 0; i < count; i++) {
        if (strcmp(records[i].table1_name, table1) == 0 &&
            strcmp(records[i].column1_name, column1) == 0 &&
            strcmp(records[i].table2_name, table2) == 0 &&
            strcmp(records[i].column2_name, column2) == 0) {
            return true;
        }
    }
    return false;
}

void create_seq() {
    AlmostSeq seq;
    cout << "Введите название колонки: ";
    cin >> seq.column_name;
    
    if (seq_exists(seq.column_name)) {
        cout << "Ошибка: последовательность для колонки '" << seq.column_name << "' уже существует!" << endl;
        return;
    }
    
    cout << "Введите ID типа данных: ";
    cin >> seq.data_type_id;
    
    if (add_record("almost_seq.bin", &seq, sizeof(AlmostSeq))) {
        cout << "Последовательность успешно создана!" << endl;
    } else {
        cout << "Ошибка создания последовательности!" << endl;
    }
}

void view_all_seq() {
    int count;
    vector<char> buffer = read_all_records("almost_seq.bin", sizeof(AlmostSeq), &count);
    
    cout << "\n=== Все последовательности almost_seq ===" << endl;
    if (count == 0) {
        cout << "Записей нет" << endl;
        return;
    }
    
    AlmostSeq* records = reinterpret_cast<AlmostSeq*>(buffer.data());
    for (int i = 0; i < count; i++) {
        cout << i+1 << ". Колонка: " << records[i].column_name 
             << ", Тип данных ID: " << records[i].data_type_id << endl;
    }
    cout << "Всего записей: " << count << endl;
}

void update_seq() {
    char column_name[50];
    int new_data_type_id;
    
    cout << "Введите название колонки для обновления: ";
    cin >> column_name;
    
    if (!seq_exists(column_name)) {
        cout << "Ошибка: последовательность для колонки '" << column_name << "' не найдена!" << endl;
        return;
    }
    
    cout << "Введите новый ID типа данных: ";
    cin >> new_data_type_id;
    
    int count;
    vector<char> buffer = read_all_records("almost_seq.bin", sizeof(AlmostSeq), &count);
    AlmostSeq* records = reinterpret_cast<AlmostSeq*>(buffer.data());
    
    bool found = false;
    for (int i = 0; i < count; i++) {
        if (strcmp(records[i].column_name, column_name) == 0) {
            records[i].data_type_id = new_data_type_id;
            found = true;
            break;
        }
    }
    
    if (found) {
        fstream file("almost_seq.bin", ios::out | ios::binary);
        if (file.is_open()) {
            file.write(buffer.data(), buffer.size());
            file.close();
            cout << "Тип данных обновлен!" << endl;
        } else {
            cout << "Ошибка открытия файла для записи!" << endl;
        }
    }
}

void delete_seq() {
    char column_name[50];
    
    cout << "Введите название колонки для удаления: ";
    cin >> column_name;
    
    if (!seq_exists(column_name)) {
        cout << "Ошибка: последовательность для колонки '" << column_name << "' не найдена!" << endl;
        return;
    }
    
    int count;
    vector<char> buffer = read_all_records("almost_seq.bin", sizeof(AlmostSeq), &count);
    AlmostSeq* records = reinterpret_cast<AlmostSeq*>(buffer.data());
    
    fstream file("almost_seq.bin", ios::out | ios::binary);
    if (!file.is_open()) {
        cout << "Ошибка открытия файла!" << endl;
        return;
    }
    
    bool deleted = false;
    for (int i = 0; i < count; i++) {
        if (strcmp(records[i].column_name, column_name) != 0) {
            file.write(reinterpret_cast<char*>(&records[i]), sizeof(AlmostSeq));
        } else {
            deleted = true;
        }
    }
    file.close();
    
    if (deleted) {
        cout << "Последовательность удалена!" << endl;
    }
}

void create_restr() {
    AlmostRestr restr;
    cout << "Введите название таблицы: ";
    cin >> restr.table_name;
    cout << "Введите количество байт: ";
    cin >> restr.byte_count;
    cout << "Введите ограничение (unique/primary_key/check): ";
    cin >> restr.restriction;
    
    if (restr_exists(restr.table_name, restr.restriction)) {
        cout << "Ошибка: ограничение '" << restr.restriction << "' для таблицы '" 
             << restr.table_name << "' уже существует!" << endl;
        return;
    }
    
    if (add_record("almost_restr.bin", &restr, sizeof(AlmostRestr))) {
        cout << "Ограничение создано!" << endl;
    } else {
        cout << "Ошибка создания ограничения!" << endl;
    }
}

void view_all_restr() {
    int count;
    vector<char> buffer = read_all_records("almost_restr.bin", sizeof(AlmostRestr), &count);
    
    cout << "\n=== Все ограничения ===" << endl;
    if (count == 0) {
        cout << "Ограничений нет" << endl;
        return;
    }
    
    AlmostRestr* records = reinterpret_cast<AlmostRestr*>(buffer.data());
    for (int i = 0; i < count; i++) {
        cout << i+1 << ". Таблица: " << records[i].table_name 
             << ", Байт: " << records[i].byte_count 
             << ", Ограничение: " << records[i].restriction << endl;
    }
    cout << "Всего ограничений: " << count << endl;
}
void create_relate() {
    AlmostRelate relate;
    cout << "Введите таблицу 1: ";
    cin >> relate.table1_name;
    cout << "Введите колонку 1: ";
    cin >> relate.column1_name;
    cout << "Введите таблицу 2: ";
    cin >> relate.table2_name;
    cout << "Введите колонку 2: ";
    cin >> relate.column2_name;
    
    if (relate_exists(relate.table1_name, relate.column1_name, relate.table2_name, relate.column2_name)) {
        cout << "Ошибка: отношение уже существует!" << endl;
        return;
    }
    
    if (add_record("almost_relate.bin", &relate, sizeof(AlmostRelate))) {
        cout << "Отношение создано!" << endl;
    } else {
        cout << "Ошибка создания отношения!" << endl;
    }
}

void view_all_relate() {
    int count;
    vector<char> buffer = read_all_records("almost_relate.bin", sizeof(AlmostRelate), &count);
    
    cout << "\n=== Все отношения ===" << endl;
    if (count == 0) {
        cout << "Отношений нет" << endl;
        return;
    }
    
    AlmostRelate* records = reinterpret_cast<AlmostRelate*>(buffer.data());
    for (int i = 0; i < count; i++) {
        cout << i+1 << ". " << records[i].table1_name << "." << records[i].column1_name 
             << " -> " << records[i].table2_name << "." << records[i].column2_name << endl;
    }
    cout << "Всего отношений: " << count << endl;
}
void seq_menu() {
    int choice;
    do {
        cout << "\n=== ALMOST_SEQ - Управление последовательностями ===" << endl;
        cout << "1. Создать последовательность" << endl;
        cout << "2. Просмотреть все последовательности" << endl;
        cout << "3. Обновить тип данных" << endl;
        cout << "4. Удалить последовательность" << endl;
        cout << "0. Назад" << endl;
        cout << "Выберите действие: ";
        cin >> choice;
        
        switch(choice) {
            case 1: create_seq(); break;
            case 2: view_all_seq(); break;
            case 3: update_seq(); break;
            case 4: delete_seq(); break;
            case 0: cout << "Возврат в главное меню..." << endl; break;
            default: cout << "Неверный выбор!" << endl;
        }
    } while(choice != 0);
}

void restr_menu() {
    int choice;
    do {
        cout << "\n=== ALMOST_RESTR - Управление ограничениями ===" << endl;
        cout << "1. Создать ограничение" << endl;
        cout << "2. Просмотреть все ограничения" << endl;
        cout << "0. Назад" << endl;
        cout << "Выберите действие: ";
        cin >> choice;
        
        switch(choice) {
            case 1: create_restr(); break;
            case 2: view_all_restr(); break;
            case 0: cout << "Возврат в главное меню..." << endl; break;
            default: cout << "Неверный выбор!" << endl;
        }
    } while(choice != 0);
}

void relate_menu() {
    int choice;
    do {
        cout << "\n=== ALMOST_RELATE - Управление отношениями ===" << endl;
        cout << "1. Создать отношение" << endl;
        cout << "2. Просмотреть все отношения" << endl;
        cout << "0. Назад" << endl;
        cout << "Выберите действие: ";
        cin >> choice;
        
        switch(choice) {
            case 1: create_relate(); break;
            case 2: view_all_relate(); break;
            case 0: cout << "Возврат в главное меню..." << endl; break;
            default: cout << "Неверный выбор!" << endl;
        }
    } while(choice != 0);
}

void show_main_menu() {
    cout << "\n=== СИСТЕМА УПРАВЛЕНИЯ СЛУЖЕБНЫМИ ТАБЛИЦАМИ ===" << endl;
    cout << "1. almost_seq - Управление последовательностями" << endl;
    cout << "2. almost_restr - Управление ограничениями" << endl;
    cout << "3. almost_relate - Управление отношениями" << endl;
    cout << "4. Просмотр всех данных" << endl;
    cout << "0. Выход" << endl;
    cout << "Выберите действие: ";
}

void view_all_data() {
    cout << "\n=== ВСЕ ДАННЫЕ ИЗ СЛУЖЕБНЫХ ТАБЛИЦ ===" << endl;
    view_all_seq();
    view_all_restr();
    view_all_relate();
}

int main() {
    SetConsoleOutputCP(65001);
    
    // Инициализация файлов
    initialize_file("almost_seq.bin");
    initialize_file("almost_restr.bin");
    initialize_file("almost_relate.bin");
    
    int choice;
    do {
        show_main_menu();
        cin >> choice;
        
        switch(choice) {
            case 1: seq_menu(); break;
            case 2: restr_menu(); break;
            case 3: relate_menu(); break;
            case 4: view_all_data(); break;
            case 0: cout << "Выход из программы..." << endl; break;
            default: cout << "Неверный выбор!" << endl;
        }
    } while(choice != 0);
    
    return 0;
}