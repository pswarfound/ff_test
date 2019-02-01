#include "Sender.hpp"


Sender::Sender()
    : m_tid(0), m_bStop(false)
{

}

Sender::~Sender()
{
    stop();
    release();
}

int Sender::init()
{
    int ret = pthread_create(&m_tid, NULL, task_sender, this);
    if (ret < 0) {
        m_tid = 0;
        return -1;
    }
    return 0;
}

void Sender::start()
{

}

void Sender::stop()
{
    m_bStop = true;
}

void Sender::release()
{
    if (m_tid != 0) {
        pthread_join(m_tid, NULL);
        m_tid = 0;
    }
}

void *Sender::task_sender(void *p)
{
    Sender *obj = (Sender*)p;
    while (!obj->m_bStop) {

    }
    return NULL;
}
