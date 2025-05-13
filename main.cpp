#include <iostream>
#include <fstream>
#include <sstream>
#include "sqlite3.h"

int main() {
    sqlite3* DB;
    int exit = sqlite3_open("my_database.db", &DB);

    if (exit) {
        std::cerr << "❌ Error opening DB: " << sqlite3_errmsg(DB) << std::endl;
        return -1;
    } else {
        std::cout << "✅ Opened database successfully!" << std::endl;
    }

    // Load structure.sql
    std::ifstream file("structure.sql");
    if (!file) {
        std::cerr << "❌ Could not open structure.sql" << std::endl;
        return -1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string structure = buffer.str();

    // Execute structure
    char* errorMessage;
    exit = sqlite3_exec(DB, structure.c_str(), NULL, 0, &errorMessage);

    if (exit != SQLITE_OK) {
        std::cerr << "❌ Structure error: " << errorMessage << std::endl;
        sqlite3_free(errorMessage);
    } else {
        std::cout << "✅ Tables created successfully!" << std::endl;
    }

    sqlite3_close(DB);
    return 0;
}


/* to compile use this command 

gcc -c sqlite3.c -o sqlite3.o -I.
g++ main.cpp sqlite3.o -o my_app -I.

-I flag tells it to search current directory for header files 


client id : 815720075109-0ksu98p7ba55ntmrusruftg03roev93k.apps.googleusercontent.com
*/
