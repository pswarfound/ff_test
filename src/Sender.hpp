#pragma once
#include <pthread.h>
#include "FileList.hpp"

class Sender
{
public:
    Sender();
    ~Sender();

    int init();
    void start();
    void pause();
    void stop();
    void release();
private:
    static void *task_sender(void *p);
    pthread_t m_tid;
    bool m_bStop;
    FileList *m_lstFiles;
};
