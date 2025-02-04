#include <atomic>
#include <thread>
#include <vector>
#include <iostream>
#include <limits>
#include <cassert>

// Lamport's Bakery Lock Implementation
class WaitFreeLock {
private:
    int MAX_THREADS;
    std::vector<std::atomic<bool>> choosing;
    std::vector<std::atomic<unsigned>> number;

public:
    // Constructor with dynamic thread count
    WaitFreeLock(const int max_th = 10) : 
        MAX_THREADS(max_th),
            choosing(max_th),
                number(max_th)
    {
        MAX_THREADS = max_th;
        for(int i = 0; i < MAX_THREADS; ++i)
        {
            choosing[i].store(false, std::memory_order_relaxed);
            number[i].store(0, std::memory_order_relaxed);
        }
    }

    // Lock function for a given thread ID
    void lock(int thread_id)
    {
        choosing[thread_id].store(true, std::memory_order_relaxed);

        // Find the maximum ticket number
        unsigned max = 0;
        for(int i = 0; i < MAX_THREADS; ++i){
            unsigned num = number[i].load(std::memory_order_relaxed);
            if(num > max) max = num;
        }

        // Assign a ticket number to the current thread
        number[thread_id].store(max + 1, std::memory_order_relaxed);
        choosing[thread_id].store(false, std::memory_order_release);

        // Wait until it's this thread's turn
        for(int j = 0; j < MAX_THREADS; ++j){
            if(j == thread_id) continue;

            // Wait if thread j is choosing its number
            while(choosing[j].load(std::memory_order_acquire));

            // Wait if thread j has a lower ticket number or the same number but a lower ID
            while (number[j].load(std::memory_order_acquire) != 0 &&
                   (number[j].load(std::memory_order_acquire) < number[thread_id].load(std::memory_order_acquire) ||
                   (number[j].load(std::memory_order_acquire) == number[thread_id].load(std::memory_order_acquire) && j < thread_id)));
        }
    }

    // Unlock function for a given thread ID
    void unlock(int thread_id)
    {
        number[thread_id].store(0, std::memory_order_release);
    }
};

// Example Usage of WaitFreeLock
int main() {
    WaitFreeLock lock;
    int counter = 0;
    const int num_threads = MAX_THREADS;
    const int increments_per_thread = 100;
    std::vector<std::thread> threads;

    // Launch multiple threads
    for(int i = 0; i < num_threads; ++i){
        threads.emplace_back([&lock, &counter, increments_per_thread, i](){
            for(int j = 0; j < increments_per_thread; ++j){
                lock.lock(i);      // Acquire the lock
                // Critical Section
                ++counter;
                lock.unlock(i);    // Release the lock
            }
        });
    }

    // Wait for all threads to finish
    for(auto& t : threads){
        t.join();
    }

    // Verify the result
    std::cout << "Counter value: " << counter 
              << " (expected " << num_threads * increments_per_thread << ")\n";
    assert(counter == num_threads * increments_per_thread);
}
