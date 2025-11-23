#pragma once
#include <string>
#include <unordered_set>

// Простой интерфейс для команд
class DB {
public:
    virtual ~DB() = default;
    virtual void createTable(const std::string& table_name) = 0;
    virtual bool tableExists(const std::string& table_name) const = 0;
    virtual void dropTable(const std::string& table_name) = 0;
};

// Простая реализация для тестирования (можно заменить на DataBase позже)
class SimpleDB : public DB {
private:
    std::unordered_set<std::string> tables;

public:
    void createTable(const std::string& table_name) override {
        tables.insert(table_name);
    }

    bool tableExists(const std::string& table_name) const override {
        return tables.find(table_name) != tables.end();
    }

    void dropTable(const std::string& table_name) override {
        tables.erase(table_name);
    }
};