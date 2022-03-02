#include "remove_duplicates.h"

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>

void RemoveDuplicates(SearchServer& search_server){
    std::vector<int> candidate;
    std::set<std::set<std::string>> vectors;

    for (auto it = search_server.begin(); it != search_server.end(); ++it) {
        std::map<std::string, double> move = search_server.GetWordFrequencies(*it);
        std::set<std::string> move_str;
        for (auto i : move){
            move_str.insert(i.first);
        }
        if (vectors.count(move_str)) {
            candidate.push_back(*it);
        } else {
            vectors.insert(move_str);
        }
    }
    
    for(const int id : candidate){
        search_server.RemoveDocument(id);
       std::cout << "Found duplicate document id " << id << std::endl;
    }
}