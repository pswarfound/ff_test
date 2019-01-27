#if 1
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <malloc.h>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "Decoder.hpp"

using std::vector;
#define ALIGN (64)
extern "C" {
void *jut_malloc(size_t size);
void *jut_realloc(void *ptr, size_t size);
void  jut_free(void *ptr);
}

typedef struct {
	union {
		uint8_t res[16];
		uint32_t size;
	};
} jut_mem_hdr_t;

int policy = 4;
__thread void *tls_mem_pool = NULL;
//#define memalign(a,b) malloc(b)
extern Decoder *tls_get_ctx();
static pthread_t tid;

uint32_t jmem_get_max_size()
{
	return 0;
}

void *jut_malloc(size_t size)
{
	void *p = NULL;
	switch(policy) {
	case 1:
		return malloc(size);
	case 4: {
		jut_mem_hdr_t *hdr;
		Decoder *dc = tls_get_ctx();
		p =  memalign(ALIGN, size + ALIGN);
		if (p) {
			hdr = (jut_mem_hdr_t*)p;
			hdr->size = size;
			p = (uint8_t*)p + ALIGN;
			if (dc) {
				dc->m_mem_used += size;
				if (dc->m_mem_used > dc->memused_max) {
					dc->memused_max = dc->m_mem_used;
				}
			}
		}
		break;
	}
	case 5: {
		if (size == 0) {
			return malloc(size);
		}
		p =  memalign(ALIGN, size + ALIGN);
		if (p) {
			p = (uint8_t*)p + ALIGN;
		}
		break;
	}
	}
	return p;
}

void *jut_realloc(void *ptr, size_t size)
{
	void *p = NULL;
	switch(policy) {
	case 1:
		return realloc(ptr, size);
	case 4: {
		jut_mem_hdr_t *hdr = NULL;
		Decoder *dc = tls_get_ctx();
		if (ptr) {
			hdr = (jut_mem_hdr_t*)((uint8_t*)ptr - ALIGN);
			if (dc) {
				dc->m_mem_used -= hdr->size;
			}
			p = realloc(hdr, size + ALIGN);
		} else if (size != 0) {
			// ptr is null and size !=0, equal to malloc
			p = realloc(NULL, size + ALIGN);
		} else {
			// nothing to do
			return NULL;
		}

		if (p) {
			hdr = (jut_mem_hdr_t*)p;
			hdr->size = size;
			if (dc) {
				dc->m_mem_used += hdr->size;
			}
			p = (uint8_t*)p + ALIGN;
		}
		break;
	}
	case 5: {
		jut_mem_hdr_t *hdr = NULL;
		if (ptr) {
			hdr = (jut_mem_hdr_t*)((uint8_t*)ptr - ALIGN);
			p = realloc(hdr, size + ALIGN);
		} else if (size != 0) {
			// ptr is null and size !=0, equal to malloc
			p = realloc(ptr, size + ALIGN);
		} else {
			// nothing to do
			return NULL;
		}

		if (p) {
			hdr = (jut_mem_hdr_t*)p;
			hdr->size = size;
			p = (uint8_t*)p + ALIGN;
		}
		break;
	}
	}
	return p;
}

void jut_free(void *ptr)
{
	if (!ptr) {
		return;
	}
	switch(policy) {
	case 1:
		return free(ptr);
	case 4: {
		Decoder *dc = tls_get_ctx();
		jut_mem_hdr_t *hdr = NULL;
		hdr = (jut_mem_hdr_t*)((uint8_t*)ptr - ALIGN);

		if (dc) {
			dc->m_mem_used -= hdr->size;
		}
		free(hdr);
		break;
	}
	case 5: {
		jut_mem_hdr_t *hdr = (jut_mem_hdr_t *)((uint8_t*)ptr - ALIGN);
		free(hdr);
		break;
	}
	}
}
#endif
