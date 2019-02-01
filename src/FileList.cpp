#include <sys/types.h>
#include <dirent.h>
#include <iostream>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include "FileList.hpp"

static bool filter_default(const string &cwd, const struct dirent *de)
{
    return false;
}

FileList::FileList()
    : m_filter(filter_default)
{

}

FileList::FileList(filter_t filter)
    : m_filter(filter)
{

}

FileList::~FileList()
{
    m_lstFiles.clear();
}

int FileList::search(const string &name, int opt)
{
    int nFile = 0;
    DIR *d = NULL;

    d = opendir(name.c_str());
    if (!d) {
        std::cout << __func__ << strerror(errno) << std::endl;
        return -1;
    }

    struct dirent *de;
    while ((de = readdir(d))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
            continue;
        }
        string path;
        if (m_filter(name, de)) {
            continue;
        }
        m_lstFiles.push_back(name + de->d_name);
    }

    closedir(d);
    return nFile;
}

bool FileList::add(const string &name)
{
    m_lstFiles.push_back(name);
    return true;
}

void FileList::show(void)
{
    if (m_lstFiles.empty()) {
        return;
    }

    list_file_t::iterator iter = m_lstFiles.begin();
    while (iter != m_lstFiles.end()) {
        std::cout << *iter << std::endl;
        iter++;
    }
}
