#pragma once
#include <stdint.h>
#include "ff_headers.h"

class DecoderImpl;
class Decoder;

typedef int (*cb_read_pkt)(Decoder *, uint8_t *, int);
typedef int (*cb_recv_frame)(Decoder *decoder, AVFrame *frame);

enum {
    DEC_OK = 0,
    DEC_AGAIN,
    DEC_EOF,
    DEC_ERROR,
};

class Decoder
{
 public:

    Decoder();
    ~Decoder();


    int init();
    int release();
    int poll();
    int demux();
    int open();
    int decode();
    uint16_t get_width() const;
    uint16_t get_height() const;
  	int set_cb_read_pkt(cb_read_pkt fn, void *param);
  	void *get_read_pkt_param();
  	int set_cb_recv_frame(cb_recv_frame fn, void *param);
  	void *get_recv_frame_param();
  	uint32_t get_decoded_frame_cnt();
  	void time_usage_show();
  	static int __read_pkt(void *opaque, uint8_t *buf, int size);
  	void debug_show();
 private:
    DecoderImpl *m_impl;
 public:
    uint32_t m_mem_used;
    uint32_t memused_max;
    uint32_t memused_before_open;
    uint32_t memused_before_dec;
    uint32_t mem_demux;
    uint32_t mem_snd_pkt;
    uint32_t mem_frm;
    uint32_t decode_cnt;
};

int ff_init(void);
int ff_release();
#include <stdio.h>
#define MARK(...) printf("%s %d] ", __func__, __LINE__);printf(__VA_ARGS__);printf("\n");
extern __thread Decoder *tls_ctx;
