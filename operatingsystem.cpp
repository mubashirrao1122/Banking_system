#include <iostream>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>
#include <queue>
#include <list>
#include <map>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <string> // Include this header for std::to_string

using namespace std;

const int PAGE_SIZE = 4096; // Size of a memory page in bytes
const int MEMORY_SIZE = 10 * PAGE_SIZE; // Total memory size

enum class ErrorCode {
    SUCCESS,
    ACCOUNT_ALREADY_EXISTS,
    ACCOUNT_NOT_FOUND,
    INSUFFICIENT_FUNDS
};

// Implement comparison operators for ErrorCode
bool operator==(const ErrorCode& lhs, const ErrorCode& rhs) {
    return static_cast<int>(lhs) == static_cast<int>(rhs);
}

bool operator!=(const ErrorCode& lhs, const ErrorCode& rhs) {
    return !(lhs == rhs);
}

// Overload operator<< for ErrorCode to handle ambiguous operator<<
ostream& operator<<(ostream& os, const ErrorCode& code) {
    switch (code) {
    case ErrorCode::SUCCESS:
        os << "SUCCESS";
        break;
    case ErrorCode::ACCOUNT_ALREADY_EXISTS:
        os << "ACCOUNT_ALREADY_EXISTS";
        break;
    case ErrorCode::ACCOUNT_NOT_FOUND:
        os << "ACCOUNT_NOT_FOUND";
        break;
    case ErrorCode::INSUFFICIENT_FUNDS:
        os << "INSUFFICIENT_FUNDS";
        break;
    }
    return os;
}

class BankSystem {
public:
    ErrorCode create_account(int customer_id, double initial_balance) {
        lock_guard<mutex> lock(mtx);
        if (accounts.find(customer_id) != accounts.end()) {
            return ErrorCode::ACCOUNT_ALREADY_EXISTS; // Account already exists
        }
        accounts[customer_id] = initial_balance;
        allocate_page(customer_id);
        return ErrorCode::SUCCESS;
    }

    ErrorCode deposit(int account_id, double amount) {
        lock_guard<mutex> lock(mtx);
        if (accounts.find(account_id) == accounts.end()) {
            return ErrorCode::ACCOUNT_NOT_FOUND; // Account does not exist
        }
        accounts[account_id] += amount;
        log_transaction(account_id, "deposit", amount);
        notify_transaction_complete(account_id, "deposit", amount);
        return ErrorCode::SUCCESS;
    }

    ErrorCode withdraw(int account_id, double amount) {
        lock_guard<mutex> lock(mtx);
        if (accounts.find(account_id) == accounts.end()) {
            return ErrorCode::ACCOUNT_NOT_FOUND; // Account does not exist
        }
        if (accounts[account_id] < amount) {
            return ErrorCode::INSUFFICIENT_FUNDS; // Insufficient funds
        }
        accounts[account_id] -= amount;
        log_transaction(account_id, "withdraw", amount);
        notify_transaction_complete(account_id, "withdraw", amount);
        return ErrorCode::SUCCESS;
    }

    pair<ErrorCode, double> check_balance(int account_id) {
        lock_guard<mutex> lock(mtx);
        if (accounts.find(account_id) == accounts.end()) {
            return { ErrorCode::ACCOUNT_NOT_FOUND, -1 }; // Account does not exist
        }
        return { ErrorCode::SUCCESS, accounts[account_id] };
    }

    void create_transaction_process(ErrorCode(BankSystem::* transaction)(int, double), int account_id, double amount) {
        lock_guard<mutex> lock(mtx);
        transaction_queue.push([=] { (this->*transaction)(account_id, amount); });
    }

    void run_scheduler() {
        while (!transaction_queue.empty()) {
            auto transaction = transaction_queue.front();
            transaction_queue.pop();
            transaction();
            this_thread::sleep_for(chrono::milliseconds(time_quantum));
        }
    }

    void set_time_quantum(int tq) {
        time_quantum = tq;
    }

    void display_memory_map() {
        lock_guard<mutex> lock(mtx);
        cout << "Memory Map:" << endl;
        for (const auto& page : memory_pages) {
            cout << "Page " << page.first << ": Account " << page.second << endl;
        }
    }

    void wait_for_transaction_complete() {
        unique_lock<mutex> lock(cv_mtx);
        cv.wait(lock, [this] { return !message_queue.empty(); });
        while (!message_queue.empty()) {
            auto message = message_queue.front();
            message_queue.pop();
            cout << "Transaction complete: " << message << endl;
        }
    }

private:
    unordered_map<int, double> accounts;
    mutex mtx;
    queue<function<void()>> transaction_queue;
    int time_quantum = 100; // Default time quantum in milliseconds

    list<int> lru_list; // List to track LRU pages
    map<int, int> memory_pages; // Map to track page allocations (page number -> account ID)

    queue<string> message_queue; // Message queue for IPC
    condition_variable cv; // Condition variable for IPC
    mutex cv_mtx; // Mutex for condition variable

    void allocate_page(int account_id) {
        if (memory_pages.size() >= MEMORY_SIZE / PAGE_SIZE) {
            // Memory is full, apply LRU page replacement
            int lru_page = lru_list.back();
            lru_list.pop_back();
            memory_pages.erase(lru_page);
        }
        int new_page = static_cast<int>(memory_pages.size());
        memory_pages[new_page] = account_id;
        lru_list.push_front(new_page);
    }

    void log_transaction(int account_id, const string& type, double amount) {
        // Simulate logging transaction to memory
        allocate_page(account_id);
        ofstream log_file("transaction_log.txt", ios::app);
        log_file << "Logged " << type << " of " << amount << " for account " << account_id << endl;
        log_file.close();
    }

    void notify_transaction_complete(int account_id, const string& type, double amount) {
        lock_guard<mutex> lock(cv_mtx);
        message_queue.push("Account " + to_string(account_id) + ": " + type + " of " + to_string(amount));
        cv.notify_all();
    }
};

static void simulate_transactions(BankSystem& bank, int account_id) {
    bank.create_transaction_process(&BankSystem::deposit, account_id, 500.0);
    bank.create_transaction_process(&BankSystem::withdraw, account_id, 200.0);
}

static void handle_error(ErrorCode code) {
    switch (code) {
    case ErrorCode::SUCCESS:
        cout << "Operation successful." << endl;
        break;
    case ErrorCode::ACCOUNT_ALREADY_EXISTS:
        cout << "Error: Account already exists." << endl;
        break;
    case ErrorCode::ACCOUNT_NOT_FOUND:
        cout << "Error: Account not found." << endl;
        break;
    case ErrorCode::INSUFFICIENT_FUNDS:
        cout << "Error: Insufficient funds." << endl;
        break;
    }
}

int main()
{
    BankSystem bank;
    ErrorCode result;

    // Create account
    result = bank.create_account(1, 1000.0);
    handle_error(result);

    // Simulate concurrent transactions
    thread t1(simulate_transactions, ref(bank), 1);
    thread t2(simulate_transactions, ref(bank), 1);

    t1.join();
    t2.join();

    // Set time quantum for Round Robin scheduling
    bank.set_time_quantum(100);

    // Run the scheduler
    bank.run_scheduler();

    // Check balance
    auto [code, balance] = bank.check_balance(1);
    handle_error(code);
    if (code == ErrorCode::SUCCESS) {
        cout << "Balance for account 1: " << balance << "\n";
    }

    // Display memory map
    bank.display_memory_map();

    // Wait for transaction completions
    bank.wait_for_transaction_complete();

    // Run program: Ctrl + F5 or Debug > Start Without Debugging menu
    // Debug program: F5 or Debug > Start Debugging menu

    // Tips for Getting Started: 
    //   1. Use the Solution Explorer window to add/manage files
    //   2. Use the Team Explorer window to connect to source control
    //   3. Use the Output window to see build output and other messages
    //   4. Use the Error List window to view errors
    //   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
    //   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
}

