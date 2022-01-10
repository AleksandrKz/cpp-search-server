//24.01.2022 sprint2


// для вывода вектора
template <typename Container>
void Print(ostream& out, const Container& cont){
    bool first = true;
    for (const auto& element : cont) {
        if(!first)
            out <<  ", "s;
        out << element;
        first = false;
    }
}

// для вывода вектора
template <typename Element>
ostream& operator<<(ostream& out, const vector<Element>& container) {
    out << "["s;
    Print(out, container);
    out << "]"s;
    return out;
} 

/* 
   Подставьте сюда вашу реализацию макросов 
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

template <typename FUNC>
void RunTestImpl(FUNC func, const string& func_name) {
    func();
    cerr << func_name << " OK"s << endl;
}

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
void TestMinusWords() {
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
void TestMathDocument() {
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
void TestSortDocRelevance() {
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
void TestRaiting() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3}; // = 2
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in the"s);
        const Document& doc = found_docs[0];
        ASSERT_EQUAL(doc.rating, 2);
    }
}
// Фильтрация результатов поиска с использованием предиката
void TestPredicate() {
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
void TestStatus() {
    const int doc_id1 = 42;
    const int doc_id2 = 43;
    const string content1 = "cat in the city"s;
    const string content2 = "cat in the park"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings);
        const auto found_docs = server.FindTopDocuments("in the"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs.size(), 1u);
    }
}
// Корректное вычисление релевантности найденных документов.
void TestRelevance() {
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
        ASSERT_EQUAL(doc0.relevance, relevance_doc);// подсчитать
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMathDocument);
    RUN_TEST(TestSortDocRelevance);
    RUN_TEST(TestRaiting);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestStatus);
    RUN_TEST(TestRelevance);
}

// --------- Окончание модульных тестов поисковой системы -----------