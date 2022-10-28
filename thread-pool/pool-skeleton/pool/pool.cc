#include "pool.h"

Task::Task() {
    // Initialize condition member variable
    // Other metadata?
}

Task::~Task() {
    //cleanup condition
}

ThreadPool::ThreadPool(int num_threads) {
    // Creates and joins a number of threads equal to num_threads
    // Initialize threads with the worker function described below
}

void ThreadPool::SubmitTask(const std::string &name, Task* task) {
    
    // When a task is submitted, add it to the task queue
    // Make sure the queue mutex is used to protect the task queue
    // Also add the task into a map with key name and value task
    // After unlocking the queue mutex, signal that a task is available
}

void* WorkerFunction(void* args) {
    
    // Run in a superloop because threads should work until stop is called
    // Add a conditional wait within superloop, checking if task is available
    // Ensure mutual exclusion on task queue. Make sure lock is released before calling run
    // Dequeue a task and call that task's run function
    
}

void ThreadPool::WaitForTask(const std::string &name) {
    // Find the task using the key name to search in map
    // waits on the per task condition variable
}

void ThreadPool::Stop() {
    // cleanup threads
}

// TODO: cleanup queue and mutex