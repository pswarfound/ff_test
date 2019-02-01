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


static bool ff_filter(const string &cwd, const struct dirent *de)
{
    string fullpath = cwd + de->d_name;

    struct stat st;

    if (stat(fullpath.c_str(), &st) < 0) {
        return true;
    }

    switch (st.st_mode & S_IFMT) {
    case S_IFREG:
        return false;
    default:
        return true;
    }
}

static int intput_collect(FileList &lst)
{
	Config &cfg = Config::get_instance();

	lst.set_filter(ff_filter);
	if (!cfg.m_sDirPath.empty()) {
	    lst.search(cfg.m_sDirPath, 0);
    }
    if (!cfg.m_sFilePath.empty()) {
        lst.add(cfg.m_sFilePath);
    }
    if (lst.m_lstFiles.empty()) {
        return -1;
    }

    return 0;
}

static void wait_job_done()
{
	Config &cfg = Config::get_instance();
    TaskPool &task_pool = TaskPool::get_instance();

    int seclive = cfg.m_seclive;
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
	Config &cfg = Config::get_instance();
	FileList input_file_list;

    if (!cfg.parse(argc, argv)) {
        return -1;
    }

    if (cfg.m_help) {
    	cfg.show();
    }

    // init ffmpeg
    ff_init();
    // collect input files to decode
    if (intput_collect(input_file_list) < 0) {
        printf("no input file\n");
        return -1;
    }

    TaskPool &task_pool = TaskPool::get_instance();
    task_pool.init(&input_file_list.m_lstFiles);
    task_pool.start();

    wait_job_done();

    task_pool.clear();
    ff_release();
    return 0;
}

