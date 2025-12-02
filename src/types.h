#pragma once
#define _CRT_SECURE_NO_WARNINGS

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <cstdint>
#include <vector>

using namespace std;

enum class All_types {
    BIT = 0, // 1 bit 
    TINYINT = 1,
    SMALLINT = 2,
    INT = 3,       
    BIGINT = 4,
    FLOAT = 5,     
    REAL = 6,
    DATETIME = 7,
    SMALLDATETIME = 8,
    DATE = 9,       
    TIME = 10,
    CHAR = 11,     
    VARCHAR = 12,
    TEXT = 13
};

struct Column {
    string name;
    All_types type; 
    int size = 0;
};

struct data_list_node {
    All_types type;
    int size;
    struct data_list_node* next;
};

typedef struct data_list_node data_node;

data_node* init(All_types type, int size);
data_node* append(data_node* root, All_types type, int size);
void destroy(data_node* root);
int get_type_size(All_types column, int size = -1);
All_types get_type_from_string(const string& type_str);
int get_varchar_size(const string& type_str);