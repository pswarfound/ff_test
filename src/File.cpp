#include <unistd.h>
#include <fcntl.h>
#include "File.hpp"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#define MARK(...) printf("%s %d] ", __func__, __LINE__);printf(__VA_ARGS__);printf("\n");

MFile::MFile()
    : m_fd(-1), m_size(0)
{

}
MFile::MFile(const string &name)
    : m_name(name)
{

}

MFile::~MFile()
{
    close();
}

bool MFile::open()
{
    int sz = 0;
    int fd = ::open(m_name.c_str(), O_RDONLY);
    if (fd < 0) {
        MARK("%s %s", m_name.c_str(), strerror(errno));
        return false;
    }
    sz = lseek(fd, 0, SEEK_END);
    if (sz == -1) {
        MARK("%s %s", m_name.c_str(), strerror(errno));
        return false;
    }
    lseek(fd, 0, SEEK_SET);
    m_fd = fd;
    m_size = sz;
    return true;
}
int MFile::read(void *dst, uint32_t count)
{
    int ret;
    ret = ::read(m_fd, dst, count);
    return ret;
}

int MFile::create(const string &name)
{
    int fd = ::open(name.c_str(), O_RDWR|O_CREAT|O_TRUNC, 0666);
    if (fd < 0) {
        return -1;
    }
    m_fd = fd;
    return 0;
}

int MFile::write(const void *src, uint32_t count)
{
    int ret;
    ret = ::write(m_fd, src, count);
    return ret;
}

int MFile::close()
{
    if (m_fd > 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    return 0;
}
