//24.01.2022 sprint2

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}
// Тест проверяет, что поисковая система исключает минус-слова
void TestExcludeMinusWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const string minus_words = "cat in the park -city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_HINT(server.FindTopDocuments(minus_words).empty(), "Test error"s);

    }
}
// Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestExcludeReturnMatchDocumentWithMinusWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    //const string search_word = "in the"s;
    const vector<string> result_words = {"in", "the"};
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words1, status1] = server.MatchDocument("in the", doc_id);
        ASSERT_EQUAL(words1, result_words);
        const auto [words2, status2] = server.MatchDocument("in the -city", doc_id);
        ASSERT_HINT(words2.empty(), "Контейнер не пуст при налчии минус слова."s);

    }
}
// Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortDocumentTopToDownRelevance() {
    const int doc_id1 = 42;
    const int doc_id2 = 43;
    const string content1 = "cat in the city"s;
    const string content2 = "cat in the park"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat park"s);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        //auto compare_relevance = [doc0, doc1](){return (doc0.relevance > doc1.relevance);};
        ASSERT_HINT((doc0.relevance > doc1.relevance), "Неправильная сортировка релевантности."s);
    }
}
// Рейтинг добавленного документа равен среднему арифметическому оценок документа
void TestDocumentArithmeticMeanRaiting() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3}; // = 2
    //const int mid_rating = std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size()); //тренажёр не принимает без numeric
    int sum_ratings = 0;
    for(const int rating : ratings) {
        sum_ratings += rating;
    }
    const int mid_rating = sum_ratings / static_cast<int>(ratings.size());
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in the"s);
        const Document& doc = found_docs[0];
        ASSERT_EQUAL(doc.rating, mid_rating);
    }
}
// Фильтрация результатов поиска с использованием предиката
void TestFilteringSearchUsingPredicate() {
    const int doc_id = 2;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, "белый кот и модный ошейник"s, DocumentStatus::REMOVED, {8, -3});
        auto predicate = [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; };
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, predicate);
        const Document& doc = found_docs[0];
        ASSERT_EQUAL(doc.id, doc_id);
    }
}
// Поиск документов, имеющих заданный статус.
void TestDocumentsSearchWithGivenStatus() {
    const int doc_id1 = 42;
    const int doc_id2 = 43;
    const int doc_id3 = 44;
    const string content1 = "cat in the city"s;
    const string content2 = "cat in the park"s;
    const string content3 = "dog in the park"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings);
        server.AddDocument(doc_id3, content2, DocumentStatus::REMOVED, ratings);
        const auto found_docs_ACTUAL = server.FindTopDocuments("in the"s, DocumentStatus::ACTUAL);
        const auto found_docs_BANNED = server.FindTopDocuments("in the"s, DocumentStatus::BANNED);
        const auto found_docs_REMOVED = server.FindTopDocuments("in the"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(found_docs_ACTUAL.size(), 1u);
        ASSERT_EQUAL(found_docs_BANNED.size(), 1u);
        ASSERT_EQUAL(found_docs_REMOVED.size(), 1u);
    }
}
// Корректное вычисление релевантности найденных документов.
void TestCalculationCorrectRelevanceOfTheDocument() {
    const int doc_id1 = 42;
    const string content1 = "белый кот и модный ошейник"s;
    const string seach_string = "кот"s;
    const vector<int> ratings = {1, 2, 3};
    const double tf = (1.0 / 5.0); // freq search word(str) /-div-/ count word in doc
    const double idf = log(1.0 / 1.0); // count doc /-div-/ count word where word(str) include in doc
    const double relevance_doc = (tf * idf);
    {
        SearchServer server;
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(seach_string);
        const Document& doc0 = found_docs[0];

        auto CompareRelevance = [doc0, relevance_doc](){
            const double EPSILON = 1e-6;
            return (abs(doc0.relevance - relevance_doc) < EPSILON);
        };
        ASSERT_HINT(CompareRelevance(), "Релевантность не совпадает."s);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeReturnMatchDocumentWithMinusWords);
    RUN_TEST(TestSortDocumentTopToDownRelevance);
    RUN_TEST(TestDocumentArithmeticMeanRaiting);
    RUN_TEST(TestFilteringSearchUsingPredicate);
    RUN_TEST(TestDocumentsSearchWithGivenStatus);
    RUN_TEST(TestCalculationCorrectRelevanceOfTheDocument);
}

// --------- Окончание модульных тестов поисковой системы -----------