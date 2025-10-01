#include <iostream>
#include <fstream>
#include "types.h"

#define HEADER "Almost SQLite1"
#define PAGE_SIZE 4096
#define IS_BTREE 0
#define UNICODE 1 //1 = UTF-8; 2 = UTF-16; 3 = UTF-32
#define ENGINE_VERSION 1
#define METADATA_SIZE 50

using namespace std;

void make_file(char* filepath, char read, char write) {
	ofstream file(filepath, ios::binary);
	if (file) {
		file.write(HEADER, strlen(HEADER)); //14 bytes
		int page_size = PAGE_SIZE;
		char is_btree = IS_BTREE;
		char unicode = UNICODE;
		char engine = ENGINE_VERSION;
		file.write(reinterpret_cast<const char*>(&page_size), sizeof(page_size)); //4 bytes
		file.write(&is_btree, sizeof(char)); //1 byte
		file.write(&unicode, sizeof(char)); //1 byte
		file.write(&engine, sizeof(char)); //1 byte
		file.write(&read, sizeof(char)); //1 byte
		file.write(&write, sizeof(char)); //1 byte

		char null = 0;
		for (int i = 0; i < 27; i++) file.write(&null, sizeof(char)); //27 reserve bytes
		// Metadata of binary file is 50 bytes
		file.close();
	}
	else cout << "Error! File is not open!";
}

void make_page_meta(char* filepath, All_types* colomns) {
	ofstream file(filepath, ios::binary, ios::in | ios::out);
	file.seekp(METADATA_SIZE, ios::beg);

}

//void get_note()

int main() {
	char filepath[256] = "C:\\Users\\vanya\\OneDrive\\Desktop\\almost_sqlite\\src\\file.bin";
	cout << "Input the filepath";
	make_file(filepath, 1, 1);


}