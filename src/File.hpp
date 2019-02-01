#pragma once
#include <stdint.h>
#include <string>

using std::string;

class MFile
{
public:
    MFile(const string &name);
    MFile();
    ~MFile();

    bool open();
    int create(const string &name);
    int write(const void *src, uint32_t count);
    int read(void *dst, uint32_t count);
    int close();
    bool is_open() {
        return !(m_fd < 0);
    }

private:
    int m_fd;
public:
    string m_name;
    uint32_t m_size;
};
