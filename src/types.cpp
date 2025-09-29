#include <stdio.h>

typedef enum Numeric_type{
	BIGINT,
	INT,
	SMALLINT,
	TINYINT,
	BIT,
	DECIMAL,
	NUMERIC,
	MONEY,
	SMALLMONEY,
	FLOAT,
	REAL
} NumericType;


typedef enum Date_type {
	DATETIME,
	SMALLDATETIME,
	DATE,
	TIME
}DateType;

typedef enum Char_type {
	CHAR,
	VARCHAR,
	TEXT
}CharType;

typedef enum Types {
	NUMERIC,
	CHAR,
	DATETIME
}Types;

class Type {
public:
	Types tp;
	int size;
	virtual read_from_binary();
	virtual write_to_binary ();
	Type(Type type, int size) {
		this->tp = tp;
		this->size = size;
	}
};

class Numeric_type : public Type {
public:
	Types tp;
	NumericType subtype;
	int size;
	virtual read_from_binary();
	virtual write_to_binary();
	Numeric_type(Types tp, int size, NumericType subtype) {
		: Type(type, size);
		this->size = size;
		this->tp = tp;
		this->subtype = subtype;
	}
};

class Bigint : public Numeric_type {
public:
	Types tp;
	NumericType subtype;
	int size = 8;
	Bigint(char[20] str_bigint) {
		: NumericType (NUMERIC, 8, BIGINT)
		this->tp = NUMERIC;
		this->subtype = BIGINT;
	}

};