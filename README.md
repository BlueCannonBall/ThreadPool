# ThreadPool
ThreadPool is a modern, header-only C++ threadpool library.

## Usage Example
```cpp
#include "threadpool.hpp"
#include <iostream>
#include <memory>
#include <mutex>

int main() {
    tp::ThreadPool threadpool; // Create a threadpool
    std::mutex mutex;
    std::vector<std::shared_ptr<tp::Task>> tasks; // Vector to store threadpool tasks

    for (unsigned int i = 0; i < 10; ++i) {
        tasks.push_back(threadpool.schedule([i, &mutex](void*) {
            mutex.lock();
            std::cout << "Printing from task: " << i << std::endl;
            mutex.unlock();
        })); // Schedule tasks and add them to the vector
    }

    for (const auto& task : tasks) { // Wait on every task
        task->await();
    }
    std::cout << "All tasks done!" << std::endl;

    return 0;
}
```
