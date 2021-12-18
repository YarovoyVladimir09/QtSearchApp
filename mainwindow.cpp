#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>
#include <optional>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "documentadd.h"
#include <QMessageBox>
using namespace std;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
   window_Doc= new DocumentAdd;

   window_Doc->move(500,600);
   window_Doc->show();

    connect(window_Doc,&DocumentAdd::signal,this,&MainWindow::slotAddDocum);

}
QString id;
QString relevance;
QString rating;


MainWindow::~MainWindow()
{
    delete ui;
}
QString stop_words_from="";

void MainWindow::on_ButtonToAddStopWords_clicked()
{
    stop_words_from=ui->Stop_Word_Line->text();
}


void MainWindow::on_ButtonToDocument_clicked()
{


    window_Doc->setModal(true);


    window_Doc->exec();
}
void MainWindow::Print(QString id,QString rating,QString relevance)
{
    ui->label_id->setText(id);
    ui->label_relevance->setText(relevance);
    ui->label_rating->setText(rating);
}


const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}
vector<int> SplitIntoNumb(const string& text) {
    vector<int> rating;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                rating.push_back(atoi(word.c_str()));
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        rating.push_back(atoi(word.c_str()));
    }

    return rating;
}

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        for (string word : stop_words_)
            if (!IsValidWord(word))
                throw invalid_argument("Использование специальных символов");
    }

    explicit SearchServer(const string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0 || ind_.count(document_id) || !IsValidWord(document))
        {
            throw invalid_argument("Неправильно указан id документа или документ содержит спец символы");
        }
        ind_[document_id] = ++index_;
        const vector<string> words = SplitIntoWordsNoStop(document);

        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });

    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        if (!ValidateQuery(raw_query) || !IsValidWord(raw_query))
        {
            throw invalid_argument("Использование спец символов или нелогичное использование (-)");
        }
        const Query query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            const double err = abs(lhs.relevance - rhs.relevance);
            if (err < 1e-6) {
                return !(lhs.rating > rhs.rating);
            }
            else {
                return !(lhs.relevance > rhs.relevance);
            }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
            });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(int index) const {
        if (!ind_.count(index))
            throw out_of_range("Не найден документ по индексу");
        return ind_.at(index);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        if (!ValidateQuery(raw_query) || !IsValidWord(raw_query))
        {
            throw invalid_argument("Использование спец символов или нелогичное использование (-)");
        }
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return tie(matched_words, documents_.at(document_id).status);
    }
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    map<int, int> ind_;
    int index_ = 0;
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    bool ValidateQuery(const string& text) const
    {
        vector<string> check = SplitIntoWords(text);
        for (string word : check)
        {
            int i = 0;
            for (char c : word)
            {
                if (c == '-')
                    i++;
            }
            if (i >= 2)
                return false;
        }
        if (*(check.end() - 1) == "-" || !IsValidWord(text))
            return false;
        return true;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }
    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(),ratings.end(),0);

        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
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

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

// ------------ Пример использования ----------------

void PrintDocument(const Document& document) {
   /* cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;*/
id=QString::number(document.id);
relevance=QString::number(document.relevance);
rating =QString::number(document.rating);
   //ui.Print(id,relevance,rating);

}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast<int>(status) << ", "s
        << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
    const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const exception& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const exception& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const exception& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}
        DocumentStatus Translation(const string& status)
        {
            if (status=="ACTUAL"||"actual"||"Actual")
                return DocumentStatus::ACTUAL;
            else if (status=="IRRELEVANT"||"irrelevant"||"Irrelevant")
                return DocumentStatus::IRRELEVANT;
            else if(status=="BANNED"||"Banned"||"banned")
                return DocumentStatus::BANNED;
          else if (status=="REMOVED"||"Removed"||"removed")
                return DocumentStatus::REMOVED;
        }


 SearchServer search_server(stop_words_from.toStdString());
        void MainWindow::slotAddDocum(QString document1, QString document_id_string, QString status_string, QString rating_string)
        {

        string document=document1.toStdString();
        int document_id=atoi(document_id_string.toStdString().c_str());
        DocumentStatus status=Translation(status_string.toStdString());
        vector<int> rating=SplitIntoNumb(rating_string.toStdString());

        AddDocument(search_server, document_id,document, status,rating);
        }





void MainWindow::on_ButtonForSearch_clicked()
{
    QString q_search=ui->SearchLine->text();
    string search=q_search.toStdString();
    FindTopDocuments(search_server,search);
    Print(id,relevance,rating);
}

