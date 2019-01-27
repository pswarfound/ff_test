#include "TaskPool.hpp"
#include "Config.hpp"

TaskPool::TaskPool()
{
    sem_init(&m_sem, 0, 0);
}

TaskPool::~TaskPool()
{
    clear();
    task_list_t::const_iterator iter = m_lstTask.begin();
    while (iter != m_lstTask.end()) {
        Task *task = *iter;
        delete task;
        iter++;
    }
    m_lstTask.clear();
}

bool TaskPool::init(const list_file_t *flist)
{
    int i;
    Config &cfg = Config::get_instance();
    for (i = 0; i < cfg.m_vecCore.size(); i++) {
        Task *task = new Task;
        if (!task) {
            continue;
        }
        task->set_affinity(cfg.m_vecCore[i]);
        task->init(flist);
        m_lstTask.push_back(task);
    }
    return true;
}

bool TaskPool::start()
{
    task_list_t::const_iterator iter = m_lstTask.begin();
    while (iter != m_lstTask.end()) {
        (*iter)->start();
        iter++;
    }
    return true;
}

bool TaskPool::stop()
{
    task_list_t::const_iterator iter = m_lstTask.begin();
    while (iter != m_lstTask.end()) {
        (*iter)->stop();
        iter++;
    }
    return true;
}
bool TaskPool::is_stop()
{
    int val;
    Config &cfg = Config::get_instance();
    sem_getvalue(&m_sem, &val);
    return val == cfg.m_vecCore.size();
}

bool TaskPool::clear()
{
    task_list_t::const_iterator iter = m_lstTask.begin();
    while (iter != m_lstTask.end()) {
        (*iter)->stop();
        iter++;
    }
    iter = m_lstTask.begin();
    while (iter != m_lstTask.end()) {
        (*iter)->release();
        iter++;
    }
    return true;
}

bool TaskPool::notify()
{
    sem_post(&m_sem);
    return true;
}
