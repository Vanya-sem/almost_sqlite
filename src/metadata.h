#pragma once
#include "types.h"
#include <ctime>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#pragma pack(push, 1)

struct TableMetadataHeader {
    char signature[16];
    uint32_t version;
    uint32_t column_count;
    uint64_t created_time;
    uint64_t record_count;
    uint32_t data_file_size;
    uint16_t flags;
    char reserved[14];
};

struct ColumnMetadata {
    char name[64];
    All_types type;
    uint32_t size;
    uint16_t offset;
    uint8_t flags;
    char reserved[7];
};

#pragma pack(pop)

void serialize_metadata(const std::string& table_name, const std::vector<Column>& columns, uint64_t record_count);
bool deserialize_metadata(const std::string& table_name, std::vector<Column>& columns, uint64_t& record_count);