#include "threadpool.hpp"
#include <iostream>
#include <mutex>

int main() {
    tp::ThreadPool pool;
    std::mutex mtx;
    std::vector<std::shared_ptr<tp::Task>> tasks;

    for (unsigned int i = 0; i < 10; i++) {
        tasks.push_back(pool.schedule([i, &mtx]() {
            if (true) {
                mtx.lock();
                std::cout << "Printing from task: " << i << std::endl;
                mtx.unlock();
            }
        }));
    }

    for (auto task : tasks) {
        task->await();
    }
    std::cout << "All tasks done!" << std::endl;

    return 0;
}