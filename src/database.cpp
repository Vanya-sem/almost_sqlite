#include "../include/database.h"

Database::Database(const std::string& db_path) : db_path_(db_path) {
    storage_ = new StorageManager(db_path);
    executor_ = new QueryExecutor(storage_);
}

Database::~Database() {
    delete executor_;
    delete storage_;
}

QueryResult Database::execute(const std::string& sql) {
    ParsedQuery query = SQLParser::parse(sql);
    return executor_->execute(query);
}

QueryResult Database::executeQuery(const std::string& sql) {
    return execute(sql);
}

std::vector<std::string> Database::getTableNames() const {
    return storage_->getAllTableNames();
}

bool Database::tableExists(const std::string& table_name) const {
    return storage_->tableExists(table_name);
}

