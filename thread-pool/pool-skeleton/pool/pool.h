#ifndef POOL_H_
#include <string>
#include <pthread.h>
#include <map>
#include <queue>

class Task {
public:
    
    // Each task will need a completion condition variable to use in the wait function
    Task();
    virtual ~Task();
    virtual void Run() = 0;  // implemented by subclass
    
    pthread_cond_t* getDoneCondition() {
        return &doneCondition;
    }

    bool getFinished() {
        return finished;
    }

    void setFinishedTrue() {
        finished = true;
    }

private:
    pthread_cond_t doneCondition;
    bool finished;

};

class ThreadPool {
public:
    // Boolean private member indicating that stop has been called
    // Add member variables for taskQueue mutex and task queue

    ThreadPool(int num_threads);

    // Submit a task with a particular name.
    void SubmitTask(const std::string &name, Task *task);
 
    // Wait for a task by name, if it hasn't been waited for yet. Only returns after the task is completed.
    void WaitForTask(const std::string &name);

    // Stop all threads. All tasks must have been waited for before calling this.
    // You may assume that SubmitTask() is not caled after this is called.
    void Stop();

    bool stopCalled;
    pthread_mutex_t taskMutex;
    pthread_cond_t taskReadyCondition;
    std::map<std::string, Task*> taskMap;
    std::queue<std::string> taskQueue;
        pthread_t *threads;
    int num_threads;

};
#endif
