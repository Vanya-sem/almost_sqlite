#include "types.h"

//инициализация 1ого узла списка
data_node* init(All_types type, int size) {
    data_node* root = (data_node*)malloc(sizeof(data_node));//выделение памяти
    *root = { type, size, NULL };
    return root;
}

//добавления нового узла
data_node* append(data_node* root, All_types type, int size) {
    if (root == NULL) { //проверка на пустой список
        root = init(type, size);
    }
    else { //поиск конца списка
        data_node* current = root;
        while (current->next != NULL) {
            current = current->next;
        }

        data_node* new_node = (data_node*)malloc(sizeof(data_node));//создание нового узла
        *new_node = { type, size, NULL };
        current->next = new_node;
    }
    return root;
}

//освобождение памяти списка
void destroy(data_node* root) {
    while (root != NULL) {//до конца списка
        data_node* temp = root; //указатель во временной переменной
        root = root->next;
        free(temp);
    }
}

//размер типа данных в байтах
int get_type_size(All_types column, int size) {
    switch (column) {
    case BIT: return 1;
    case TINYINT: return 1;
    case SMALLINT: return 2;
    case INT: return 4;
    case BIGINT: return 8;
    case FLOAT: return 4;
    case REAL: return 8;
    case DATETIME: return 8;
    case SMALLDATETIME: return 4;
    case DATE: return 3;
    case TIME: return 5;
    case CHAR: return size + 2; //переменный размер поэтому size 
    case VARCHAR: return size + 2;
    case TEXT: return size;
    default: return -1;
    }
}

//строки в тип данных
All_types get_type_from_string(const string& type_str) {
    string upper_type = type_str;//приведение к верхнему регистру
    transform(upper_type.begin(), upper_type.end(), upper_type.begin(), ::toupper);

    if (upper_type == "BIT") return BIT;
    if (upper_type == "TINYINT") return TINYINT;
    if (upper_type == "SMALLINT") return SMALLINT;
    if (upper_type == "INT") return INT;
    if (upper_type == "BIGINT") return BIGINT;
    if (upper_type == "FLOAT") return FLOAT;
    if (upper_type == "REAL") return REAL;
    if (upper_type == "DATETIME") return DATETIME;
    if (upper_type == "SMALLDATETIME") return SMALLDATETIME;
    if (upper_type == "DATE") return DATE;
    if (upper_type == "TIME") return TIME;
    if (upper_type == "CHAR") return CHAR;
    if (upper_type == "VARCHAR") return VARCHAR;
    if (upper_type == "TEXT") return TEXT;

    cout << "Warning: Unknown type '" << type_str << "', using TINYINT as default" << endl;//неизвестный тип возвращаем по умолчанию
    return TINYINT;
}

//числовой размер из VARCHAR
int get_varchar_size(const string& type_str) {
    size_t start = type_str.find("(");//поиск скобок
    size_t end = type_str.find(")");
    if (start == string::npos || end == string::npos) {//скобки не найдены
        return -1;
    }
    string size_str = type_str.substr(start + 1, end - start - 1);//подстрока с размером (начинам с символа после скобки)
    try {//обработка ошибок преобразования
        return stoi(size_str);
    }
    catch (const exception& e) {
        return -1;
    }
}