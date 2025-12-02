// Command.h
#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "types.h"
#include "validator.h"
#include "db.h" 

class Command {
public:
    virtual ~Command() = default;
    virtual void execute(DB* db) = 0;
    virtual std::string getCommandType() const = 0;
    virtual bool validate() const = 0;
};

class CreateCommand : public Command {
public:
    CreateCommand(const std::string& table_name,
        const std::vector<std::string>& column_names,
        const std::vector<std::string>& data_types,
        const std::vector<bool>& is_nullable);

    void execute(DB* db) override;
    std::string getCommandType() const override;
    bool validate() const override;

    const std::string& getTableName() const { return table_name_; }
    const std::vector<std::string>& getColumnNames() const { return column_names_; }
    const std::vector<std::string>& getDataTypes() const { return data_types_; }
    const std::vector<bool>& getIsNullable() const { return is_nullable_; }

private:
    std::string table_name_;
    std::vector<std::string> column_names_;
    std::vector<std::string> data_types_;
    std::vector<bool> is_nullable_;
};

class SelectCommand : public Command {
public:
    SelectCommand(const std::vector<std::string>& columns,
        const std::string& table_name,
        const std::vector<std::string>& where_conditions = {},
        const std::vector<std::string>& order_by = {},
        int limit = -1);

    void execute(DB* db) override;
    std::string getCommandType() const override;
    bool validate() const override;

    const std::vector<std::string>& getColumns() const { return columns_; }
    const std::string& getTableName() const { return table_name_; }
    const std::vector<std::string>& getWhereConditions() const { return where_conditions_; }
    const std::vector<std::string>& getOrderBy() const { return order_by_; }
    int getLimit() const { return limit_; }

private:
    std::vector<std::string> columns_;
    std::string table_name_;
    std::vector<std::string> where_conditions_;
    std::vector<std::string> order_by_;
    int limit_;
};

class UpdateCommand : public Command {
public:
    UpdateCommand(const std::string& table_name,
        const std::vector<std::pair<std::string, std::string>>& set_clauses,
        const std::vector<std::string>& where_conditions = {});

    void execute(DB* db) override;
    std::string getCommandType() const override;
    bool validate() const override;

    const std::string& getTableName() const { return table_name_; }
    const std::vector<std::pair<std::string, std::string>>& getSetClauses() const { return set_clauses_; }
    const std::vector<std::string>& getWhereConditions() const { return where_conditions_; }

private:
    std::string table_name_;
    std::vector<std::pair<std::string, std::string>> set_clauses_;
    std::vector<std::string> where_conditions_;
};

class DeleteCommand : public Command {
public:
    DeleteCommand(const std::string& table_name,
        const std::vector<std::string>& where_conditions = {});

    void execute(DB* db) override;
    std::string getCommandType() const override;
    bool validate() const override;

    const std::string& getTableName() const { return table_name_; }
    const std::vector<std::string>& getWhereConditions() const { return where_conditions_; }

private:
    std::string table_name_;
    std::vector<std::string> where_conditions_;
};

class InsertCommand : public Command {
public:
    InsertCommand(const std::string& table_name,
        const std::vector<std::string>& column_names,
        const std::vector<std::vector<std::string>>& values);

    void execute(DB* db) override;  
    std::string getCommandType() const override;
    bool validate() const override;

    const std::string& getTableName() const { return table_name_; }
    const std::vector<std::string>& getColumnNames() const { return column_names_; }
    const std::vector<std::vector<std::string>>& getValues() const { return values_; }

private:
    std::string table_name_;
    std::vector<std::string> column_names_;
    std::vector<std::vector<std::string>> values_;
};

class AlterCommand : public Command {
public:
    enum OperationType {
        ADD_COLUMN,
        DROP_COLUMN,
        MODIFY_COLUMN,
        ADD_CONSTRAINT
    };

    AlterCommand(const std::string& table_name,
        OperationType operation_type,
        const std::string& column_name = "",
        const std::string& data_type = "",
        const std::string& constraint = "",
        bool nullable = true);

    void execute(DB* db) override;
    std::string getCommandType() const override;
    bool validate() const override;

    const std::string& getTableName() const { return table_name_; }
    OperationType getOperationType() const { return operation_type_; }
    const std::string& getColumnName() const { return column_name_; }
    const std::string& getDataType() const { return data_type_; }
    const std::string& getConstraint() const { return constraint_; }
    bool isNullable() const { return nullable_; }

private:
    std::string table_name_;
    OperationType operation_type_;
    std::string column_name_;
    std::string data_type_;
    std::string constraint_;
    bool nullable_;
};

#endif 