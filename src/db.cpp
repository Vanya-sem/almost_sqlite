#include <iostream>
#include <fstream>
#include "types.h"

#define HEADER "Almost SQLite1"
#define PAGE_SIZE 4096
#define IS_BTREE 0
#define UNICODE 1 //1 = UTF-8; 2 = UTF-16; 3 = UTF-32
#define ENGINE_VERSION 1
#define RAW_METADATA 24
#define FILE_METADATA_SIZE 50
#define PAGE_METADATA_SIZE 50

using namespace std;
typedef struct File_meta {
	char header[15];
	bool is_btree;
	unsigned int page_size;
	char unicode;
	char engine;
	bool read_flag;
	bool write_flag;
}File_meta;

typedef struct Page_meta {
	unsigned int page_num;
	unsigned long long prev_page;//Pointer of previous page in file
	unsigned long long next_page;//Pointer of next page in file
	char data_type; //0 - Data, 1 - Index
	int LSN; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅ
	int checksum; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ
	int slot_count; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ
	unsigned int upper; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ
	unsigned int lower; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ
}Page_meta;

class DataBase {
public:
	char* filepath;
	unsigned int page_size;
	unsigned int pages = 0;
	DataBase(char* filepath, unsigned int page_size=PAGE_SIZE) : filepath(filepath), page_size(page_size){
		this->read_flag = 1;
		this->write_flag = 1;
	}

	File_meta make_metafile() {
		ofstream file(this->filepath, ios::binary | ios::out);
		if (file) {
			file.write(HEADER, strlen(HEADER)); //14 bytes
			file.write("\0", sizeof(char)); //1 byte
			bool is_btree = IS_BTREE;
			char unicode = UNICODE;
			char engine = ENGINE_VERSION;
			file.write(reinterpret_cast<const char*>(&(this->page_size)), sizeof(this->page_size)); //4 bytes
			file.write(reinterpret_cast<const char*>(&is_btree), sizeof(bool)); //1 byte
			file.write(&unicode, sizeof(char)); //1 byte
			file.write(&engine, sizeof(char)); //1 byte
			file.write(reinterpret_cast<const char*>(&(this->read_flag)), sizeof(char)); //1 byte
			file.write(reinterpret_cast<const char*>(&(this->write_flag)), sizeof(char)); //1 byte


			for (int i = 0; i < FILE_METADATA_SIZE - RAW_METADATA; i++) file.write("\0", sizeof(char)); //26 reserve bytes
			// Metadata of binary file is 50 bytes
			if (file.tellp() != 50) {
				cerr << "пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ";
				return File_meta{};
			}
			file.close();

			return File_meta{ HEADER, is_btree, page_size, unicode, engine, read_flag, write_flag };
		}
		else {
				cerr << "Error! File is not open!";
				return File_meta{};
			}
	}


	Page_meta make_page_meta_db(unsigned int page_num) {
		fstream file(this->filepath, ios::binary | ios::out | ios::in);
		unsigned long long prev_page = NULL; //Pointer of prev page in file
		unsigned long long next_page = NULL; //Pointer of next page in file
		if (!file) {
			cerr << "File is not opened!";
			return Page_meta{};
		}
		if (page_num != this->pages && page_num < 0) {
			cerr << "пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ";
			return Page_meta{};
		}

		long long file_size;
		file.seekp(0, ios::end);
		file_size = file.tellp();

		if (file_size < FILE_METADATA_SIZE + page_num * this->page_size) {
			cerr << "пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ, пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ";
			return Page_meta{};
		}
		

		file.seekp(FILE_METADATA_SIZE + page_num * this->page_size, ios::beg);
		if (page_num != 0) {
			file.write(reinterpret_cast<char*>(&page_num), sizeof(page_num));
			file.seekp(-streamoff(this->page_size + sizeof(page_num)), ios::cur);
			prev_page = file.tellp();
			file.seekp(sizeof(page_num) + sizeof(prev_page), ios::cur);
			next_page = FILE_METADATA_SIZE + page_num * this->page_size;
			file.write(reinterpret_cast<char*>(&next_page), sizeof(next_page));
			file.seekp(this->page_size - sizeof(next_page) - sizeof(prev_page), ios::cur);
			file.write(reinterpret_cast<char*>(&prev_page), sizeof(prev_page));
			std::string zeros(8, '\0');
			file.write(zeros.data(), zeros.size());

		}
		else {
			file.write(reinterpret_cast<char*>(&page_num), sizeof(page_num));
			string zeros(16, '\0');
			file.write(zeros.data(), zeros.size());
		}

		char data_type = 0; //0 - Data, 1 - Index
		int LSN = 0; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅ
		int checksum = 0; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ
		int slot_count = 0; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ
		unsigned int upper = 0; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ
		unsigned int lower = this->page_size; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ

		file.write(&data_type, sizeof(data_type));
		file.write(reinterpret_cast<char*>(&LSN), sizeof(LSN));
		file.write(reinterpret_cast<char*>(&checksum), sizeof(checksum));
		file.write(reinterpret_cast<char*>(&slot_count), sizeof(slot_count));
		file.write(reinterpret_cast<char*>(&upper), sizeof(upper));
		file.write(reinterpret_cast<char*>(&lower), sizeof(lower));
		std::string zeros(8, '\0');
		file.write(zeros.data(), zeros.size());
		file.seekp(this->page_size - PAGE_METADATA_SIZE, ios::cur);
		file.write("\0", sizeof(char));
		if (file.tellp() % this->page_size != FILE_METADATA_SIZE) {
			cerr << "пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ: " << page_num;
		}
		file.close();
		
		this->pages += 1;
		return Page_meta{page_num, prev_page, next_page, data_type, LSN, checksum, slot_count, upper, lower};
	}


	Page_meta print_pagemeta(short int page) {
		unsigned int page_num;
		unsigned long long prev_page = NULL; //Pointer of prev page in file
		unsigned long long next_page = NULL; //Pointer of next page in file
		char data_type; //0 - Data, 1 - Index
		int LSN; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅ
		int checksum; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ
		int slot_count; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ
		unsigned int upper; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ
		unsigned int lower; // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ

		ifstream file(this->filepath, ios::binary | ios::in);
		if (!file) {
			cerr << "File is not opened!";
			return Page_meta{};
		}

		long long file_size;
		file.seekg(0, ios::end);
		file_size = file.tellg();

		if (file_size < FILE_METADATA_SIZE + this->page_size * page || page < 0) {
			cerr << "Ошибка номера страницы при получении метаданных";
			return Page_meta{};
			cerr << "пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ";
			return Page_meta{};
		}

		file.seekg(FILE_METADATA_SIZE + this->page_size * page, ios::beg);
		file.read(reinterpret_cast<char*>(&page_num), sizeof(page_num));
		file.read(reinterpret_cast<char*>(&prev_page), sizeof(prev_page));
		file.read(reinterpret_cast<char*>(&next_page), sizeof(next_page));
		file.read(&data_type, sizeof(data_type));
		file.read(reinterpret_cast<char*>(&LSN), sizeof(LSN));
		file.read(reinterpret_cast<char*>(&checksum), sizeof(checksum));
		file.read(reinterpret_cast<char*>(&slot_count), sizeof(slot_count));
		file.read(reinterpret_cast<char*>(&upper), sizeof(upper));
		file.read(reinterpret_cast<char*>(&lower), sizeof(lower));

		cout << "Page_num: " << (int)page_num << endl;
		cout << "Prev_page: " << (int)prev_page << endl;
		cout << "Next_page: " << (int)next_page << endl;
		cout << "Data_type: " << data_type << endl;
		cout << "LSN: " << (int)LSN << endl;
		cout << "Checksum: " << (int)checksum << endl;
		cout << "Slot_count: " << (int)slot_count << endl;
		cout << "Upper: " << (int)upper << endl;
		cout << "Lower: " << (int)lower << endl;

		file.close();
		return Page_meta{ page_num, prev_page, next_page, data_type, LSN, checksum, slot_count, upper, lower };
	}

	File_meta print_metafile() {
		char header[15], unicode, engine;
		bool read_flag, write_flag;
		bool is_btree;
		unsigned int page_size;
		fstream file(this->filepath, ios::binary | ios::in);
		if (file) {

			file.read(header, sizeof(header));
			file.read(reinterpret_cast<char*>(&page_size), sizeof(page_size));
			file.read(reinterpret_cast<char*>(&is_btree), sizeof(is_btree));
			file.read(&unicode, sizeof(unicode));
			file.read(&engine, sizeof(engine));
			file.read(reinterpret_cast<char*>(&read_flag), sizeof(read_flag));
			file.read(reinterpret_cast<char*>(&write_flag), sizeof(write_flag));
			cout << "File header: " << header << endl;
			cout << "Is b_tree: " << (int)is_btree << endl;
			cout << "Unicode: " << (int)unicode << endl;
			cout << "Engine version: " << (int)engine << endl;
			cout << "Page size: " << (int)page_size << endl;
			cout << "Is able to read: " << (int)read_flag << endl;
			cout << "Is able to write: " << (int)write_flag << endl;
			file.close();
			return File_meta{ HEADER, is_btree, page_size, unicode, engine, read_flag, write_flag };
		}
		return File_meta{};
	}

	bool check_read() {
		bool read;
		ifstream file(this->filepath, ios::binary | ios::in);
		file.seekg(22);
		file.read(reinterpret_cast<char*>(&read), sizeof(int));
		if (read != this->read_flag) {
			cerr << "пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ!";
			return false;
		}
		file.close();
		return read;
	}

	bool check_write() {
		bool write;
		ifstream file(this->filepath, ios::binary | ios::in);
		file.seekg(23);
		file.read(reinterpret_cast<char*>(&write), sizeof(bool));
		if (write != this->write_flag) {
			cerr << "пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ!";
			return false;
		}
		file.close();
		return write;
	}

private:
	bool read_flag, write_flag;
	void set_read(char read) {
		if (read != 0 && read != 1) {
			cerr << "пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ";
			return;
		}
		ofstream file(this->filepath, ios::binary | ios::out);
		file.seekp(22);
		file.write(&read, sizeof(char));
		this->read_flag = read;
		file.close();
	}
	void set_write(char write) {
		if (write != 0 && write != 1) {
			cerr << "пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ";
			return;
		}
		ofstream file(this->filepath, ios::binary | ios::out);
		file.seekp(23);
		file.write(&write, sizeof(write));
		this->write_flag = write;
		file.close();
	}
};


//void get_note()


int main_main_main() {
	char filepath[256] = "file.bin";  //пїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ
	cout << "Creating file: file.bin" << endl;
	DataBase Test(filepath, PAGE_SIZE);
	Test.make_metafile();
	Test.print_metafile();
	cout << endl;
	Test.make_page_meta_db(0);
	Test.make_page_meta_db(1);
	Test.make_page_meta_db(2);
	Test.print_pagemeta(2);
	return 0;
}