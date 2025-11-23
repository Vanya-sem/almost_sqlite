#include "ValidationError.h"

ValidationError::ValidationError(int error_code, const std::string& message)
    : error_code_(error_code), message_(message) {
}

const char* ValidationError::what() const noexcept {
    return message_.c_str();
}

int ValidationError::getErrorCode() const {
    return error_code_;
}