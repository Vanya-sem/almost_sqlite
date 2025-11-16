#include "types.h"

//������������� 1��� ���� ������
data_node* init(All_types type, int size) {
    data_node* root = (data_node*)malloc(sizeof(data_node));
    if (root == NULL) {
        return NULL;  
    }
    *root = { type, size, NULL };
    return root;
}

//���������� ������ ����
data_node* append(data_node* root, All_types type, int size) {
    if (root == NULL) {
        root = init(type, size);
    }
    else {
        data_node* current = root;
        while (current->next != NULL) {
            current = current->next;
        }

        data_node* new_node = (data_node*)malloc(sizeof(data_node));
        if (new_node == NULL) {
            return root; 
        }
        *new_node = { type, size, NULL };
        current->next = new_node;
    }
    return root;
}

//������������ ������ ������
void destroy(data_node* root) {
    while (root != NULL) {//�� ����� ������
        data_node* temp = root; //��������� �� ��������� ����������
        root = root->next;
        free(temp);
    }
}

//������ ���� ������ � ������
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
    case CHAR: return size + 2; //���������� ������ ������� size 
    case VARCHAR: return size + 2;
    case TEXT: return size;
    default: return -1;
    }
}

//������ � ��� ������
All_types get_type_from_string(const string& type_str) {
    string upper_type = type_str;//���������� � �������� ��������
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

    cout << "Warning: Unknown type '" << type_str << "', using TINYINT as default" << endl;//����������� ��� ���������� �� ���������
    return TINYINT;
}

//�������� ������ �� VARCHAR
int get_varchar_size(const string& type_str) {
    size_t start = type_str.find("(");//����� ������
    size_t end = type_str.find(")");
    if (start == string::npos || end == string::npos) {//������ �� �������
        return -1;
    }
    string size_str = type_str.substr(start + 1, end - start - 1);//��������� � �������� (������� � ������� ����� ������)
    try {//��������� ������ ��������������
        return stoi(size_str);
    }
    catch (const exception&) {
        return -1;
    }
}