#ifndef POOL_H_
#include <string>
#include <pthread.h>

class Task {
public:
    // Each task will need a completion condition variable to use in the wait function

    Task();
    virtual ~Task();

    virtual void Run() = 0;  // implemented by subclass
};

class ThreadPool {
public:

    // Add member variables for taskQueue mutex and task queue

    ThreadPool(int num_threads);

    // Submit a task with a particular name.
    void SubmitTask(const std::string &name, Task *task);
 
    // Wait for a task by name, if it hasn't been waited for yet. Only returns after the task is completed.
    void WaitForTask(const std::string &name);

    // Worker function for thread initialization
    void* WorkerFunction(void* args);

    // Stop all threads. All tasks must have been waited for before calling this.
    // You may assume that SubmitTask() is not caled after this is called.
    void Stop();
};
#endif
