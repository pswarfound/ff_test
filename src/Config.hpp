#pragma once
#include <stdint.h>
#include <string>
#include <list>
#include <vector>

using std::vector;
using std::list;
using std::string;

class Config
{
private:
    Config();
    ~Config();
public:
    static Config &get_instance() {
        static Config instance;
        return instance;
    }

    bool parse(int argc, char **argv);
    void show(void);
    /*
     * configuration members
     */
    string  m_sFilePath;
    string  m_sDirPath;
    string  m_sOutputPath;
    vector<int>    m_vecCore;
    uint16_t m_skip_frame;
    uint16_t m_get_buffer;
    uint16_t m_help;
    int     m_seclive;
};
