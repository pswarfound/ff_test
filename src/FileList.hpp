#pragma once
#include <sys/types.h>
#include <dirent.h>
#include <list>
#include <string>

using std::string;
using std::list;

typedef list<string> list_file_t;
typedef bool (*filter_t)(struct dirent *de);
class FileList
{
public:
    FileList();
    FileList(filter_t filter);
    ~FileList();

    int search(const string &name, int opt);
    bool add(const string &name);
    void show(void);
    void set_filter(filter_t filter);
    list_file_t m_lstFiles;
    filter_t    m_filter;
};
