#include <iostream>
#include <fstream>
#include <sstream>
#include "sqlite3.h"

int main() {
    sqlite3* DB;
    int exit = sqlite3_open("interests_edges.db", &DB);

    if (exit) {
        std::cerr << "Error opening DB: " << sqlite3_errmsg(DB) << std::endl;
        return -1;
    } else {
        std::cout << "Opened database successfully!" << std::endl;
    }

    // Load sharedintereststructure.sql
    std::ifstream file("sharedintereststructure.sql");
    if (!file) {
        std::cerr << "Could not open sharedintereststructure.sql" << std::endl;
        return -1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sharedintereststructure = buffer.str();

    // Execute sharedintereststructure
    char* errorMessage;
    exit = sqlite3_exec(DB, sharedintereststructure.c_str(), NULL, 0, &errorMessage);

    if (exit != SQLITE_OK) {
        std::cerr << "Sharedintereststructure error: " << errorMessage << std::endl;
        sqlite3_free(errorMessage);
    } else {
        std::cout << "Tables created successfully!" << std::endl;
    }

    sqlite3_close(DB);


    sqlite3* DB2;
    int exit2 = sqlite3_open("goals_edges.db", &DB2);

    if (exit2) {
        std::cerr << "Error opening DB2: " << sqlite3_errmsg(DB2) << std::endl;
        return -1;
    } else {
        std::cout << "Opened database successfully!" << std::endl;
    }

    // Load sharedgoalsstructure.sql
    std::ifstream file2("sharedgoalsstructure.sql");
    if (!file2) {
        std::cerr << "Could not open sharedgoalsstructure.sql" << std::endl;
        return -1;
    }

    std::stringstream buffer2;
    buffer2 << file2.rdbuf();
    std::string sharedgoalsstructure = buffer2.str();

    // Execute sharedgoalsstructure
    char* errorMessage2;
    exit = sqlite3_exec(DB2, sharedgoalsstructure.c_str(), NULL, 0, &errorMessage2);

    if (exit != SQLITE_OK) {
        std::cerr << "Sharedgoalsstructure error: " << errorMessage2 << std::endl;
        sqlite3_free(errorMessage2);
    } else {
        std::cout << "Tables created successfully!" << std::endl;
    }

    sqlite3_close(DB2);



    sqlite3* DB3;
    int exit3 = sqlite3_open("company_groups.db", &DB3);

    if (exit3) {
        std::cerr << "Error opening DB3: " << sqlite3_errmsg(DB3) << std::endl;
        return -1;
    } else {
        std::cout << "Opened database successfully!" << std::endl;
    }

    // Load company_groups.sql
    std::ifstream file3("company_groups.sql");
    if (!file3) {
        std::cerr << "Could not open company_groups.sql" << std::endl;
        return -1;
    }

    std::stringstream buffer3;
    buffer3 << file3.rdbuf();
    std::string companygroupsstructure = buffer3.str();

    // Execute companygroupsstructure
    char* errorMessage3;
    exit3 = sqlite3_exec(DB3, companygroupsstructure.c_str(), NULL, 0, &errorMessage3);

    if (exit3 != SQLITE_OK) {
        std::cerr << "companygroupsstructure error: " << errorMessage3 << std::endl;
        sqlite3_free(errorMessage3);
    } else {
        std::cout << "Tables created successfully!" << std::endl;
    }

    sqlite3_close(DB3);



    sqlite3* DB4;
    int exit4 = sqlite3_open("previous_companies.db", &DB4);

    if (exit4) {
        std::cerr << "Error opening DB4: " << sqlite3_errmsg(DB4) << std::endl;
        return -1;
    } else {
        std::cout << "Opened database successfully!" << std::endl;
    }

    // Load previous_companies.sql
    std::ifstream file4("previous_companies.sql");
    if (!file4) {
        std::cerr << "Could not open previous_companies.sql" << std::endl;
        return -1;
    }

    std::stringstream buffer4;
    buffer4 << file4.rdbuf();
    std::string previouscompaniesstructure = buffer4.str();

    // Execute previouscompaniesstructure
    char* errorMessage4;
    exit4 = sqlite3_exec(DB4, previouscompaniesstructure.c_str(), NULL, 0, &errorMessage4);

    if (exit4 != SQLITE_OK) {
        std::cerr << "previouscompaniesstructure error: " << errorMessage4 << std::endl;
        sqlite3_free(errorMessage4);
    } else {
        std::cout << "Tables created successfully!" << std::endl;
    }

    sqlite3_close(DB4);







    sqlite3* DB5;
    int exit5 = sqlite3_open("skill_edges.db", &DB5);

    if (exit5) {
        std::cerr << "Error opening DB5: " << sqlite3_errmsg(DB5) << std::endl;
        return -1;
    } else {
        std::cout << "Opened database successfully!" << std::endl;
    }

    // Load sharedskillsstructure.sql
    std::ifstream file5("sharedskillsstructure.sql");
    if (!file5) {
        std::cerr << "Could not open sharedskillsstructure.sql" << std::endl;
        return -1;
    }

    std::stringstream buffer5;
    buffer5 << file5.rdbuf();
    std::string sharedskillsstructure = buffer5.str();

    // Execute sharedskillsstructure
    char* errorMessage5;
    exit5 = sqlite3_exec(DB5, sharedskillsstructure.c_str(), NULL, 0, &errorMessage5);

    if (exit5 != SQLITE_OK) {
        std::cerr << "sharedskillsstructure error: " << errorMessage5 << std::endl;
        sqlite3_free(errorMessage5);
    } else {
        std::cout << "tables created successfully!" << std::endl;
    }

    sqlite3_close(DB5);






    sqlite3* DB6;
    int exit6 = sqlite3_open("college_groups.db", &DB6);

    if (exit6) {
        std::cerr << "Error opening DB6: " << sqlite3_errmsg(DB6) << std::endl;
        return -1;
    } else {
        std::cout << "Opened database successfully!" << std::endl;
    }

    // Load college_groups.sql
    std::ifstream file6("college_groups.sql");
    if (!file6) {
        std::cerr << "Could not open sharedskillsstructure.sql" << std::endl;
        return -1;
    }

    std::stringstream buffer6;
    buffer6 << file6.rdbuf();
    std::string collegegroupsstructure = buffer6.str();

    // Execute collegegroupsstructure
    char* errorMessage6;
    exit6 = sqlite3_exec(DB6, collegegroupsstructure.c_str(), NULL, 0, &errorMessage6);

    if (exit6 != SQLITE_OK) {
        std::cerr << "❌ collegegroupsstructure error: " << errorMessage6 << std::endl;
        sqlite3_free(errorMessage6);
    } else {
        std::cout << "tables created successfully" << std::endl;
    }

    sqlite3_close(DB6);


    sqlite3* DB7;
int exit7 = sqlite3_open("highschool_groups.db", &DB7);

if (exit7) {
    std::cerr << "Error opening DB7: " << sqlite3_errmsg(DB7) << std::endl;
    return -1;
} else {
    std::cout << "Opened database successfully" << std::endl;
}

// Load highschool_groups.sql
std::ifstream file7("highschool_groups.sql");
if (!file7) {
    std::cerr << "Could not open highschool_groups.sql" << std::endl;
    return -1;
}

std::stringstream buffer7;
buffer7 << file7.rdbuf();
std::string highschoolgroupsstructure = buffer7.str();

// Execute highschoolgroupsstructure
char* errorMessage7;
exit7 = sqlite3_exec(DB7, highschoolgroupsstructure.c_str(), NULL, 0, &errorMessage7);

if (exit7 != SQLITE_OK) {
    std::cerr << "highschoolgroupsstructure error: " << errorMessage7 << std::endl;
    sqlite3_free(errorMessage7);
} else {
    std::cout << "Tables created successfully!" << std::endl;
}

sqlite3_close(DB7);


sqlite3* DB8;
int exit8 = sqlite3_open("industry_groups.db", &DB8);

if (exit8) {
    std::cerr << "Error opening DB8: " << sqlite3_errmsg(DB8) << std::endl;
    return -1;
} else {
    std::cout << "Opened database successfully!" << std::endl;
}

// Load industry_groups.sql
std::ifstream file8("industry_groups.sql");
if (!file8) {
    std::cerr << "Could not open industry_groups.sql" << std::endl;
    return -1;
}

std::stringstream buffer8;
buffer8 << file8.rdbuf();
std::string industrygroupsstructure = buffer8.str();

// Execute industrygroupsstructure
char* errorMessage8;
exit8 = sqlite3_exec(DB8, industrygroupsstructure.c_str(), NULL, 0, &errorMessage8);

if (exit8 != SQLITE_OK) {
    std::cerr << "industrygroupsstructure error: " << errorMessage8 << std::endl;
    sqlite3_free(errorMessage8);
} else {
    std::cout << "Tables created successfully!" << std::endl;
}

sqlite3_close(DB8);



    return 0;
}


/* to compile use this command 

gcc -c sqlite3.c -o sqlite3.o -I.
g++ createadditionaldatabases.cpp sqlite3.o -o adddb -I.

-I flag tells it to search current directory for header files 

*/
