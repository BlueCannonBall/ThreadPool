#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace tp {
    enum class CommandType {
        Quit,
        Execute
    };

    enum class CommandStatus {
        Running,
        Success,
        Failure
    };

    class Command {
    private:
        friend class ThreadPool;
        std::mutex mutex;
        std::condition_variable condition;

    public:
        CommandType type;
        CommandStatus status = CommandStatus::Running;
        void* data; // User data

        Command() = default;

        Command(CommandType type) :
            type(type) { }

        CommandStatus await() {
            std::unique_lock<std::mutex> lock(mutex);
            if (status != CommandStatus::Running) {
                return status;
            }
            while (status == CommandStatus::Running) {
                condition.wait(lock);
            }
            return status;
        }
    };

    class CommandExecute: public Command {
    public:
        std::function<void()> func;
        std::exception error;

        CommandExecute(std::function<void()> func) :
            func(std::move(func)) {
            type = CommandType::Execute;
        }
    };

    using Task = CommandExecute;

    class ThreadPool {
    protected:
        struct CommandQueue {
            std::queue<std::shared_ptr<Command>> queue;
            std::mutex mutex;
            std::condition_variable condition;
        };

        void runner(CommandQueue* commands) {
            for (;;) {
                std::unique_lock<std::mutex> lock(commands->mutex);
                while (commands->queue.empty()) {
                    commands->condition.wait(lock);
                }
                std::shared_ptr<Command> command = commands->queue.front();
                commands->queue.pop();
                lock.unlock();

                switch (command->type) {
                    default: {
                        throw "Invalid command type";
                        break;
                    }

                    case CommandType::Execute: {
                        auto cmd = (CommandExecute*) command.get();
                        try {
                            cmd->func();

                            std::unique_lock<std::mutex> lock(cmd->mutex);
                            cmd->status = CommandStatus::Success;
                        } catch (std::exception& e) {
                            std::unique_lock<std::mutex> lock(cmd->mutex);
                            cmd->status = CommandStatus::Failure;
                            cmd->error = e;
                        }

                        cmd->condition.notify_all();
                        break;
                    }

                    case CommandType::Quit: {
                        return;
                    }
                }
            }
        }

        std::vector<std::pair<std::thread, CommandQueue*>> threads;
        unsigned int sched_counter = 0;

    public:
        ThreadPool(unsigned int pool_size = std::thread::hardware_concurrency()) {
            for (unsigned int i = 0; i < pool_size; i++) {
                auto new_queue = new CommandQueue;
                std::thread new_thread(&ThreadPool::runner, this, new_queue);
                threads.push_back({std::move(new_thread), new_queue});
            }
        };

        ~ThreadPool() {
            for (auto& thread : threads) {
                auto cmd = std::make_shared<Command>(CommandType::Quit);

                {
                    std::unique_lock<std::mutex> lock(thread.second->mutex);
                    thread.second->queue.push(cmd);
                }
                thread.second->condition.notify_one();
            }

            for (auto& thread : threads) {
                thread.first.join();
                delete thread.second;
            }
        };

        std::shared_ptr<Task> schedule(std::function<void()> func) {
            CommandQueue* commands = threads[sched_counter].second;
            auto cmd = std::make_shared<CommandExecute>(std::move(func));

            {
                std::unique_lock<std::mutex> lock(commands->mutex);
                commands->queue.push(cmd);
            }

            commands->condition.notify_one();
            sched_counter = (sched_counter + 1) % threads.size();

            return cmd;
        };

        void resize(unsigned int new_pool_size) {
            if (new_pool_size == threads.size()) {
                return;
            }

            if (new_pool_size < threads.size()) {
                for (unsigned int i = 0; i < new_pool_size; i++, sched_counter = (sched_counter + 1) % threads.size()) {
                    auto cmd = std::make_shared<Command>(CommandType::Quit);

                    {
                        std::unique_lock<std::mutex> lock(threads[sched_counter].second->mutex);
                        threads[sched_counter].second->queue.push(cmd);
                    }
                    threads[sched_counter].second->condition.notify_one();

                    threads[sched_counter].first.join();
                    delete threads[sched_counter].second;
                }

                threads.resize(new_pool_size);
            } else {
                for (unsigned int i = threads.size(); i < new_pool_size; i++) {
                    auto new_queue = new CommandQueue;
                    std::thread new_thread(&ThreadPool::runner, this, new_queue);
                    threads.push_back({std::move(new_thread), new_queue});
                }
            }
        }

        inline decltype(threads)::size_type size() const {
            return threads.size();
        }
    };
} // namespace tp