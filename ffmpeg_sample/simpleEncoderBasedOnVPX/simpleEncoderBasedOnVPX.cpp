#include <stdio.h>
#include <stdlib.h>

#include "vpx/vp8cx.h"
#include "vpx/vpx_encoder.h"

#define INTERFACE (&vpx_codec_vp8_cx_algo)
#define fourcc 0x30385056

static void mem_put_1e16(char* mem, unsigned int val)
{
    mem[0] = val;
    mem[1] = val >> 8;
}

static void mem_put_1e32(char* mem, unsigned int val)
{
    mem[0] = val;
    mem[1] = val >> 8;
    mem[2] = val >> 16;
    mem[3] = val >> 24;
}

static void write_ivf_file_header(FILE* outfile, const vpx_codec_enc_cfg_t* cfg, int frame_cnt)
{
    char header[32];
    if (cfg->g_pass != VPX_RC_ONE_PASS && cfg->g_pass != VPX_RC_LAST_PASS)
        return;

    header[0] = 'D';
    header[1] = 'K';
    header[2] = 'I';
    header[3] = 'F';
    mem_put_1e16(header + 4, 0);  // version
    mem_put_1e16(header + 6, 32); // headersize
    mem_put_1e32(header + 8, fourcc);
    mem_put_1e16(header + 12, cfg->g_w);
    mem_put_1e16(header + 14, cfg->g_h);
    mem_put_1e32(header + 16, cfg->g_timebase.den);
    mem_put_1e32(header + 20, cfg->g_timebase.num);
    mem_put_1e32(header + 24, frame_cnt);
    mem_put_1e32(header + 28, 0);

    fwrite(header, 1, 32, outfile);
}

static void write_ivf_frame_header(FILE* outfile, const vpx_codec_cx_pkt_t* pkt) {
    char header[12];
    vpx_codec_pts_t pts;
    
    if (pkt->kind != VPX_CODEC_CX_FRAME_PKT)
        return;

    pts = pkt->data.frame.pts;
    mem_put_1e32(header, pkt->data.frame.sz);
    mem_put_1e32(header + 4, pts & 0xFFFFFFFF);
    mem_put_1e32(header + 8, pts >> 32);

    fwrite(header, 1, 12, outfile);
}

int main()
{
    // Open input file for this encoding pass
    FILE* infile = fopen("../clips/bbc_640x480_374.yuv", "rb");
    FILE* outfile = fopen("./bbc_out.ivf", "wb");

    if (infile == NULL || outfile == NULL) {
        printf("Error open file\n");
        return -1;
    }

    vpx_image_t raw;
    int width = 640;
    int height = 480;

    if (!vpx_img_alloc(&raw, VPX_IMG_FMT_I420, width, height, 1)) {
        printf("Fail to allocate image\n");
        return -1;
    }

    printf("Using %s\n", vpx_codec_iface_name(INTERFACE));

    // Populate encoder configuration
    vpx_codec_err_t ret;
    vpx_codec_enc_cfg_t cfg;
    ret = vpx_codec_enc_config_default(INTERFACE, &cfg, 0);
    if (ret) {
        printf("Failed to get config: %s\n", vpx_codec_err_to_string(ret));
        return -1;
    }

    // Update the default setting with our setting
    cfg.rc_target_bitrate = 800;
    cfg.g_w = width;
    cfg.g_h = height;

    write_ivf_file_header(outfile, &cfg, 0);

    vpx_codec_ctx_t codec;
    // Initialize codec
    if (vpx_codec_enc_init(&codec, INTERFACE, &cfg, 0)) {
        printf("Failed to initialize encoder\n");
        return -1;
    }

    int frame_avail = 1;
    int got_data = 0;

    int y_size = cfg.g_w * cfg.g_h;
    int frame_cnt = 0;
    int flags = 0;

    while (frame_avail || got_data) {
        vpx_codec_iter_t iter = NULL;
        const vpx_codec_cx_pkt_t* pkt;
        if (fread(raw.planes[0], 1, y_size * 3 / 2, infile) != y_size * 3 / 2) {
            frame_avail = 0;
        }

        if (frame_avail) {
            ret = vpx_codec_encode(&codec, &raw, frame_cnt, 1, flags, VPX_DL_REALTIME);
        } else {
            ret = vpx_codec_encode(&codec, NULL, frame_cnt, 1, flags, VPX_DL_REALTIME);
        }

        if (ret) {
            printf("Failed to encode frame\n");
            return -1;
        }

        got_data = 0;

        while (pkt = vpx_codec_get_cx_data(&codec, &iter)) {
            got_data = 1;
            switch (pkt->kind) {
            case VPX_CODEC_CX_FRAME_PKT:
                write_ivf_frame_header(outfile, pkt);
                fwrite(pkt->data.frame.buf, 1, pkt->data.frame.sz, outfile);
                break;
            default:
                break;
            }
        }

        printf("Succeed encode frame: %5d\n", frame_cnt);
        ++frame_cnt;
    }

    fclose(infile);
    vpx_codec_destroy(&codec);

    if (!fseek(outfile, 0, SEEK_SET)) {
        write_ivf_file_header(outfile, &cfg, frame_cnt - 1);
    }
    fclose(outfile);
}
