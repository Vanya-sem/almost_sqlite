#include <iostream>
#include <string>
#include <memory>
#include <windows.h>
#include <unordered_set>
#include "Command.h"
#include "parser.h"

using namespace std;

int main() {
    // Установка кодировки для русских символов в Windows
    SetConsoleOutputCP(65001);

    SimpleDB db;  
    string command;

    cout << "=== Тестирование интеграции парсера и команд ===" << endl;
    cout << "Введите SQL команду (или 'exit' для выхода):" << endl;
    cout << "Примеры команд:" << endl;
    cout << "  CREATE TABLE users (id INT, name VARCHAR(50), age INT)" << endl;
    cout << "  INSERT INTO users (id, name, age) VALUES (1, 'John', 25)" << endl;
    cout << "  SELECT * FROM users" << endl;
    cout << "  UPDATE users SET name = 'Jane' WHERE id = 1" << endl;
    cout << "  DELETE FROM users WHERE id = 1" << endl;
    cout << "=============================================" << endl;

    while (true) {
        cout << "SQL> ";
        getline(cin, command);

        if (command == "exit" || command == "EXIT") {
            cout << "Выход из тестирования..." << endl;
            break;
        }

        if (command.empty()) continue;

        try {
            auto sql_command = parse_sql_command(command);
            cout << "✓ Команда успешно распознана: " << sql_command->getCommandType() << endl;

            // Тестируем валидацию
            if (sql_command->validate()) {
                cout << "✓ Валидация прошла успешно" << endl;

                // Выполняем команду
                sql_command->execute(&db);
                cout << "✓ Команда выполнена успешно" << endl;
            }
            else {
                cout << "✗ Валидация не прошла" << endl;
            }

        }
        catch (const ValidationError& e) {
            cout << "✗ Ошибка валидации: " << e.what() << endl;
        }
        catch (const exception& e) {
            cout << "✗ Неожиданная ошибка: " << e.what() << endl;
        }

        cout << "----------------------------------------" << endl;
    }

    return 0;
}