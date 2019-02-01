#pragma once
#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>
#include "FileList.hpp"
#include "File.hpp"

class Task
{
public:
    Task();
    ~Task();

    bool set_affinity(uint8_t core_id);
    bool init(const list_file_t *flist);
    bool start();
    bool stop();
    bool release();
private:
    static void *svc(void *p);
    sem_t m_sem;
    pthread_t m_tid;
    bool m_bExit;
    uint8_t m_core_id;
    const list_file_t *m_flist;
public:
    MFile *m_fdin;
    MFile *m_fdout;
};
