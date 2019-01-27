#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <iostream>
#include "Config.hpp"

Config::Config()
    : m_skip_frame(0), m_get_buffer(0), m_help(0), m_seclive(0)
{
    m_vecCore.push_back(1);
}

Config::~Config()
{
    m_vecCore.clear();
}

bool Config::parse(int argc, char **argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "h::t:c:d:f:o:s:b:l:")) != -1) {
        switch (opt) {
        case 'h':
            m_help = 1;
            break;
        case 'o':
            m_sOutputPath = optarg;
            break;
        case 't':
            break;
        case 'c': {
            const char *c = optarg;
            m_vecCore.clear();
            while (*c) {
                m_vecCore.push_back(*c - '0');
                c++;
            }
            break;
            }
        case 'f':
            m_sFilePath = optarg;
            break;
        case 'd':
            m_sDirPath = optarg;
            break;
        case 's':
            m_skip_frame = atoi(optarg);
            break;
        case 'b':
            m_get_buffer = atoi(optarg);
            break;
        case 'l':
            m_seclive = atoi(optarg);
            break;
        case '?':
            break;
        }
    }
    return true;
}

void Config::show()
{
    std::cout << ""
            << "\nfile          : " << m_sFilePath
            << "\ndir           : " <<  m_sDirPath
            << "\noutput        : " <<  m_sOutputPath
            << "\nskip frame    : " <<  m_skip_frame
            << "\nget_buffer    : " <<  m_get_buffer
            << std::endl;
}
