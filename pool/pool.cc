#include "pool.h"
#include <iostream>
#include <unistd.h>

void* WorkerFunction(void* args);
struct argstruct {
    ThreadPool *pool;
    int thread_id;
};

Task::Task() {
    // Initialize condition member variable
    pthread_cond_init(&doneCondition, NULL);
    finished = false;
}

Task::~Task() {
    //cleanup condition
    pthread_cond_destroy(&doneCondition);
}

ThreadPool::ThreadPool(int num_threads) {
    // Initialize the queue/map mutex and task ready condition
    // Create a number of threads equal to num_threads
    // Initialize threads with the worker function described below
    
    stopCalled = false;

    pthread_mutex_init(&taskMutex, NULL);
    pthread_cond_init(&taskReadyCondition, NULL);
    
    this->threads = new pthread_t[num_threads];
    this->num_threads = num_threads;

    for (int i = 0; i < num_threads; i++) {
        std::cout << "creating thread " << i << std::endl;
        if (pthread_create(&threads[i], NULL, WorkerFunction, this) != 0) {
            perror("Failed to create thread");
        }
    }
}

void ThreadPool::SubmitTask(const std::string &name, Task* task) {   
    // When a task is submitted, add it to the task queue
    // Make sure the queue mutex is used to protect the task queue
    // Also add the task into a map with key name and value task
    // After unlocking the queue mutex, signal that a task is available

    pthread_mutex_lock(&taskMutex);
    taskQueue.push(name);
    taskMap.insert({name, task});
    pthread_mutex_unlock(&taskMutex);

    pthread_cond_signal(&taskReadyCondition);

}

void* WorkerFunction(void* args) {   
    // Run in a superloop because threads should work until stop is called
    // Add a conditional wait within superloop, checking if task is available
    // Ensure mutual exclusion on task queue. Make sure lock is released before calling run
    // Dequeue a task and call that task's run function

    ThreadPool* pool = (ThreadPool *) args;

    while (true) {
        pthread_mutex_lock(&pool->taskMutex);
        while(pool->taskQueue.empty() && !pool->stopCalled) {
            pthread_cond_wait(&pool->taskReadyCondition, &pool->taskMutex);
        }
        if (pool->stopCalled) {
            pthread_mutex_unlock(&pool->taskMutex);
            return nullptr;
        }

        std::string taskKey = pool->taskQueue.front();
        pool->taskQueue.pop();
        Task* myTask = pool->taskMap[taskKey];

        pthread_mutex_unlock(&pool->taskMutex);

        myTask->Run();
        myTask->setFinishedTrue();

        pthread_cond_signal(myTask->getDoneCondition());
    }
    
}

void ThreadPool::WaitForTask(const std::string &name) {
    // Find the task using the key name to search in map
    // waits on the per task condition variable

    pthread_mutex_lock(&taskMutex);
    if (taskMap.count(name)) {
        Task* myTask = taskMap[name];
        while (!myTask->getFinished()) {
            pthread_cond_wait(myTask->getDoneCondition(), &taskMutex);
        }
        taskMap.erase(name);
        delete myTask;
    }
    pthread_mutex_unlock(&taskMutex);
}

void ThreadPool::Stop() {
    // cleanup threads

    pthread_mutex_lock(&taskMutex);
    stopCalled = true;
    pthread_cond_broadcast(&taskReadyCondition);
    pthread_mutex_unlock(&taskMutex);

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Problem joining thread");
        }
    }
    
    pthread_mutex_destroy(&taskMutex);
    pthread_cond_destroy(&taskReadyCondition);

    delete[] threads;
}
