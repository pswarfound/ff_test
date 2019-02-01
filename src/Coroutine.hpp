#pragma once
#include <stdio.h>

typedef void (*routine_t)(void *p);
template <typename T>
class CoroutineFactory;

class Coroutine
{
public:
    Coroutine(void *main_hwnd, void *sstk);
    ~Coroutine();

    bool create(routine_t routine, void *arg, size_t stack_size);
    void resume();
    static void yield();
    bool is_stop();

    static void routine_proxy(void);
    void *m_hwnd = NULL;
    void *m_fp = NULL;
    void *m_arg = NULL;
    void *m_sstk = NULL;
    void *m_main_hwnd;
    bool m_is_stop = false;
};

template <typename T>
class CoroutineFactory
{
public:
    CoroutineFactory(size_t shared_stack_size = 0);
    ~CoroutineFactory();

public:
    bool cr_get(const T &key, Coroutine **cr);
    bool cr_create(routine_t routine, void *arg, size_t stack_size, T &key);
    bool cr_yield(const T &key);
    bool cr_resume(const T &key);
    bool cr_is_quit(const T &key);
    bool cr_remove(const T &key);
    bool empty(void);
    size_t size(void);
private:
    void *m_hwnd = NULL;
    void *m_sstk = NULL;
    void *m_map = NULL;
};
