#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "Decoder.hpp"
#include "DecoderImpl.hpp"
#include "ff_headers.h"
#include <sys/time.h>
#include <malloc.h>
#include <pthread.h>
#include "Utils.hpp"
#include "Config.hpp"

__thread Decoder *tls_ctx = NULL;

Decoder *tls_get_ctx()
{
    return tls_ctx;
}

void cb_log(void *s, int a, const char *m, va_list va)
{
//    printf(m, va);
}

int ff_init(void)
{
	av_version_info();
    av_register_all();
    av_log_set_level(AV_LOG_FATAL);
    av_log_set_callback(cb_log);
}

int ff_release()
{
    av_lockmgr_register(NULL);
}

static void lavc_ReleaseFrame(void *opaque, uint8_t *data)
{
    (void) data;
    free(opaque);
}

static int update_frame_pool(AVCodecContext *avctx, AVFrame *frame)
{
    FramePool *pool = avctx->internal->pool;
    int i, ret;
    DecoderImpl *dc = (DecoderImpl*)avctx->opaque;
    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_VIDEO: {
        uint8_t *data[4];
        int linesize[4];
        int size[4] = { 0 };
        int w = frame->width;
        int h = frame->height;
        int tmpsize, unaligned;

        if (pool->format == frame->format &&
            pool->width == frame->width && pool->height == frame->height)
            return 0;

        avcodec_align_dimensions2(avctx, &w, &h, pool->stride_align);

        do {
            // NOTE: do not align linesizes individually, this breaks e.g. assumptions
            // that linesize[0] == 2*linesize[1] in the MPEG-encoder for 4:2:2
            ret = av_image_fill_linesizes(linesize, avctx->pix_fmt, w);
            if (ret < 0)
                return ret;
            // increase alignment of w for next try (rhs gives the lowest bit set in w)
            w += w & ~(w - 1);

            unaligned = 0;
            for (i = 0; i < 4; i++)
                unaligned |= linesize[i] % pool->stride_align[i];
        } while (unaligned);

        tmpsize = av_image_fill_pointers(data, avctx->pix_fmt, h,
                                         NULL, linesize);
        if (tmpsize < 0)
            return -1;

        for (i = 0; i < 3 && data[i + 1]; i++)
            size[i] = data[i + 1] - data[i];
        size[i] = tmpsize - (data[i] - data[0]);

        for (i = 0; i < 4; i++) {
            dc->size[i] = size[i];
            pool->linesize[i] = linesize[i];
        }
        pool->format = frame->format;
        pool->width  = frame->width;
        pool->height = frame->height;

        break;
        }
    default:
        return -1;
    }
    return 0;
fail:
    pool->format = -1;
    pool->planes = pool->channels = pool->samples = 0;
    pool->width  = pool->height = 0;
    return ret;
}

static int lavc_GetFrame(struct AVCodecContext *ctx, AVFrame *frame, int flags)
{
    unsigned i;

    int ret = update_frame_pool(ctx, frame);
    if (ret < 0) {
        return ret;
    }
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get((AVPixelFormat)frame->format);
    memset(frame->data, 0, sizeof(frame->data));
    frame->extended_data = frame->data;
    DecoderImpl *dc = (DecoderImpl*)ctx->opaque;
    FramePool *pool = ctx->internal->pool;
    for (i = 0; i < 4 && dc->size[i]; i++) {
        frame->linesize[i] = pool->linesize[i];
        frame->data[i] = (uint8_t*)memalign(16, dc->size[i]);
        if ((frame->data[i] == NULL))
        {
            while (i > 0)
                av_buffer_unref(&frame->buf[--i]);
            goto error;
        }
        frame->buf[i] = av_buffer_create(frame->data[i], dc->size[i], lavc_ReleaseFrame,
                frame->data[i], 0);
        if ((frame->buf[i] == NULL))
        {
            while (i > 0)
                av_buffer_unref(&frame->buf[--i]);
            goto error;
        }
    }
    for (; i < AV_NUM_DATA_POINTERS; i++) {
        frame->data[i] = NULL;
        frame->linesize[i] = 0;
    }
    if (desc->flags & AV_PIX_FMT_FLAG_PAL ||
        desc->flags & AV_PIX_FMT_FLAG_PSEUDOPAL)
        avpriv_set_systematic_pal2((uint32_t *)frame->data[1], (AVPixelFormat)frame->format);
    return 0;
error:
    return -1;
}

DecoderImpl::DecoderImpl(Decoder *decoder)
    : m_io_ctx(NULL), m_codec_ctx(NULL), m_fmt_ctx(NULL), 
        m_pkt(NULL), m_sws_ctx(NULL), m_swr_ctx(NULL), m_opqua(NULL),
        m_stream_idx(-1), m_decoder(decoder), m_decoded_frame_cnt(0),
		m_width(0), m_height(0)
{
	memset(m_time_usage, 0, sizeof(m_time_usage));
}

DecoderImpl::~DecoderImpl()
{

}

Decoder::Decoder()
    : m_impl(new DecoderImpl(this)) , m_mem_used(0),memused_max(0),
memused_before_open(0), memused_before_dec(0), decode_cnt(0)
{
    mem_demux = 0;
    mem_snd_pkt = 0;
    mem_frm = 0;
}

Decoder::~Decoder()
{
	release();

    if (m_impl) {
        delete m_impl;
    }
}

int Decoder::release()
{
    if (m_impl->m_pkt) {
        av_packet_free(&m_impl->m_pkt);
        m_impl->m_pkt = NULL;
    }
    
    if (m_impl->m_io_ctx) {
        if (m_impl->m_io_ctx->buffer) {
            av_free(m_impl->m_io_ctx->buffer);
        }
        av_free(m_impl->m_io_ctx);
        m_impl->m_io_ctx = NULL;
    }

    if (m_impl->m_codec_ctx) {
        avcodec_free_context(&m_impl->m_codec_ctx);
        m_impl->m_codec_ctx = NULL;
    }

    if (m_impl->m_fmt_ctx) {
        avformat_close_input(&m_impl->m_fmt_ctx);
        m_impl->m_fmt_ctx = NULL;
    }
    if (m_impl->m_sws_ctx) {
        sws_freeContext(m_impl->m_sws_ctx);
    }
    m_impl->m_decoded_frame_cnt = 0;
}

int Decoder::poll()
{
    return 1;
}

int DecoderImpl::open_codec_ctx()
{
    Config &cfg = Config::get_instance();
    int ret, stream_index = -1;
    AVStream *st;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;
    enum AVMediaType type = AVMEDIA_TYPE_VIDEO;
    uint32_t mem = m_decoder->m_mem_used;
    ret = av_find_best_stream(m_fmt_ctx, type, -1, -1, &dec, 0);
    if (ret < 0) { 
        return ret;
    } else {
        int i;
        for (i = 0; i < m_fmt_ctx->nb_streams; i++) {
            st = m_fmt_ctx->streams[i];
            if (st->codecpar) {
            } else {
                continue;
            }
            if (st->codecpar->codec_type == type) {
                stream_index = i;
                break;
            }
        }
        if (stream_index < 0) {
            return -1;
        }
        st = m_fmt_ctx->streams[stream_index];

        if (!dec) 
            return AVERROR(EINVAL);

        // Allocate a codec context for the decoder 
        mem = m_decoder->m_mem_used;
        m_codec_ctx = avcodec_alloc_context3(dec);
        if (!m_codec_ctx) { 
            return AVERROR(ENOMEM);
        }
        if (cfg.m_get_buffer) {
            m_codec_ctx->get_buffer2 = lavc_GetFrame;
        }
        m_codec_ctx->opaque = this;
        switch(cfg.m_skip_frame) {
        case 0:
            m_codec_ctx->skip_frame = AVDISCARD_DEFAULT;
            break;
        case 1:
            m_codec_ctx->skip_frame = AVDISCARD_NONREF;
            break;
        case 2:
            m_codec_ctx->skip_frame = AVDISCARD_NONKEY;
            break;
        }
        mem = m_decoder->m_mem_used;
        // Copy codec parameters from input stream to output codec context 
        if ((ret = avcodec_parameters_to_context(m_codec_ctx, st->codecpar)) < 0) {
            return ret;
        }
        // Init the decoders, with or without reference counting 
        // av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
        mem = m_decoder->m_mem_used;
        if ((ret = avcodec_open2(m_codec_ctx, dec, &opts)) < 0) { 
            return ret;
        }

        m_stream_idx = stream_index;
    }
    return 0;
}

int DecoderImpl::stream_open()
{
    uint32_t mem = m_decoder->m_mem_used;
    int ret = avformat_open_input(&m_fmt_ctx, NULL, NULL, NULL);
    if (ret < 0) {
        MARK("");
        return -1;
    }
    /* retrieve stream information */
    mem = m_decoder->m_mem_used;
    ret = avformat_find_stream_info(m_fmt_ctx, NULL);
    if (ret < 0) {
        MARK("");
        return -1;
    }

    ret = open_codec_ctx();
    if (ret < 0)  {
        MARK("");
        return -1;
    }

    m_width = m_codec_ctx->width;
    m_height = m_codec_ctx->height;
    return 0;
}

int DecoderImpl::demux()
{
	int ret;
	struct timeval t1, t2;

	gettimeofday(&t1, NULL);
	av_packet_unref(m_pkt);
	av_init_packet(m_pkt);
	uint32_t mem = m_decoder->m_mem_used;
	ret = av_read_frame(m_fmt_ctx, m_pkt);
	m_decoder->mem_demux += m_decoder->m_mem_used - mem;
	gettimeofday(&t2, NULL);
	m_time_usage[0] += util_diff_time_us(t1, t2);

	if (ret < 0) {
		if (ret == AVERROR(EAGAIN)) {
			// need more data
			MARK("");
			return DEC_AGAIN;
		} else if (ret == AVERROR_EOF) {
		    m_pkt->data = 0;
		    m_pkt->size = 0;
		    return DEC_EOF;
		}
		char buf[64];
		av_strerror(ret, buf, sizeof(buf) -1);
		MARK("%s", buf);
		// sth wrong
		return DEC_ERROR;
	}

    if( m_pkt->stream_index != m_stream_idx) {
        av_packet_unref(m_pkt);
		//MARK("");
        return DEC_AGAIN;
    }
	// gotcha
    m_decoded_frame_cnt++;
	return DEC_OK;
}

void DecoderImpl::flush()
{
    if( avcodec_is_open(m_codec_ctx)) {
        avcodec_flush_buffers(m_codec_ctx);
    }
}

int DecoderImpl::decode()
{
	int ret = 0;
	struct timeval t1, t2;
	bool eof = false;
	//we're getting 0-sized packets before EOF for some reason. This seems like a semi-critical bug. Don't trigger EOF, and skip the packet
	if (m_pkt->data == NULL && m_pkt->size == 0) {
	    eof = true;
	}

	gettimeofday(&t1, NULL);
	char buf[64];
	uint32_t mem = m_decoder->m_mem_used;
	if (eof) {
        ret = avcodec_send_packet(m_codec_ctx, NULL);
	} else {
	    ret = avcodec_send_packet(m_codec_ctx, m_pkt);
	}
#if 1
    if (ret < 0)
    {
        if (ret == AVERROR_EOF) {
            //read flush cached frames
            return DEC_EOF;
        } else if (ret == AVERROR(EAGAIN)) {
            av_packet_unref(m_pkt);
            return DEC_AGAIN;
        } else {
            return DEC_ERROR;
        }
    } else {
        //we're getting 0-sized packets before EOF for some reason. This seems like a semi-critical bug. Don't trigger EOF, and skip the packet
        //av_packet_unref(m_pkt);
        //return 0;
    }
#else
    m_decoder->mem_snd_pkt += m_decoder->m_mem_used - mem;
	av_packet_unref(m_pkt);
	gettimeofday(&t2, NULL);
	m_time_usage[1] += diff_time_us(t1, t2);

	if (ret != 0 && ret != AVERROR(EAGAIN)) {
	    av_strerror(ret, buf, sizeof(buf));
		MARK("%s", buf);
		if (ret == AVERROR_EOF) {
		    return 2;
		}
		return -1;
	} else if (ret == AVERROR(EAGAIN)) {
        MARK("%s", buf);
	    return 0;
	}
#endif
	AVFrame *frame = av_frame_alloc();
	if (frame == NULL) {
		MARK("");
		return -1;
	}

	do {
		gettimeofday(&t1, NULL);
		mem = m_decoder->m_mem_used;
		ret = avcodec_receive_frame(m_codec_ctx, frame);
		m_decoder->mem_frm += m_decoder->m_mem_used - mem;
#if 1
        if( ret != 0 && ret != AVERROR(EAGAIN) )
        {
            if (ret == AVERROR(ENOMEM) || ret == AVERROR(EINVAL))
            {
                MARK("avcodec_receive_frame critical error");
                //*error = true;
            }
            av_frame_free(&frame);
            /* After draining, we need to reset decoder with a flush */
            if( ret == AVERROR_EOF ) {
                avcodec_flush_buffers(m_codec_ctx);
                return DEC_EOF;
            }
            return DEC_ERROR;
        } else if (ret == AVERROR(EAGAIN)) {
            break;
        }
#else
		if (ret != 0 && ret != AVERROR(EAGAIN)) {
		    av_frame_free(&frame);
			if (ret == AVERROR_EOF) {
				MARK("");
				//avcodec_flush_buffers(m_codec_ctx);
				return 2;
			}
			MARK("");
			return -1;
		}
		if (ret) {
			// EAGAIN
		    av_frame_free(&frame);
			return 0;
		}
#endif
#if 0
		uint64_t mem_used;
	    if (!m_sws_ctx) {
	        mem_used = m_decoder->m_mem_used;
	        m_sws_ctx = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format, frame->width, frame->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL,NULL,NULL);
	        printf("sws %d frm:%d pkt:%d fmt:%d codec:%d\n", m_decoder->m_mem_used - mem_used,
	                sizeof(*frame), sizeof(*m_pkt),
	                sizeof(*m_fmt_ctx), sizeof(*m_codec_ctx));
	    }
	    if (frame->format != AV_PIX_FMT_YUV420P) {
	    int video_dst_linesize[4] = {frame->width, frame->width/2, frame->width/2, 0};
	    int dst_buf_size = (frame->width * frame->height * 3) >> 1;
	    int video_y_size = frame->width * frame->height;
	    int video_u_size = video_y_size / 4;
	    char *msg = (char*)malloc(dst_buf_size);
	    uint8_t *video_dst_data[4] = {NULL};
	    video_dst_data[0] = (uint8_t *)msg;
        video_dst_data[1] = (uint8_t *)msg + video_y_size;
        video_dst_data[2] = (uint8_t *)msg + video_y_size + video_u_size;
        mem_used = m_decoder->m_mem_used;
		sws_scale(m_sws_ctx, (const uint8_t * const*)frame->data, frame->linesize, 0, frame->height, video_dst_data, video_dst_linesize);
		printf("sws %d\n", m_decoder->m_mem_used - mem_used);
		free(msg);
	    }
#endif
		m_decoder->decode_cnt++;
		m_cb_recv_frame(m_decoder, frame);
		av_frame_unref(frame);
	} while (ret >= 0);
    av_frame_free(&frame);
	gettimeofday(&t2, NULL);
	m_time_usage[2] += util_diff_time_us(t1, t2);
	if (eof) {
	    return DEC_EOF;
	}
	return DEC_OK;
}
void Decoder::time_usage_show()
{
	int i;
	for (i = 0; i < 10; i++) {
		MARK("time useage %d: %d ms", i, m_impl->m_time_usage[i] / 1000);
	}
}
uint16_t Decoder::get_width() const
{
	return m_impl->m_width;
}
uint16_t Decoder::get_height() const
{
	return m_impl->m_height;
}

int Decoder::demux()
{
	return m_impl->demux();
}

int Decoder::open()
{
    uint32_t mem = m_mem_used;
	int ret = m_impl->stream_open();
	memused_before_dec = m_mem_used;//memused_max;
	return ret;
}

int Decoder::decode()
{
	return m_impl->decode();
}

int Decoder::__read_pkt(void *opaque, uint8_t *buf, int size)
{
    Decoder *dc = (Decoder*)opaque;
    return dc->m_impl->m_cb_read_pkt(dc, buf, size);
}

int Decoder::set_cb_read_pkt(cb_read_pkt fn, void *param)
{
	m_impl->m_cb_read_pkt = fn;
	m_impl->m_read_pkt_param = param;
	return 0;
}

void *Decoder::get_read_pkt_param()
{
	return m_impl->m_read_pkt_param;
}

int Decoder::set_cb_recv_frame(cb_recv_frame fn, void *param)
{
	m_impl->m_cb_recv_frame = fn;
	m_impl->m_recv_frame_param = param;
	return 0;
}

void *Decoder::get_recv_frame_param()
{
	return m_impl->m_recv_frame_param;
}

uint32_t Decoder::get_decoded_frame_cnt()
{
	return m_impl->m_decoded_frame_cnt;
}

int Decoder::init()
{
    const uint32_t avio_ctx_buffer_size = 4096;
    uint8_t * avio_ctx_buffer = NULL;

    if (!(m_impl->m_fmt_ctx = avformat_alloc_context())) {
        MARK("");
        goto end;
    }

    avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        MARK("");
        goto end;

    }

    static uint8_t buf[24];
    m_impl->m_io_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, this, &__read_pkt, NULL, NULL);
    if (!m_impl->m_io_ctx) {
        MARK("");
        goto end;
    }      
    m_impl->m_fmt_ctx->pb = m_impl->m_io_ctx;
    m_impl->m_pkt = av_packet_alloc();
    if (!m_impl->m_pkt) {
        MARK("");
        goto end;
    }
//    memused_before_open = memused_max;
    memused_before_open = m_mem_used;
    return 0;
end:
    return -1;
}

void Decoder::debug_show()
{
    printf("raw_packet_buffer_remaining_size:%d\n", m_impl->m_fmt_ctx->internal->raw_packet_buffer_remaining_size);
}

