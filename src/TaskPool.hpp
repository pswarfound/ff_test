#pragma once

#include <stdint.h>
#include "Task.hpp"
#include <list>
#include <semaphore.h>

using std::list;

typedef list<Task*> task_list_t;

class TaskPool
{
private:
    TaskPool();
    ~TaskPool();
private:
    friend class Task;
    bool notify();
public:
    static TaskPool&get_instance() {
        static TaskPool instance;
        return instance;
    }
    bool init(const list_file_t *flist);
    bool start();
    bool stop();
    bool is_stop();
    bool clear();
    sem_t m_sem;
    task_list_t m_lstTask;
};
