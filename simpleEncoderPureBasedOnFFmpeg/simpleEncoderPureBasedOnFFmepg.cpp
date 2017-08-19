#include <stdio.h>
#define __STDC_CONSTANT_MACROS

//#ifdef __cpluscplus
extern "C" {
//#endif
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
//#ifdef __cpluscplus
}
//#endif

#define TEST_H264 1
#define TEST_HEVC 0

int main()
{
    int i, ret, got_output;
    FILE* fp_in;
    FILE* fp_out;
    AVFrame* pFrame;
    AVPacket pkt;
    int y_size;

    char filename_in[] = "../clips/bbc_640x480_374.yuv";

#if TEST_H264
    AVCodecID codec_id = AV_CODEC_ID_H264;
    char filename_out[] = "outbbc.h264";
#else
    AVCodecID codec_id = AV_CODEC_ID_HEVC;
    char filename_out[] = "outbbc.hevc";
#endif

    int in_w = 640;
    int in_h = 480;
    int framecnt = 100;

    avcodec_register_all();

    AVCodec* pCodec = avcodec_find_encoder(codec_id);
    if (!pCodec) {
        printf("Codec not found\n");
        return -1;
    }

    AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        printf("Could not allocate video codec context\n");
        return -1;
    }

    pCodecCtx->bit_rate = 400000;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
    pCodecCtx->gop_size = 10;
    pCodecCtx->max_b_frames = 1;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec_id == AV_CODEC_ID_H264) {
        av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec\n");
        return -1;
    }

    pFrame = av_frame_alloc();
    if (!pFrame) {
        printf("Could not allocate video frame\n");
        return -1;
    }

    pFrame->format = pCodecCtx->pix_fmt;
    pFrame->width = pCodecCtx->width;
    pFrame->height = pCodecCtx->height;

    ret = av_image_alloc(pFrame->data, pFrame->linesize, pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 16);

    if (ret < 0) {
        printf("Could not allocate raw picture buffer\n");
        return -1;
    }

    //input raw data
    fp_in = fopen(filename_in, "rb");
    if (!fp_in) {
        printf("Could not open %s\n", filename_in);
        return -1;
    }

    //Output bitstream
    fp_out = fopen(filename_out, "wb");
    if (!fp_out) {
        printf("Could not open %s\n", filename_out);
        return -1;
    }

    y_size = pCodecCtx->width * pCodecCtx->height;

    //Encode
    for (i = 0; i < framecnt; ++i) {
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;
        //Read raw YUV data
        if (fread(pFrame->data[0], 1, y_size, fp_in)     <= 0 ||
            fread(pFrame->data[1], 1, y_size / 4, fp_in) <= 0 ||
            fread(pFrame->data[2], 1, y_size / 4, fp_in) <= 0)
            return -1;
        else if (feof(fp_in)) {
            break;
        }
        pFrame->pts = i;
        ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_output);
        if (ret < 0) {
            printf("Error encoding frame\n");
            return -1;
        }
        
        if (got_output) {
            printf("Succeed to encode frame: %5d\tsize:%5d\n", framecnt, pkt.size);
            framecnt++;
            fwrite(pkt.data, 1, pkt.size, fp_out);
            av_free_packet(&pkt);
        }
    }

    //Flush Encoder
    for (got_output = 1; got_output; i++) {
        ret = avcodec_encode_video2(pCodecCtx, &pkt, NULL, &got_output);
        if (ret < 0) {
            printf("Error encoding frame\n");
            return -1;
        }

        if (got_output) {
            printf("Flush encoder: Succeed to encode 1 frame!\tsize:%5d\n", pkt.size);
            fwrite(pkt.data, 1, pkt.size, fp_out);
            av_free_packet(&pkt);
        }
    }

    fclose(fp_out);
    avcodec_close(pCodecCtx);
    av_free(pCodecCtx);
    av_freep(&pFrame->data[0]);
    av_frame_free(&pFrame);

    return 0;
}
