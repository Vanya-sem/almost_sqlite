#include <iostream>
#include <fstream>
#include <stdlib.h>

typedef enum All_types {
	BIT, //id = 0
	TINYINT, //id = 1
	SMALLINT, //id = 2
	INT, //id = 3
	BIGINT, //id = 4
	FLOAT, //id = 5
	REAL, //id = 6
	DATETIME, //id = 7
	SMALLDATETIME, //id = 8
	DATE, //id = 9
	TIME, //id = 10
	CHAR, //id = 11
	VARCHAR, //id = 12
	TEXT //id = 13
} All_types;

typedef struct data_list_node data_node;
struct data_list_node {
	char id;
	int size;
	struct data_list_node* next;
};

data_node* init(char id, int size);
data_node* append(data_node* root, char id, int size);
void destroy(data_node* root);

#pragma once
