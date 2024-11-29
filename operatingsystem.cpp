/*
/*
Project: A Simple Banking System Simulation in C++

Objective:
Simulate a banking system that allows multiple customers to perform transactions concurrently. The system will demonstrate core operating system concepts, including process creation, multithreading, synchronization, CPU scheduling, memory management, inter-process communication (IPC), logging, and error handling.

---

Functional Requirements:
1. **System Call Interface**:
   - Implement API functions for customer operations:
     - `create_account(int customer_id, float initial_balance)`: Create a new account with a unique account ID.
     - `deposit(int account_id, float amount)`: Deposit money into a specified account.
     - `withdraw(int account_id, float amount)`: Withdraw money from an account, with error handling for insufficient funds.
     - `check_balance(int account_id)`: Retrieve the current balance of an account.

2. **Process Creation**:
   - Each transaction (e.g., deposit, withdrawal) is treated as a separate process.
   - Maintain a **process table** that tracks:
     - Transaction ID
     - Process state (e.g., Ready, Running, Waiting, Terminated)
     - Associated account and customer details.

3. **Multithreading & Synchronization**:
   - Use threads for concurrent execution of transactions.
   - Synchronize shared account data access using:
     - `std::mutex` or `std::semaphore` for locking.
     - Prevent race conditions or data inconsistencies.
   - Ensure threads for a single customer handle transactions safely.

4. **CPU Scheduling**:
   - Implement a **Round Robin scheduler**:
     - Allocate fixed time slices (quantum) to each transaction process.
     - Track metrics like **average waiting time** and **CPU utilization**.
   - Visualize scheduling with a **Gantt chart**.

5. **Memory Management**:
   - Divide memory into **pages** for storing:
     - Account details (e.g., balances, IDs).
     - Transaction logs.
   - Use the **Least Recently Used (LRU)** algorithm for page replacement.
   - Display a **memory map** showing how pages are allocated and replaced.

6. **Inter-Process Communication (IPC)**:
   - Use **message queues** to enable communication between processes.
   - Support both synchronous (blocking) and asynchronous (non-blocking) communication.
   - Notify processes about transaction completions or errors.

7. **Logging**:
   - Maintain a **transaction log file** (`transactions.log`):
     - Record transaction details (type, account ID, amount, timestamp).
   - Maintain an **error log file** (`errors.log`):
     - Log errors like insufficient funds, invalid account IDs, or deadlocks.
   - Use `std::fstream` for file handling.

8. **Error Handling**:
   - Handle errors gracefully:
     - Insufficient funds during withdrawals.
     - Invalid account IDs.
     - Deadlocks or resource contention issues.
   - Provide meaningful error messages to users.
   - Log all errors for debugging and analysis.

---

Additional Technical Requirements:
1. **Modular Design**:
   - Implement each feature as a separate class or function.
   - Example modules:
     - `AccountManager`: For account operations.
     - `ProcessManager`: For process creation and management.
     - `Scheduler`: For CPU scheduling.
     - `MemoryManager`: For paging and memory management.
     - `Logger`: For transaction and error logging.
     - `IPCManager`: For message queue handling.

2. **Libraries to Use**:
   - `#include <thread>` for multithreading.
   - `#include <mutex>` for synchronization.
   - `#include <queue>` for message queues.
   - `#include <map>` and `#include <list>` for data structures.
   - `#include <fstream>` for file logging.

3. **Best Practices**:
   - Use **comments** to explain critical sections.
   - Ensure thread safety using locks and proper synchronization.
   - Follow modern C++ standards (C++17 or above).
   - Prioritize clean, readable, and maintainable code.

---

Output Expectations:
1. A fully functional C++ program demonstrating:
   - Account creation, deposit, withdrawal, and balance checking.
   - Concurrent transactions with synchronized access.
   - CPU scheduling with metrics and a Gantt chart.
   - Efficient memory management using paging and LRU replacement.
   - Logging and error handling for all operations.
2. Detailed comments explaining each module and function.

---

Example User Scenarios:
1. **Account Creation**:
   - Customer creates an account with an initial balance.
   - System assigns a unique account ID and stores the details in memory.

2. **Concurrent Transactions**:
   - Customer performs a deposit and withdrawal simultaneously.
   - Threads handle these operations while synchronizing access to shared account data.

3. **CPU Scheduling**:
   - Multiple transaction processes are scheduled using the Round Robin algorithm.
   - Time slices are visualized in a Gantt chart.

4. **Error Scenarios**:
   - A withdrawal request fails due to insufficient funds.
   - Error is logged and displayed to the user.

5. **Memory Overflow**:
   - Memory for account details exceeds the allocated limit.
   - The LRU algorithm replaces old pages with new ones.

---

Write the complete C++ implementation for the above system. Ensure all modules, synchronization, logging, error handling, and visualization are implemented as described. Follow clean and modular coding practices.
*/
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <fstream>
#include <queue>
#include <list>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <atomic>
#include <iomanip>
#include <ctime>
#include <string>

using namespace std;

// Logger class for transaction and error logging
class Logger {
private:
    ofstream transaction_log;
    ofstream error_log;
    mutex log_mtx;

    string get_current_time() {
        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);
        char buffer[26];
        ctime_s(buffer, sizeof(buffer), &now_time);
        buffer[24] = '\0'; // Remove the newline character
        return string(buffer);
    }

public:
    Logger() {
        transaction_log.open("transactions.log", ios::app);
        error_log.open("errors.log", ios::app);
    }

    ~Logger() {
        transaction_log.close();
        error_log.close();
    }

    void log_transaction(const string& message) {
        lock_guard<mutex> lock(log_mtx);
        transaction_log << "[" << get_current_time() << "] " << message << endl;
    }

    void log_error(const string& message) {
        lock_guard<mutex> lock(log_mtx);
        error_log << "[" << get_current_time() << "] " << message << endl;
    }
};

// AccountManager class for account operations
class AccountManager {
private:
    struct Account {
        int account_id;
        int customer_id;
        float balance;
    };

    map<int, Account> accounts;
    mutex mtx;
    int next_account_id = 1;
    Logger& logger;

public:
    AccountManager(Logger& logger) : logger(logger) {}

    int add_account(int customer_id, float initial_balance) {
        lock_guard<mutex> lock(mtx);
        int account_id = next_account_id++;
        accounts[account_id] = { account_id, customer_id, initial_balance };
        logger.log_transaction("Account created: ID=" + to_string(account_id) + ", Initial Balance=" + to_string(initial_balance));
        return account_id;
    }

    Account get_account(int account_id) {
        lock_guard<mutex> lock(mtx);
        if (accounts.find(account_id) != accounts.end()) {
            return accounts[account_id];
        }
        logger.log_error("Get account failed: Invalid Account ID=" + to_string(account_id));
        return { -1, -1, -1.0f }; // Indicate invalid account
    }

    bool update_balance(int account_id, float new_balance) {
        lock_guard<mutex> lock(mtx);
        if (accounts.find(account_id) != accounts.end()) {
            accounts[account_id].balance = new_balance;
            logger.log_transaction("Balance updated: Account ID=" + to_string(account_id) + ", New Balance=" + to_string(new_balance));
            return true;
        }
        logger.log_error("Update balance failed: Invalid Account ID=" + to_string(account_id));
        return false;
    }

    bool delete_account(int account_id) {
        lock_guard<mutex> lock(mtx);
        if (accounts.erase(account_id)) {
            logger.log_transaction("Account deleted: ID=" + to_string(account_id));
            return true;
        }
        logger.log_error("Delete account failed: Invalid Account ID=" + to_string(account_id));
        return false;
    }

    bool deposit(int account_id, float amount) {
        lock_guard<mutex> lock(mtx);
        if (accounts.find(account_id) != accounts.end()) {
            accounts[account_id].balance += amount;
            logger.log_transaction("Deposit: Account ID=" + to_string(account_id) + ", Amount=" + to_string(amount));
            return true;
        }
        logger.log_error("Deposit failed: Invalid Account ID=" + to_string(account_id));
        return false;
    }

    bool withdraw(int account_id, float amount) {
        lock_guard<mutex> lock(mtx);
        if (accounts.find(account_id) != accounts.end() && accounts[account_id].balance >= amount) {
            accounts[account_id].balance -= amount;
            logger.log_transaction("Withdrawal: Account ID=" + to_string(account_id) + ", Amount=" + to_string(amount));
            return true;
        }
        logger.log_error("Withdrawal failed: Insufficient funds or Invalid Account ID=" + to_string(account_id));
        return false;
    }

    float check_balance(int account_id) {
        lock_guard<mutex> lock(mtx);
        if (accounts.find(account_id) != accounts.end()) {
            return accounts[account_id].balance;
        }
        logger.log_error("Check balance failed: Invalid Account ID=" + to_string(account_id));
        return -1.0f; // Indicate invalid account
    }
};

// ProcessManager class for process creation and management
class ProcessManager {
private:
    struct Process {
        int transaction_id;
        string state;
        int account_id;
        int customer_id;

        // Constructor to initialize member variables
        Process(int t_id, const string& st, int a_id, int c_id)
            : transaction_id(t_id), state(st), account_id(a_id), customer_id(c_id) {
        }
    };

    map<int, Process> process_table;
    mutex mtx;
    int next_transaction_id = 1;

public:
    int create_transaction_process(int customer_id, int account_id) {
        lock_guard<mutex> lock(mtx);
        int transaction_id = next_transaction_id++;
        process_table[transaction_id] = Process(transaction_id, "Ready", account_id, customer_id);
        return transaction_id;
    }

    void terminate_transaction_process(int transaction_id) {
        lock_guard<mutex> lock(mtx);
        if (process_table.find(transaction_id) != process_table.end()) {
            process_table[transaction_id].state = "Terminated";
            process_table.erase(transaction_id);
        }
    }

    void update_process_state(int transaction_id, const string& state) {
        lock_guard<mutex> lock(mtx);
        if (process_table.find(transaction_id) != process_table.end()) {
            process_table[transaction_id].state = state;
        }
    }

    void remove_process(int transaction_id) {
        lock_guard<mutex> lock(mtx);
        process_table.erase(transaction_id);
    }

    map<int, Process> get_process_table() {
        lock_guard<mutex> lock(mtx);
        return process_table;
    }
};

// Scheduler class for CPU scheduling
class Scheduler {
private:
    queue<int> ready_queue;
    mutex mtx;
    condition_variable cv;
    atomic<bool> running;
    ProcessManager& process_manager;
    vector<pair<int, string>> gantt_chart;
    int time_slice;

public:
    Scheduler(ProcessManager& pm, int ts) : process_manager(pm), running(true), time_slice(ts) {}

    void add_to_ready_queue(int transaction_id) {
        lock_guard<mutex> lock(mtx);
        ready_queue.push(transaction_id);
        cv.notify_one();
    }

    void stop() {
        running = false;
        cv.notify_all();
    }

    void run() {
        while (running) {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [this] { return !ready_queue.empty() || !running; });

            if (!running) break;

            int transaction_id = ready_queue.front();
            ready_queue.pop();
            lock.unlock();

            process_manager.update_process_state(transaction_id, "Running");
            this_thread::sleep_for(chrono::milliseconds(time_slice)); // Simulate process execution
            process_manager.update_process_state(transaction_id, "Terminated");

            gantt_chart.push_back({ transaction_id, "Running" });
        }
    }

    void display_gantt_chart() {
        cout << "Gantt Chart:" << endl;
        for (const auto& entry : gantt_chart) {
            cout << "Transaction ID: " << entry.first << " - State: " << entry.second << endl;
        }
    }
};

// MemoryManager class for paging and memory management
class MemoryManager {
private:
    struct Page {
        int account_id;
        float balance;
    };

    list<Page> memory;
    size_t max_pages;
    mutex mtx;

public:
    MemoryManager(size_t max_pages) : max_pages(max_pages) {}

    void store_data_in_page(int account_id, float balance) {
        lock_guard<mutex> lock(mtx);
        if (memory.size() >= max_pages) {
            replace_page(account_id, balance);
        }
        else {
            memory.push_back({ account_id, balance });
        }
    }

    void replace_page(int account_id, float balance) {
        lock_guard<mutex> lock(mtx);
        memory.pop_front(); // Remove the least recently used page
        memory.push_back({ account_id, balance });
    }

    void display_memory_map() {
        lock_guard<mutex> lock(mtx);
        cout << "Memory Map:" << endl;
        for (const auto& page : memory) {
            cout << "Account ID: " << page.account_id << ", Balance: " << page.balance << endl;
        }
    }
};

// IPCManager class for message queue handling
class IPCManager {
private:
    queue<string> message_queue;
    mutex mtx;
    condition_variable cv;

public:
    void send_message(const string& message) {
        lock_guard<mutex> lock(mtx);
        message_queue.push(message);
        cv.notify_one();
    }

    string receive_message(bool blocking = true) {
        unique_lock<mutex> lock(mtx);
        if (blocking) {
            cv.wait(lock, [this] { return !message_queue.empty(); });
        }
        else {
            if (message_queue.empty()) {
                return "";
            }
        }
        string message = message_queue.front();
        message_queue.pop();
        return message;
    }
};

// Error Handling Module
class ErrorHandler {
private:
    Logger& logger;

public:
    ErrorHandler(Logger& logger) : logger(logger) {}

    void handle_error(const string& error_message) {
        logger.log_error(error_message);
    }

    bool validate_account_id(int account_id, AccountManager& accountManager) {
        if (accountManager.get_account(account_id).account_id == -1) {
            handle_error("Invalid Account ID: " + to_string(account_id));
            return false;
        }
        return true;
    }

    bool validate_amount(float amount) {
        if (amount <= 0) {
            handle_error("Invalid amount: " + to_string(amount));
            return false;
        }
        return true;
    }
};

// System Call Interface Module
class SystemCallInterface {
private:
    AccountManager& accountManager;
    ErrorHandler& errorHandler;

public:
    SystemCallInterface(AccountManager& am, ErrorHandler& eh) : accountManager(am), errorHandler(eh) {}

    int create_account(int customer_id, float initial_balance) {
        if (initial_balance < 0) {
            errorHandler.handle_error("Create account failed: Initial balance cannot be negative.");
            return -1;
        }
        return accountManager.add_account(customer_id, initial_balance);
    }

    bool deposit(int account_id, float amount) {
        if (!errorHandler.validate_account_id(account_id, accountManager) || !errorHandler.validate_amount(amount)) {
            return false;
        }
        return accountManager.deposit(account_id, amount);
    }

    bool withdraw(int account_id, float amount) {
        if (!errorHandler.validate_account_id(account_id, accountManager) || !errorHandler.validate_amount(amount)) {
            return false;
        }
        return accountManager.withdraw(account_id, amount);
    }

    float check_balance(int account_id) {
        if (!errorHandler.validate_account_id(account_id, accountManager)) {
            return -1.0f;
        }
        return accountManager.check_balance(account_id);
    }
};

// Multithreading & Synchronization Module
void run_transaction(SystemCallInterface& sysCallInterface, int account_id, float amount, bool is_deposit) {
    if (is_deposit) {
        sysCallInterface.deposit(account_id, amount);
    }
    else {
        sysCallInterface.withdraw(account_id, amount);
    }
}

int main() {
    Logger logger;
    AccountManager accountManager(logger);
    ErrorHandler errorHandler(logger);
    ProcessManager processManager;
    Scheduler scheduler(processManager, 100); // 100 milliseconds time slice
    MemoryManager memoryManager(5);
    IPCManager ipcManager;
    SystemCallInterface sysCallInterface(accountManager, errorHandler);

    thread scheduler_thread(&Scheduler::run, &scheduler);

    // Example usage
    int account_id1 = sysCallInterface.create_account(1, 1000.0f);
    int account_id2 = sysCallInterface.create_account(2, 2000.0f);
    cout << "Account ID 1: " << account_id1 << endl;
    cout << "Account ID 2: " << account_id2 << endl;

    thread t1(run_transaction, ref(sysCallInterface), account_id1, 500.0f, true);
    thread t2(run_transaction, ref(sysCallInterface), account_id1, 200.0f, false);
    thread t3(run_transaction, ref(sysCallInterface), account_id2, 300.0f, true);
    thread t4(run_transaction, ref(sysCallInterface), account_id2, 100.0f, false);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    cout << "Balance after transactions for Account ID 1: " << sysCallInterface.check_balance(account_id1) << endl;
    cout << "Balance after transactions for Account ID 2: " << sysCallInterface.check_balance(account_id2) << endl;

    int transaction_id1 = processManager.create_transaction_process(1, account_id1);
    int transaction_id2 = processManager.create_transaction_process(2, account_id2);
    scheduler.add_to_ready_queue(transaction_id1);
    scheduler.add_to_ready_queue(transaction_id2);

    memoryManager.store_data_in_page(account_id1, sysCallInterface.check_balance(account_id1));
    memoryManager.store_data_in_page(account_id2, sysCallInterface.check_balance(account_id2));
    memoryManager.display_memory_map();

    ipcManager.send_message("Transaction completed for Account ID 1");
    ipcManager.send_message("Transaction completed for Account ID 2");
    cout << "IPC Message 1: " << ipcManager.receive_message() << endl;
    cout << "IPC Message 2: " << ipcManager.receive_message() << endl;

    processManager.terminate_transaction_process(transaction_id1);
    processManager.terminate_transaction_process(transaction_id2);

    scheduler.stop();
    scheduler_thread.join();

    scheduler.display_gantt_chart();

    return 0;
}


