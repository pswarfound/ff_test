#pragma once
#include "ff_headers.h"
#include "Decoder.hpp"

class DecoderImpl
{
 public:
    DecoderImpl(Decoder *decoder);
    ~DecoderImpl();
 private:
    friend class Decoder;
    AVIOContext         *m_io_ctx;
    AVCodecContext      *m_codec_ctx;
    AVFormatContext     *m_fmt_ctx;
    AVPacket            *m_pkt;

    struct SwsContext   *m_sws_ctx;
    struct SwrContext   *m_swr_ctx;
    void                *m_opqua;
    int m_stream_idx;
    Decoder *m_decoder;

    cb_read_pkt			m_cb_read_pkt;
    void *				m_read_pkt_param;
    cb_recv_frame		m_cb_recv_frame;
    void *				m_recv_frame_param;

    uint32_t			m_decoded_frame_cnt;
    uint32_t			m_time_usage[10];
    uint16_t m_width, m_height;
    int open_codec_ctx();
    int stream_open();
    int decode();
    int demux();
    void flush();
 public:
    int size[4];
};

