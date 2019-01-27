#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include "ff_headers.h"
#include "Decoder.hpp"
#include <vector>
#include <pthread.h>
#include "Config.hpp"
#include "Utils.hpp"
#include "TaskPool.hpp"
#include "FileList.hpp"

FileList g_list_file;
Config &g_cfg = Config::get_instance();

static int intput_collect()
{
    if (!g_cfg.m_sDirPath.empty()) {
        g_list_file.search(g_cfg.m_sDirPath, 0);
    }
    if (!g_cfg.m_sFilePath.empty()) {
        g_list_file.add(g_cfg.m_sFilePath);
    }
    if (g_list_file.m_lstFiles.empty()) {
        return -1;
    }
    return 0;
}

static void wait_job_done()
{
    TaskPool &task_pool = TaskPool::get_instance();
    int seclive = g_cfg.m_seclive;
    if (seclive < 0) {
        while (1) {
            pause();
        }
    }else if (seclive == 0) {
        MARK("running in one loop mode");
        while (!task_pool.is_stop()) {
            sleep(1);
        }
    } else {
        while (seclive) {
            sleep(1);
            seclive--;
        }
    }
}

int main(int argc, char **argv)
{
    if (!g_cfg.parse(argc, argv)) {
        return -1;
    }

    if (g_cfg.m_help) {
        g_cfg.show();
    }

    // init ffmpeg
    ff_init();
    // collect input files to decode
    if (intput_collect() < 0) {
        printf("no input file\n");
        return -1;
    }

    TaskPool &task_pool = TaskPool::get_instance();
    task_pool.init(&g_list_file.m_lstFiles);
    task_pool.start();

    wait_job_done();

    task_pool.clear();
    ff_release();
    return 0;
}

