#pragma once

#include <utility>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <string>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <iostream>
#include <execution>
#include <numeric>
#include <functional>
#include <future>

#include "document.h"
#include "concurrent_map.h"
#include "string_processing.h"

#include "paginator.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    
    explicit SearchServer(std::string_view stop_words_text);
    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    //old
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const ;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    //policy
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const;
    
    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const;

    std::set<int>::const_iterator begin() const;

    std::set<int>::const_iterator end() const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string text;
    };
        
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    
    bool IsStopWord(std::string_view word) const;
    
    static bool IsValidWord(std::string_view word);
    
    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;
    
    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(const std::string_view text) const;
    
    Query ParseQuery(std::string_view text, bool skip_sort = false) const;
    // Existence required
    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const;
};


template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        	using namespace std::string_literals;
            throw std::invalid_argument("Some of stop words are invalid"s);
        }
    }

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(policy, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            return lhs.relevance > rhs.relevance
            || (std::abs(lhs.relevance - rhs.relevance) < EPSILON && lhs.rating > rhs.rating);
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [&status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]] int rating) {
        return document_status == status;
    });
}

template <typename ExecutionPolicy> 
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    return FindAllDocuments(std::execution::seq, query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(GetDocumentCount());

    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [this, &document_to_relevance, &document_predicate](const std::string_view word){
        if (word_to_document_freqs_.count(word) > 0) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        }
    });
    
    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [this, &document_to_relevance](const std::string_view word){
        if (word_to_document_freqs_.count(word) > 0) {
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.Erase(document_id);
            }
        }
    });
    
    auto doc_to_rel = document_to_relevance.BuildOrdinaryMap();
    std::vector<Document> matched_documents;
    matched_documents.reserve(doc_to_rel.size());
    for(const auto& [document_id, term_freq] : doc_to_rel) {
    	matched_documents.push_back(Document(document_id, term_freq, documents_.at(document_id).rating));
    }
    return matched_documents;
}