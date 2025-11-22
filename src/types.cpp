#include "types.h"

data_node* init(All_types type, int size) {
    data_node* root = (data_node*)malloc(sizeof(data_node));
    if (root == NULL) {
        return NULL;
    }
    *root = { type, size, NULL };
    return root;
}

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

void destroy(data_node* root) {
    while (root != NULL) {
        data_node* temp = root;
        root = root->next;
        free(temp);
    }
}

int get_type_size(All_types column, int size) {
    switch (column) {
    case All_types::BIT: return 1;
    case All_types::TINYINT: return 1;
    case All_types::SMALLINT: return 2;
    case All_types::INT_TYPE: return 4;  
    case All_types::BIGINT: return 8;
    case All_types::FLOAT_TYPE: return 4; 
    case All_types::REAL: return 8;
    case All_types::DATETIME: return 8;
    case All_types::SMALLDATETIME: return 4;
    case All_types::DATE_TYPE: return 3; 
    case All_types::TIME: return 5;
    case All_types::CHAR_TYPE: return size + 2;  
    case All_types::VARCHAR: return size + 2;
    case All_types::TEXT: return size;
    default: return -1;
    }
}

All_types get_type_from_string(const string& type_str) {
    string upper_type = type_str;
    transform(upper_type.begin(), upper_type.end(), upper_type.begin(), ::toupper);

    if (upper_type == "BIT") return All_types::BIT;
    if (upper_type == "TINYINT") return All_types::TINYINT;
    if (upper_type == "SMALLINT") return All_types::SMALLINT;
    if (upper_type == "INT") return All_types::INT_TYPE;        
    if (upper_type == "BIGINT") return All_types::BIGINT;
    if (upper_type == "FLOAT") return All_types::FLOAT_TYPE;    
    if (upper_type == "REAL") return All_types::REAL;
    if (upper_type == "DATETIME") return All_types::DATETIME;
    if (upper_type == "SMALLDATETIME") return All_types::SMALLDATETIME;
    if (upper_type == "DATE") return All_types::DATE_TYPE;      
    if (upper_type == "TIME") return All_types::TIME;
    if (upper_type == "CHAR") return All_types::CHAR_TYPE;      
    if (upper_type == "VARCHAR") return All_types::VARCHAR;
    if (upper_type == "TEXT") return All_types::TEXT;

    cout << "Warning: Unknown type '" << type_str << "', using TINYINT as default" << endl;
    return All_types::TINYINT; :
}

int get_varchar_size(const string& type_str) {
    size_t start = type_str.find("(");
    size_t end = type_str.find(")");
    if (start == string::npos || end == string::npos) {
        return -1;
    }
    string size_str = type_str.substr(start + 1, end - start - 1);
    try {
        return stoi(size_str);
    }
    catch (const exception&) {
        return -1;
    }
}