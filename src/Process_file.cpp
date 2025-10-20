#include <iostream>
#include <fstream>
#include "types.h"

#define HEADER "Almost SQLite1"
#define PAGE_SIZE 4096
#define IS_BTREE 0
#define UNICODE 1 //1 = UTF-8; 2 = UTF-16; 3 = UTF-32
#define ENGINE_VERSION 1
#define RAW_METADATA 23
#define METADATA_SIZE 50

using namespace std;

void make_file(char* filepath, char read, char write, unsigned int page_size = PAGE_SIZE) {
	ofstream file(filepath, ios::binary);
	if (file) {
		file.write(HEADER, strlen(HEADER)); //14 bytes
		char is_btree = IS_BTREE;
		char unicode = UNICODE;
		char engine = ENGINE_VERSION;
		file.write(reinterpret_cast<const char*>(&page_size), sizeof(page_size)); //4 bytes
		file.write(&is_btree, sizeof(char)); //1 byte
		file.write(&unicode, sizeof(char)); //1 byte
		file.write(&engine, sizeof(char)); //1 byte
		file.write(&read, sizeof(char)); //1 byte
		file.write(&write, sizeof(char)); //1 byte

		for (int i = 0; i < METADATA_SIZE - RAW_METADATA; i++) file.write(0, sizeof(char)); //27 reserve bytes
		// Metadata of binary file is 50 bytes
		file.close();
	}
	else cout << "Error! File is not open!";
}


void make_page_meta(char* filepath, unsigned int page_size = PAGE_SIZE) {
	short int page_num = 0;
	long long prev_page = NULL; //Pointer of prev page in file
	long long next_page = NULL; //Pointer of next page in file
	fstream file(filepath, ios::binary, ios::in | ios::out);
	file.seekg(METADATA_SIZE, ios::beg);
	streampos current = file.tellg();
	file.seekg(0, std::ios::end);
	streamoff remaining = file.tellg() - current;
	file.seekg(current);
	if (remaining % page_size != 0) {
		cerr << "���� �� ������� �� ������ �������";
		return;
	}
	while (remaining > 1) {
		file.read(reinterpret_cast<char*>(&page_num), sizeof(page_num));
		file.read(reinterpret_cast<char*>(&prev_page), sizeof(prev_page));
		file.read(reinterpret_cast<char*>(&next_page), sizeof(next_page));
		file.seekg(next_page, ios::beg);
		streampos current = file.tellg();
		file.seekg(0, std::ios::end);
		streamoff remaining = file.tellg() - current;
		file.seekg(current);
	}
	if (page_num == 0) {
		next_page = METADATA_SIZE + page_size;
	}
	else {
		next_page += page_size;
		prev_page += page_size;
		page_num++;
	}
	file.write(reinterpret_cast<char*>(&page_num), sizeof(page_num));
	file.write(reinterpret_cast<char*>(&prev_page), sizeof(prev_page));
	file.write(reinterpret_cast<char*>(&next_page), sizeof(next_page));

	char data_type = 0; //0 - Data, 1 - Index
	int LSN = 0; // ��������� ������ ���������� �� ��������� � ��� ����
	int checksum = 0; // ����������� �����
	int slot_count = 0; // ���������� ������
	int upper = 0; // 
	unsigned int lower = page_size;

	file.write(&data_type, sizeof(data_type));
	file.write(reinterpret_cast<char*>(&LSN), sizeof(LSN));
	file.write(reinterpret_cast<char*>(&checksum), sizeof(checksum));
	file.write(reinterpret_cast<char*>(&slot_count), sizeof(slot_count));
	file.write(reinterpret_cast<char*>(&upper), sizeof(upper));
	file.write(reinterpret_cast<char*>(&lower), sizeof(lower));
	file.flush();
	file.seekp(page_size-39, ios::cur);
	file.write(0, sizeof(char));
	file.close();
}




//void get_note()

int do_meta() {
	char filepath[256] = "file.bin";  //���� ��������� � ������� �����
	cout << "Creating file: file.bin" << endl;
	make_file(filepath, 1, 1);
	return 0;
}