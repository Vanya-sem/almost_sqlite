#pragma once

#include "Command.h"
#include "ValidationError.h"
#include <memory>
#include <string>

// Главная функция парсера
std::unique_ptr<Command> parse_sql_command(const std::string& command);