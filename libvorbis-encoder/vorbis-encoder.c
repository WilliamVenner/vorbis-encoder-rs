// Author: Hossein Noroozpour
// Email:  hossein.noroozpour@gmail.com
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

#define READ_SIZE 4096

typedef struct {
	ogg_stream_state os;
	ogg_page         og;
	ogg_packet       op;
	vorbis_info      vi;
	vorbis_comment   vc;
	vorbis_dsp_state vd;
	vorbis_block     vb;
	void            *data;
	unsigned int     data_length;
} vorbis_encoder_helper_block;

typedef struct {
	void *private_data;
} vorbis_encoder_helper;

static void write_data(vorbis_encoder_helper *hp, void *data, unsigned int len) {
	vorbis_encoder_helper_block *hb = (vorbis_encoder_helper_block *)hp->private_data;
	unsigned int size = len + hb->data_length;
	void *tmp = malloc(size);
	memcpy(tmp, hb->data, hb->data_length);
	memcpy(tmp + hb->data_length, data, len);
	hb->data_length += len;
	free(hb->data);
	hb->data = tmp;
}

int vorbis_encoder_helper_init(vorbis_encoder_helper *hp, unsigned int ch, unsigned long int rt, float q) {
	hp->private_data = malloc(sizeof(vorbis_encoder_helper_block));
	vorbis_encoder_helper_block *hb = (vorbis_encoder_helper_block *)hp->private_data;
	vorbis_info_init(&(hb->vi));
	int ret = vorbis_encode_init_vbr(&(hb->vi), ch, rt, q);
	if (ret != 0) return ret;
	vorbis_comment_init(&(hb->vc));
	vorbis_analysis_init(&(hb->vd), &(hb->vi));
	vorbis_block_init(&(hb->vd), &(hb->vb));
	srand(time(NULL));
	ogg_stream_init(&(hb->os), rand());
	ogg_packet header;
	ogg_packet header_comm;
	ogg_packet header_code;
	vorbis_analysis_headerout(&(hb->vd), &(hb->vc), &header, &header_comm,
		&header_code);
	ogg_stream_packetin(&(hb->os), &header);
	ogg_stream_packetin(&(hb->os), &header_comm);
	ogg_stream_packetin(&(hb->os), &header_code);
	while(1) {
		int result = ogg_stream_flush(&(hb->os), &(hb->og));
		if(result==0) break;
		write_data(hp, (hb->og).header, (hb->og).header_len);
		write_data(hp, (hb->og).body,   (hb->og).body_len);
	}
	return 0;
}


static int vorbis_encoder_helper_block_out(vorbis_encoder_helper *hp) {
	vorbis_encoder_helper_block *hb = (vorbis_encoder_helper_block *)hp->private_data;
	while (vorbis_analysis_blockout(&(hb->vd), &(hb->vb)) == 1) {
		vorbis_analysis(&(hb->vb),NULL);
		vorbis_bitrate_addblock(&(hb->vb));
		while (vorbis_bitrate_flushpacket(&(hb->vd), &(hb->op))) {
			ogg_stream_packetin(&(hb->os),&(hb->op));
			unsigned int eos = 0;
			while (!eos) {
				int result = ogg_stream_pageout(&(hb->os), &(hb->og));
				if (result == 0) break;
				write_data(hp, (hb->og).header, (hb->og).header_len);
				write_data(hp, (hb->og).body,   (hb->og).body_len);
				if (ogg_page_eos(&(hb->og))) eos = 1;
			}
		}
	}
	return 0;
}

int vorbis_encoder_helper_flush(vorbis_encoder_helper *hp) {
	vorbis_encoder_helper_block *hb = (vorbis_encoder_helper_block *)hp->private_data;
	vorbis_analysis_wrote(&(hb->vd), 0);
	return vorbis_encoder_helper_block_out(hp);
}

unsigned int vorbis_encoder_helper_get_data_length(vorbis_encoder_helper *hp) {
	vorbis_encoder_helper_block *hb = (vorbis_encoder_helper_block *)hp->private_data;
	return hb->data_length;
}

void vorbis_encoder_helper_get_data(vorbis_encoder_helper *hp, unsigned char *data) {
	vorbis_encoder_helper_block *hb = (vorbis_encoder_helper_block *)hp->private_data;
	memcpy(data, hb->data, hb->data_length);
	free(hb->data);
	hb->data = malloc(0);
	hb->data_length = 0;
}

int vorbis_encoder_helper_encode(vorbis_encoder_helper *hp, int16_t *data, unsigned int bits) {
	vorbis_encoder_helper_block *hb = (vorbis_encoder_helper_block *)hp->private_data;
#define BUFSZ 4096
	for(unsigned int read = BUFSZ < bits? BUFSZ: bits;
			bits > 0;
			bits -= read, data += read, read = BUFSZ < bits? BUFSZ: bits) {
		unsigned int samples = read / (hb->vi.channels);
		float **buffer=vorbis_analysis_buffer(&(hb->vd), samples);
		unsigned int i, j, data_index = 0;
		for (i = 0; i < samples; ++i) {
			for (j = 0; j < (hb->vi.channels); ++j, ++data_index) {
				buffer[j][i]= ((float)(data[data_index])) / 32768.0f;
			}
		}
		vorbis_analysis_wrote(&(hb->vd), samples);
		int res = vorbis_encoder_helper_block_out(hp);
		if (res != 0) {
			return res;
		}
	}
	return 0;
#undef BUFSZ
}

int vorbis_encoder_helper_free(vorbis_encoder_helper *hp) {
	vorbis_encoder_helper_block *hb = (vorbis_encoder_helper_block *)hp->private_data;
	ogg_stream_clear(&(hb->os));
	vorbis_block_clear(&(hb->vb));
	vorbis_dsp_clear(&(hb->vd));
	vorbis_comment_clear(&(hb->vc));
	vorbis_info_clear(&(hb->vi));
	free(hb->data);
	free(hb);
	return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
