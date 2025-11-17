#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <cstdint>
#include <vector>

using namespace std;

//��� ���� ������
typedef enum All_types {
    BIT = 0,            // id = 0
    TINYINT = 1,        // id = 1
    SMALLINT = 2,       // id = 2
    INT = 3,            // id = 3
    BIGINT = 4,         // id = 4
    FLOAT = 5,          // id = 5
    REAL = 6,           // id = 6
    DATETIME = 7,       // id = 7
    SMALLDATETIME = 8,  // id = 8
    DATE = 9,           // id = 9
    TIME = 10,          // id = 10
    CHAR = 11,          // id = 11
    VARCHAR = 12,       // id = 12
    TEXT = 13           // id = 13 
} All_types;

struct Column {
    string name;
    All_types type = INT; 
    int size = 0;          
};

//���� ������������ ������
struct data_list_node {
    All_types type;  //id 
    int size;        //������ ������
    struct data_list_node* next;
};

typedef struct data_list_node data_node;

data_node* init(All_types type, int size);
data_node* append(data_node* root, All_types type, int size);
void destroy(data_node* root);
int get_type_size(All_types column, int size = -1);
All_types get_type_from_string(const string& type_str);
int get_varchar_size(const string& type_str);