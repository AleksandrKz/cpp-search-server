#include "remove_duplicates.h"

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>

void RemoveDuplicates(SearchServer& search_server){
    std::vector<int> candidates_to_delete;
    std::set<std::set<std::string>> all_documents;

    for (auto it = search_server.begin(); it != search_server.end(); ++it) {
        std::map<std::string, double> current_document = search_server.GetWordFrequencies(*it);
        std::set<std::string> document_words;
        for (const auto& i : current_document){
            document_words.insert(i.first);
        }
        if (all_documents.count(document_words)) {
            candidates_to_delete.push_back(*it);
        } else {
            all_documents.insert(document_words);
        }
    }
    
    for(const int id : candidates_to_delete){
        search_server.RemoveDocument(id);
       std::cout << "Found duplicate document id " << id << std::endl;
    }
}