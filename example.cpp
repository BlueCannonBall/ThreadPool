#include "threadpool.hpp"
#include <iostream>
#include <mutex>

int main() {
    tp::ThreadPool pool; // Create a threadpool
    std::mutex mtx;
    std::vector<std::shared_ptr<tp::Task>> tasks; // Vector to store threadpool tasks

    for (unsigned int i = 0; i < 10; i++) {
        tasks.push_back(std::move(pool.schedule([i, &mtx](void*) {
            if (true) {
                mtx.lock();
                std::cout << "Printing from task: " << i << std::endl;
                mtx.unlock();
            }
        }))); // Schedule tasks and add them to the vector
    }

    for (auto task : tasks) { // Wait on every task
        task->await();
    }
    std::cout << "All tasks done!" << std::endl;

    return 0;
}