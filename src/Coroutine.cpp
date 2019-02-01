#include <map>
#include "Coroutine.hpp"
#include "libaco/aco.h"

using std::pair;
using std::map;
#define MARK(...) printf("%s %d]", __func__, __LINE__);printf(__VA_ARGS__);printf("\n");
Coroutine::Coroutine(void *main_hwnd, void *sstk)
    : m_main_hwnd(main_hwnd), m_sstk(sstk)
{

}

Coroutine::~Coroutine()
{
    if (m_hwnd) {
        aco_destroy((aco_t *)m_hwnd);
        m_hwnd = NULL;
    }
}

void Coroutine::routine_proxy(void)
{
    Coroutine *cr = (Coroutine *)aco_get_arg();
    routine_t fp = (routine_t)cr->m_fp;
    fp(cr->m_arg);
    cr->m_is_stop = true;
    aco_exit();
}
/*
aco_t* aco_create(
        aco_t* main_co,
        aco_share_stack_t* share_stack,
        size_t save_stack_sz,
        aco_cofuncp_t fp, void* arg
    );
*/
bool Coroutine::create(routine_t routine, void *arg, size_t stack_size)
{
    if (m_hwnd) {
        return false;
    }
    m_fp = (void*)routine;
    m_arg = arg;
    m_hwnd = aco_create((aco_t*)m_main_hwnd, (aco_share_stack_t*)m_sstk, stack_size, routine_proxy, (void*)this);
    return m_hwnd == NULL?false:true;
}

void Coroutine::resume()
{
    aco_resume((aco_t*)m_hwnd);
}

void Coroutine::yield()
{
    aco_yield();
}

bool Coroutine::is_stop()
{
    return m_is_stop;
}

template <typename T>
CoroutineFactory<T>::CoroutineFactory(size_t shared_stack_size)
{
    aco_thread_init(NULL);
    m_hwnd = aco_create(NULL, NULL, 0, NULL, NULL);
    m_sstk = aco_share_stack_new(shared_stack_size);
    m_map = new std::map<T, Coroutine*>;
}

template <typename T>
CoroutineFactory<T>::~CoroutineFactory()
{
    map<T, Coroutine*> *cr_map = (map<T, Coroutine*>*)m_map;
    typename map<T, Coroutine*>::iterator iter = cr_map->begin();
    while (iter != cr_map->end()) {
        delete iter->second;
        iter++;
    }
    aco_share_stack_destroy((aco_share_stack_t*)m_sstk);
    aco_destroy((aco_t*)m_hwnd);
    cr_map->clear();
    delete cr_map;
}

template <typename T>
bool CoroutineFactory<T>::cr_create(routine_t routine, void *arg, size_t stack_size, T &key)
{
    map<T, Coroutine*> *cr_map = (map<T, Coroutine*>*)m_map;
    if (cr_map->find(key) != cr_map->end()) {
        return false;
    }
    Coroutine *cr = new Coroutine(m_hwnd, m_sstk);
    cr->create(routine, arg, stack_size);
    bool ret = cr_map->insert(pair<T, Coroutine*>(key, cr)).second;
    if (!ret) {
        delete cr;
        return false;
    }
    return true;
}

template <typename T>
bool CoroutineFactory<T>::cr_yield(const T&key)
{
    map<T, Coroutine*> *cr_map = (map<T, Coroutine*>*)m_map;
    typename map<T, Coroutine*>::iterator iter = cr_map->find(key);
    if (iter == cr_map->end()) {
        return false;
    }
    iter->second->yield();
    return true;
}

template <typename T>
bool CoroutineFactory<T>::cr_resume(const T&key)
{
    map<T, Coroutine*> *cr_map = (map<T, Coroutine*>*)m_map;
    typename map<T, Coroutine*>::iterator iter = cr_map->find(key);
    if (iter == cr_map->end()) {
        return false;
    }
    iter->second->resume();
    return true;
}

template <typename T>
bool CoroutineFactory<T>::cr_is_quit(const T &key)
{
    map<T, Coroutine*> *cr_map = (map<T, Coroutine*>*)m_map;
    typename map<T, Coroutine*>::iterator iter = cr_map->find(key);
    if (iter == cr_map->end()) {
        return false;
    }
    return iter->second->is_stop();
}

template <typename T>
bool CoroutineFactory<T>::cr_get(const T &key, Coroutine **cr)
{
    map<T, Coroutine*> *cr_map = (map<T, Coroutine*>*)m_map;
    typename map<T, Coroutine*>::iterator iter = cr_map->find(key);
    if (iter == cr_map->end()) {
        return false;
    }
    *cr = iter->second;
    return true;
}

template <typename T>
bool CoroutineFactory<T>::cr_remove(const T &key)
{
    map<T, Coroutine*> *cr_map = (map<T, Coroutine*>*)m_map;
    typename map<T, Coroutine*>::iterator iter = cr_map->find(key);
    if (iter == cr_map->end()) {
        return false;
    }
    delete iter->second;
    cr_map->erase(key);
    return true;
}

template <typename T>
bool CoroutineFactory<T>::empty(void)
{
    map<T, Coroutine*> *cr_map = (map<T, Coroutine*>*)m_map;
    return cr_map->empty();
}

template <typename T>
size_t CoroutineFactory<T>::size(void)
{
    map<T, Coroutine*> *cr_map = (map<T, Coroutine*>*)m_map;
    return cr_map->size();
}

template class CoroutineFactory<int>;
