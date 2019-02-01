#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include "Task.hpp"
#include "Utils.hpp"
#include "Decoder.hpp"
#include "ff_headers.h"
#include "Config.hpp"
#include "File.hpp"
#include "TaskPool.hpp"

Task::Task()
    : m_tid(0), m_bExit(false), m_core_id(1), m_fdin(NULL), m_fdout(NULL)
{
    sem_init(&m_sem, 0, 0);
}

Task::~Task()
{
    release();
    sem_destroy(&m_sem);
}

bool Task::init(const list_file_t *flist)
{
    m_flist = flist;
    int ret = pthread_create(&m_tid, NULL, svc, this);
    if (ret < 0) {
        m_tid = 0;
        return false;
    }
    util_cpu_bind(m_tid, 1 << m_core_id);
    return true;
}

bool Task::start()
{
    if (m_tid != 0) {
        sem_post(&m_sem);
        return true;
    }
    return false;
}

bool Task::stop()
{
    m_bExit = true;
    return true;
}

bool Task::release()
{
    m_bExit = true;
    if (m_tid != 0) {
        pthread_join(m_tid, NULL);
        m_tid = 0;
    }
    return true;
}

bool Task::set_affinity(uint8_t core_id)
{
    m_core_id = core_id;
    if (m_tid != 0) {
        if (util_cpu_bind(m_tid, 1 << m_core_id) != 0) {
            return false;
        }
    }
    return true;
}



int read_pkt_file(Decoder *decoder, uint8_t *buf, int size)
{
    Task *task = (Task *)decoder->get_read_pkt_param();
    MFile *f = task->m_fdin;
    if (!f || !f->is_open()) {
        return -1;
    }
    int r = f->read(buf, size);

    return r;
}

int on_recv_frame(Decoder *dc, AVFrame *frame)
{
    Task *task = (Task *)dc->get_recv_frame_param();
    MFile *f = task->m_fdout;
    if (!f || !f->is_open()) {
        return 0;
    }
    if (frame->width == frame->linesize[0])
    {
       f->write(frame->data[0], frame->width * frame->height);
    }
    else
    {
        uint16_t i;
        for (i = 0; i < frame->height; i++)
        {
            f->write(frame->data[0] + i * frame->linesize[0], frame->width);
        }
    }
    return 0;
}

static void get_file_name(const string &path, string &fname)
{
    const char *p = strrchr(path.c_str(), '/');
    if (p) {
        fname = p + 1;
    } else {
        fname = path;
    }
}

int output_file_create(uint8_t core, MFile *f, const string &path, const Decoder &dc)
{
    Config &cfg = Config::get_instance();
    char fullpath[256];
    int w = dc.get_width();
    int h = dc.get_height();
    string fname;
    get_file_name(path, fname);
    if (fname.empty() || cfg.m_sOutputPath.empty()) {
        return -1;
    }
    snprintf(fullpath, sizeof(fullpath), "%s/%d_%s_%dx%d.yuv", cfg.m_sOutputPath.c_str(), core,
            fname.c_str(), dc.get_width(), dc.get_height());
    MARK("%s", fullpath);
    return f->create(fullpath);
}

#if 0
static void decode_routine()
{
	Decoder dc;
	MFile fout;

	tls_ctx = &dc;

    if (dc.init() < 0) {
        MARK("");
        return;
    }
	dc.set_cb_read_pkt(read_pkt_file, p);
	dc.set_cb_recv_frame(on_recv_frame, p);
	int ret;
     gettimeofday(&t1, NULL);
     do {
         ret = dc.open();
         if (ret != 0) {
             MARK("");
             break;
         }
         if (!cfg.m_sOutputPath.empty()) {
             if (output_file_create(obj->m_core_id, &fout, *iter, dc) == 0) {
                 obj->m_fdout = &fout;
             }
         }
         while (!obj->m_bExit) {
             ret = dc.demux();
             if (ret == DEC_OK || ret == DEC_EOF) {
                 ret = dc.decode();
             } else if (ret == DEC_AGAIN) {
                 // eagain
                 continue;
             } else {
                 break;
             }
             if (ret == DEC_EOF) {
                 // eof
                 break;
             } else if (ret == DEC_ERROR) {
                 //MARK("err occured");
                 //break;
             }
         }
     } while(0);

     gettimeofday(&t2, NULL);
     MARK("[%d]total frame %d, %d, %d ms, data: %d MB rate:%d Mbps mem_max:%d w:%d h:%d mem init:%d mem_open:%d dec:%d max:%d",
     		obj->m_core_id,
             dc.get_decoded_frame_cnt(),
             dc.decode_cnt,
             util_diff_time_us(t1, t2) / 1000,
				fin.m_size >> 20,
     ((uint64_t)fin.m_size * 8 *1000000 / (util_diff_time_us(t1, t2) + 0)) >> 20, 0,
             dc.get_width(), dc.get_height(),
             dc.memused_before_open >> 10,
             (dc.memused_before_dec - dc.memused_before_open) >> 10,
             (dc.memused_max - dc.memused_before_dec) >> 10,
             dc.memused_max >> 10
             );

     dc.release();
}
#endif

void *Task::svc(void *p)
{
    Task *obj = (Task*)p;
    const list_file_t *flist = obj->m_flist;
    Config &cfg = Config::get_instance();
    struct timeval t1, t2;

    sem_wait(&obj->m_sem);
    while (!obj->m_bExit) {
        list_file_t::const_iterator iter = flist->begin();


        if (cfg.m_seclive == 0) {
            MARK("task %d loop done, exit ", obj->m_core_id);
            break;
        }
    }

    TaskPool &pool = TaskPool::get_instance();
    pool.notify();
    return NULL;
}
