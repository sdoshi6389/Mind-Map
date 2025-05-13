#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <tuple>
#include <sstream>
#include <curl/curl.h>
#include <set>
#include <algorithm>

using json = nlohmann::json;

struct entry {
    std::string name;
    std::string email;
    std::string phone;
    std::string location;

    std::string currCompany;
    std::string prevCompanies; 
    std::string industry;
    std::string jobTitle;

    std::string relationship;
    std::string closeness;
    std::string reliability;

    std::string careerGoals;
    std::string skills;
    std::string talent;
};

struct GeoEntry {
    int id;
    std::string name;
    std::string location;
    double latitude;
    double longitude;
};

GeoEntry convert_to_geoentry(int id, const entry& e, const std::map<std::string, std::pair<double, double>>& location_coords) {
    GeoEntry g;
    g.id = id;
    g.name = e.name;
    g.location = e.location;

    auto it = location_coords.find(e.location);
    if (it != location_coords.end()) {
        g.latitude = it->second.first;
        g.longitude = it->second.second;
    } else {
        g.latitude = 0.0;
        g.longitude = 0.0;
    }

    return g;
}

std::string getSafeText(sqlite3_stmt* stmt, int col) {
    const unsigned char* text = sqlite3_column_text(stmt, col);
    return text ? reinterpret_cast<const char*>(text) : "";
}

std::string buffer;  // Global buffer for simplicity

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realSize = size * nmemb;
    buffer.append((char*)contents, realSize);
    return realSize;
}

bool get_lat_lon(const std::string& location, double& lat, double& lon) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string api_key = "abafb6fc8b6d4c12866c8a4a6f120911";

    // Escape the location query
    char* escaped = curl_easy_escape(curl, location.c_str(), 0);
    if (!escaped) {
        curl_easy_cleanup(curl);
        return false;
    }

    std::string url = "https://api.opencagedata.com/geocode/v1/json?q=" + std::string(escaped) + "&key=" + api_key;
    curl_free(escaped); 

    buffer.clear(); 

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr);

    // Disable SSL verification for local testing
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    try {
        //std::cout << "[DEBUG] Raw response:\n" << buffer << std::endl;

        auto j = json::parse(buffer);
        if (j["results"].empty()) return false;

        lat = j["results"][0]["geometry"]["lat"];
        lon = j["results"][0]["geometry"]["lng"];
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "JSON parse failed: " << ex.what() << std::endl;
        return false;
    }
}

struct PixelPosition {
    double x;
    double y;
};

PixelPosition geo_to_scrollable_pixel(double lat, double lon,
                                      double minLat, double maxLat,
                                      double minLon, double maxLon,
                                      double width, double baseHeight) {
    // Normalize to [0,1]
    double normX = (lon - minLon) / (maxLon - minLon);
    double normY = (maxLat - lat) / (maxLat - minLat);  // y = 0 is top

    // Scale
    double x = normX * width;
    double y = normY * baseHeight;

    return {x, y};
}

json generate_edges_by_default(const std::vector<entry>& entries) {
    // default
    json result;
    result["nodes"] = json::array();

    for (size_t i = 0; i < entries.size(); ++i) {
        result["nodes"].push_back({
            {"id", static_cast<int>(i + 1)},
            {"name", entries[i].name}
        });
    }

    result["edges"] = json::array();  // no edges yet

    std::cout << result.dump(2) << std::endl;

    return result;
}


json generate_edges_by_talent(const std::vector<entry>& entries) {
    json result;
    result["nodes"] = json::array();
    result["edges"] = json::array();

    std::map<int, std::vector<std::pair<entry, PixelPosition>>> talentMap;
    std::map<std::string, PixelPosition> locationMap;
    std::map<std::string, int> nameToId;

    // Add root node
    result["nodes"].push_back({
        {"id", 0},
        {"name", "Sohil Doshi"},
        {"location", "Sunnyvale, CA"},
        {"x", 2000},
        {"y", 0}
    });

    int next_id = 1;

    for (const auto& e : entries) {
        int talent = std::stoi(e.talent.empty() ? "0" : e.talent);
        double lat, lon;

        if (!get_lat_lon(e.location, lat, lon)) {
            lat = 82.8628; lon = 135;
        }

        PixelPosition pos = geo_to_scrollable_pixel(lat, lon, -90, 90, -180, 180, 4000, 1000);
        talentMap[talent].push_back({e, pos});
        locationMap[e.name] = pos;
    }

    for (int level = 10; level >= 1; --level) {
        for (auto& [person, pos] : talentMap[level]) {
            int myId = next_id++;
            nameToId[person.name] = myId;

            result["nodes"].push_back({
                {"id", myId},
                {"name", person.name},
                {"location", person.location},
                {"x", pos.x},
                {"y", pos.y}
            });
        }
    }

    // create red bidirectional edges for same location, same level
    for (int level = 10; level >= 1; --level) {
        const auto& people = talentMap[level];
        for (size_t i = 0; i < people.size(); ++i) {
            for (size_t j = i + 1; j < people.size(); ++j) {
                const auto& [p1, _] = people[i];
                const auto& [p2, __] = people[j];
                if (p1.location == p2.location && p1.name != p2.name) {
                    int id1 = nameToId[p1.name];
                    int id2 = nameToId[p2.name];
                    result["edges"].push_back({
                        {"source", id1}, {"target", id2}, {"color", "red"}
                    });
                    result["edges"].push_back({
                        {"source", id2}, {"target", id1}, {"color", "red"}
                    });
                }
            }
        }
    }

    // create blue directional edges to higher level nodes
    for (int level = 10; level >= 1; --level) {
        for (auto& [person, pos] : talentMap[level]) {
            int myId = nameToId[person.name];

            for (int hl = level + 1; hl <= 10; ++hl) {
                if (!talentMap.count(hl)) continue;

                double minDist = 1e9;
                int closestId = -1;
                for (auto& [higher, higherPos] : talentMap[hl]) {
                    if (!nameToId.count(higher.name)) continue;
                    int hid = nameToId[higher.name];
                    double dx = pos.x - higherPos.x;
                    double dy = pos.y - higherPos.y;
                    double dist = dx * dx + dy * dy;
                    if (dist < minDist) {
                        minDist = dist;
                        closestId = hid;
                    }
                }

                if (closestId != -1) {
                    result["edges"].push_back({
                        {"source", closestId}, {"target", myId}, {"color", "blue"}
                    });
                    break;
                }
            }

            if (level == 10) {
                result["edges"].push_back({
                    {"source", 0}, {"target", myId}, {"color", "blue"}
                });
            }
        }
    }

    std::cerr << "[DEBUG] Finished generate_edges_by_talent" << std::endl;
    std::cout << result.dump(2) << std::endl;
    return result;
}


json generate_edges_by_closeness(const std::vector<entry>& entries) {
    json result;
    result["nodes"] = json::array();
    result["edges"] = json::array();

    std::map<int, std::vector<std::pair<entry, PixelPosition>>> closenessMap;
    std::map<std::string, PixelPosition> locationMap;
    std::map<std::string, int> nameToId;

    // Add root node
    result["nodes"].push_back({
        {"id", 0},
        {"name", "Sohil Doshi"},
        {"location", "Sunnyvale, CA"},
        {"x", 2000},
        {"y", 0}
    });

    int next_id = 1;

    for (const auto& e : entries) {
        int close = std::stoi(e.closeness.empty() ? "0" : e.closeness);
        double lat, lon;

        if (!get_lat_lon(e.location, lat, lon)) {
            lat = 82.8628; lon = 135;
        }

        PixelPosition pos = geo_to_scrollable_pixel(lat, lon, -90, 90, -180, 180, 4000, 1000);
        closenessMap[close].push_back({e, pos});
        locationMap[e.name] = pos;
    }

    for (int level = 10; level >= 1; --level) {
        for (auto& [person, pos] : closenessMap[level]) {
            int myId = next_id++;
            nameToId[person.name] = myId;

            result["nodes"].push_back({
                {"id", myId},
                {"name", person.name},
                {"location", person.location},
                {"x", pos.x},
                {"y", pos.y}
            });
        }
    }

    // Create red bidirectional edges for same location, same level
    for (int level = 10; level >= 1; --level) {
        const auto& people = closenessMap[level];
        for (size_t i = 0; i < people.size(); ++i) {
            for (size_t j = i + 1; j < people.size(); ++j) {
                const auto& [p1, _] = people[i];
                const auto& [p2, __] = people[j];
                if (p1.location == p2.location && p1.name != p2.name) {
                    int id1 = nameToId[p1.name];
                    int id2 = nameToId[p2.name];
                    result["edges"].push_back({
                        {"source", id1}, {"target", id2}, {"color", "red"}
                    });
                    result["edges"].push_back({
                        {"source", id2}, {"target", id1}, {"color", "red"}
                    });
                }
            }
        }
    }

    // Create blue directional edges to higher level nodes
    for (int level = 10; level >= 1; --level) {
        for (auto& [person, pos] : closenessMap[level]) {
            int myId = nameToId[person.name];

            for (int hl = level + 1; hl <= 10; ++hl) {
                if (!closenessMap.count(hl)) continue;

                double minDist = 1e9;
                int closestId = -1;
                for (auto& [higher, higherPos] : closenessMap[hl]) {
                    if (!nameToId.count(higher.name)) continue;
                    int hid = nameToId[higher.name];
                    double dx = pos.x - higherPos.x;
                    double dy = pos.y - higherPos.y;
                    double dist = dx * dx + dy * dy;
                    if (dist < minDist) {
                        minDist = dist;
                        closestId = hid;
                    }
                }

                if (closestId != -1) {
                    result["edges"].push_back({
                        {"source", closestId}, {"target", myId}, {"color", "blue"}
                    });
                    break;
                }
            }

            if (level == 10) {
                result["edges"].push_back({
                    {"source", 0}, {"target", myId}, {"color", "blue"}
                });
            }
        }
    }

    std::cerr << "[DEBUG] Finished generate_edges_by_closeness" << std::endl;
    std::cout << result.dump(2) << std::endl;
    return result;
}


json generate_edges_by_location(const std::vector<entry>& entries) {
    json result;
    result["nodes"] = json::array();
    result["edges"] = json::array(); // empty for now

    std::vector<std::string> locations;
    std::vector<std::tuple<entry, double, double>> entriesLat; 

    //printf("%d", entries.size());

    /* for debugging
    for (const auto& e : entries) {
        locations.push_back(e.location);
        std::cout << "Location: " << e.location << std::endl;
    }

    for (const auto& l : locations) {
        double lat;
        double lon;
        if (get_lat_lon(l, lat, lon)) {
            std::cout << "Lat: " << lat << ", Lon: " << lon << std::endl;
        } else {
            std::cout << "Geocoding failed" << std::endl;
        }
    } */

    for (const auto& e : entries) {
        double lat;
        double lon;
        if (get_lat_lon(e.location, lat, lon)) {
            std::ostringstream oss;
            oss << lat << " " << lon;
            entriesLat.push_back(std::make_tuple(e, lat, lon));
            //std::cout << "Location: " << e.location << ", Lat: " << lat << ", Lon: " << lon << std::endl;
        } else {
            entriesLat.push_back(std::make_tuple(e, 82.8628, 135)); //set to antartica
            //std::cout << "Geocoding failed" << std::endl;
        }
    }

    for (size_t i = 0; i < entriesLat.size(); ++i) {
        const auto& g = entriesLat[i];
        PixelPosition pos = geo_to_scrollable_pixel(std::get<1>(g), std::get<2>(g), -90, 90, -180, 180, 4000, 1000);
    
        result["nodes"].push_back({
            {"id", static_cast<int>(i + 1)},
            {"name", std::get<0>(g).name},
            {"location", std::get<0>(g).location},
            {"x", pos.x},
            {"y", pos.y}
        });
    }    

    std::cout << result.dump(2) << std::endl;
    
    return result;
}


std::vector<std::string> split_people(const std::string &peopleStr) {
    std::vector<std::string> people;
    std::stringstream ss(peopleStr);
    std::string name;

    while (std::getline(ss, name, ',')) {
        name.erase(std::remove_if(name.begin(), name.end(), ::isspace), name.end());
        if (!name.empty()) {
            people.push_back(name);
        }
    }

    return people;
}


json generate_edges_by_college(const std::string &db_path = "college_groups.db") {
    json result;
    result["nodes"] = json::array();
    result["edges"] = json::array();

    sqlite3 *db;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return result;
    }

    const char *query = "SELECT college, people FROM college_groups;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return result;
    }

    std::set<std::string> unique_nodes;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string college = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string people_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        std::vector<std::string> people = split_people(people_str);

        // Add nodes
        for (const auto &person : people) {
            if (unique_nodes.insert(person).second) {
                result["nodes"].push_back({{"id", person}});
            }
        }

        // Add edges for all combinations if more than one person
        for (size_t i = 0; i < people.size(); ++i) {
            for (size_t j = i + 1; j < people.size(); ++j) {
                result["edges"].push_back({
                    {"source", people[i]},
                    {"target", people[j]},
                    {"label", college}
                });
                result["edges"].push_back({
                    {"source", people[j]},
                    {"target", people[i]},
                    {"label", college}
                });
            }
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    std::cout << result.dump(2) << std::endl;

    return result;

}


json generate_edges_by_high_school(const std::string &db_path = "highschool_groups.db") {
    json result;
    result["nodes"] = json::array();
    result["edges"] = json::array();

    sqlite3 *db;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return result;
    }

    const char *query = "SELECT highschool, people FROM highschool_groups;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return result;
    }

    std::set<std::string> unique_nodes;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string high_school = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string people_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        std::vector<std::string> people = split_people(people_str);

        // Add nodes
        for (const auto &person : people) {
            if (unique_nodes.insert(person).second) {
                result["nodes"].push_back({{"id", person}});
            }
        }

        // Add edges for all combinations if more than one person
        for (size_t i = 0; i < people.size(); ++i) {
            for (size_t j = i + 1; j < people.size(); ++j) {
                result["edges"].push_back({
                    {"source", people[i]},
                    {"target", people[j]},
                    {"label", high_school}
                });
                result["edges"].push_back({
                    {"source", people[j]},
                    {"target", people[i]},
                    {"label", high_school}
                });
            }
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << result.dump(2) << std::endl;

    return result;
}


json generate_edges_by_industry(const std::string &db_path = "industry_groups.db") {
    json result;
    result["nodes"] = json::array();
    result["edges"] = json::array();

    sqlite3 *db;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return result;
    }

    const char *query = "SELECT industry, people FROM industry_groups;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return result;
    }

    std::set<std::string> unique_nodes;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string industry = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string people_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        std::vector<std::string> people = split_people(people_str);

        // Add nodes
        for (const auto &person : people) {
            if (unique_nodes.insert(person).second) {
                result["nodes"].push_back({{"id", person}});
            }
        }

        // Add edges for all combinations if more than one person
        for (size_t i = 0; i < people.size(); ++i) {
            for (size_t j = i + 1; j < people.size(); ++j) {
                result["edges"].push_back({
                    {"source", people[i]},
                    {"target", people[j]},
                    {"label", industry}
                });
                result["edges"].push_back({
                    {"source", people[j]},
                    {"target", people[i]},
                    {"label", industry}
                });
            }
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << result.dump(2) << std::endl;

    return result;
}


json generate_edges_by_current_company(const std::string &db_path = "company_groups.db") {
    json result;
    result["nodes"] = json::array();
    result["edges"] = json::array();

    sqlite3 *db;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return result;
    }

    const char *query = "SELECT company, people FROM company_groups;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return result;
    }

    std::set<std::string> unique_nodes;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string company = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string people_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        std::vector<std::string> people = split_people(people_str);

        // Add nodes
        for (const auto &person : people) {
            if (unique_nodes.insert(person).second) {
                result["nodes"].push_back({{"id", person}});
            }
        }

        // Add edges for all combinations if more than one person
        for (size_t i = 0; i < people.size(); ++i) {
            for (size_t j = i + 1; j < people.size(); ++j) {
                result["edges"].push_back({
                    {"source", people[i]},
                    {"target", people[j]},
                    {"label", company}
                });
                result["edges"].push_back({
                    {"source", people[j]},
                    {"target", people[i]},
                    {"label", company}
                });
            }
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << result.dump(2) << std::endl;

    return result;
}


json generate_edges_by_previous_company(const std::string &db_path = "previous_companies.db") {
    json result;
    result["nodes"] = json::array();
    result["edges"] = json::array();

    sqlite3 *db;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return result;
    }

    const char *query = "SELECT company, people FROM previous_companies;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return result;
    }

    std::set<std::string> unique_nodes;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string previous_company = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string people_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        std::vector<std::string> people = split_people(people_str);

        // Add nodes
        for (const auto &person : people) {
            if (unique_nodes.insert(person).second) {
                result["nodes"].push_back({{"id", person}});
            }
        }

        // Add edges for all combinations if more than one person
        for (size_t i = 0; i < people.size(); ++i) {
            for (size_t j = i + 1; j < people.size(); ++j) {
                result["edges"].push_back({
                    {"source", people[i]},
                    {"target", people[j]},
                    {"label", previous_company}
                });
                result["edges"].push_back({
                    {"source", people[j]},
                    {"target", people[i]},
                    {"label", previous_company}
                });
            }
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << result.dump(2) << std::endl;

    return result;
}


json generate_edges_by_shared_skills(const std::string &db_path = "skill_edges.db") {
    json result;
    result["nodes"] = json::array();
    result["edges"] = json::array();

    sqlite3 *db;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return result;
    }

    const char *query = "SELECT person1, person2, shared_skills FROM shared_skills;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return result;
    }

    std::set<std::string> unique_nodes;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string person1 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string person2 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string skills = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

        if (unique_nodes.insert(person1).second) {
            result["nodes"].push_back({{"id", person1}});
        }
        if (unique_nodes.insert(person2).second) {
            result["nodes"].push_back({{"id", person2}});
        }

        result["edges"].push_back({
            {"source", person1},
            {"target", person2},
            {"label", skills}
        });
        result["edges"].push_back({
            {"source", person2},
            {"target", person1},
            {"label", skills}
        });
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << result.dump(2) << std::endl;

    return result;
}


json generate_edges_by_shared_interests(const std::string &db_path = "interests_edges.db") {
    json result;
    result["nodes"] = json::array();
    result["edges"] = json::array();

    sqlite3 *db;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return result;
    }

    const char *query = "SELECT person1, person2, shared_interests FROM interest_edges;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return result;
    }

    std::set<std::string> unique_nodes;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string person1 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string person2 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string interests = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

        if (unique_nodes.insert(person1).second) {
            result["nodes"].push_back({{"id", person1}});
        }
        if (unique_nodes.insert(person2).second) {
            result["nodes"].push_back({{"id", person2}});
        }


        result["edges"].push_back({
            {"source", person1},
            {"target", person2},
            {"label", interests}
        });
        result["edges"].push_back({
            {"source", person2},
            {"target", person1},
            {"label", interests}
        });
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << result.dump(2) << std::endl;

    return result;
}


json generate_edges_by_shared_goals(const std::string &db_path = "goals_edges.db") {
    json result;
    result["nodes"] = json::array();
    result["edges"] = json::array();

    sqlite3 *db;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return result;
    }

    const char *query = "SELECT person1, person2, shared_goal FROM share_goals;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return result;
    }

    std::set<std::string> unique_nodes;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string person1 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string person2 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string goal = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

        if (unique_nodes.insert(person1).second) {
            result["nodes"].push_back({{"id", person1}});
        }
        if (unique_nodes.insert(person2).second) {
            result["nodes"].push_back({{"id", person2}});
        }

        result["edges"].push_back({
            {"source", person1},
            {"target", person2},
            {"label", goal}
        });
        result["edges"].push_back({
            {"source", person2},
            {"target", person1},
            {"label", goal}
        });
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << result.dump(2) << std::endl;

    return result;
}


int main(int argc, char* argv[]) {
    //std::cout << "[DEBUG] Entered main()\n";

    if (argc != 2) {
        std::cerr << "Usage: ./generate_edges <feature_column>" << std::endl;
        return 1;
    }

    std::string feature_column = argv[1];
    sqlite3* db;
    if (sqlite3_open("my_database.db", &db)) {
        std::cerr << "Can't open DB\n";
        return 1;
    }

    std::vector<entry> all_entries;

    std::string query = R"(
        SELECT 
            contacts.name, contacts.email, contacts.phone, contacts.location,
            employment.current_company, employment.previous_companies, employment.industry, employment.job_title,
            relationships.relationship_type, relationships.closeness, relationships.reliability,
            profile.career_goals, profile.skills, profile.talent_rating
        FROM contacts
        LEFT JOIN employment ON contacts.id = employment.contact_id
        LEFT JOIN background ON contacts.id = background.contact_id
        LEFT JOIN profile ON contacts.id = profile.contact_id
        LEFT JOIN relationships ON contacts.id = relationships.contact_id;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Query error\n";
        return 1;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        entry e;
        e.name          = getSafeText(stmt, 0);
        e.email         = getSafeText(stmt, 1);
        e.phone         = getSafeText(stmt, 2);
        e.location      = getSafeText(stmt, 3);
        e.currCompany   = getSafeText(stmt, 4);
        e.prevCompanies = getSafeText(stmt, 5);
        e.industry      = getSafeText(stmt, 6);
        e.jobTitle      = getSafeText(stmt, 7);
        e.relationship  = getSafeText(stmt, 8);
        e.closeness     = getSafeText(stmt, 9);
        e.reliability   = getSafeText(stmt, 10);
        e.careerGoals   = getSafeText(stmt, 11);
        e.skills        = getSafeText(stmt, 12);
        e.talent        = getSafeText(stmt, 13);
        all_entries.push_back(e);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    //generate_edges_by_location(all_entries);

    // 🔧 Use the feature_column to route to specific logic (hooked here)
    if (feature_column == "") {
        generate_edges_by_default(all_entries);
    } else if (feature_column == "location") {
        generate_edges_by_location(all_entries);
    } else if (feature_column == "current_company") {
        generate_edges_by_current_company();
    } else if (feature_column == "previous_companies") {
        generate_edges_by_previous_company();
    } else if (feature_column == "industry") {
        generate_edges_by_industry();
    } else if (feature_column == "interests") {
        generate_edges_by_shared_interests();
    } else if (feature_column == "college") {
        generate_edges_by_college();
    } else if (feature_column == "high_school") {
        generate_edges_by_high_school();
    } else if (feature_column == "career_goals") {
        generate_edges_by_shared_goals();
    } else if (feature_column == "skills") {
        generate_edges_by_shared_skills();
    } else if (feature_column == "talent_rating") {
        generate_edges_by_talent(all_entries);
    } else if (feature_column == "closeness") {
        generate_edges_by_closeness(all_entries);
    } else {
        std::cerr << "Unknown feature column: " << feature_column << std::endl;
        return 1;
    }
    return 0;
}

/* Compile using:
    gcc -c sqlite3.c -o sqlite3.o
    g++ generate_edges.cpp sqlite3.o -o generate_edges -g -I. -I./curl -I./nlohmann -L./lib -lcurl -lssl -lcrypto -lz -lws2_32 -lbrotlidec -lbrotlicommon
*/
