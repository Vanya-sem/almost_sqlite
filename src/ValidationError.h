#ifndef VALIDATIONERROR_H
#define VALIDATIONERROR_H

#include <stdexcept>
#include <string>

enum ErrorCode {
    INVALID_SYNTAX = 1,
    TABLE_NOT_EXISTS = 2,
    TABLE_ALREADY_EXISTS = 3,
    COLUMN_NOT_EXISTS = 4,
    TYPE_MISMATCH = 5
};

class ValidationError : public std::exception {
public:
    ValidationError(int error_code, const std::string& message);
    const char* what() const noexcept override;
    int getErrorCode() const;

private:
    int error_code_;
    std::string message_;
};

#endif