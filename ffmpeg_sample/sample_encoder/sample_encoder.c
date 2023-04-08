#include <stdio.h>
#define __STDC_CONSTANT_MACROS

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>

const char* codec_name = "libx264";
/*const char* codec_name = "h264_nvenc";*/

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
        FILE *outfile)
{
    int ret;

    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %3" PRId64 "\n", frame->pts);

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            printf("EAGAIN OR AVERROR_EOF\n");
            return;
        }
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }

        if (pkt->flags & AV_PKT_FLAG_KEY) {
            printf("KEY Frame\n");
        }

        printf("Write packet %3" PRId64 " (size=%5d)\n", pkt->pts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
}

int main(int argc, char** argv)
{
    av_log_set_level(AV_LOG_DEBUG);
    if (argc <= 4) {
        fprintf(stderr, "Usage: %s <input file> <output file> input_width input_height\n"
                "And check your input file is raw yuv file.\n", argv[0]);
        exit(0);
    }
    const char* filename_in = argv[1];
    const char* filename_out = argv[2];
    int in_w = atoi(argv[3]);
    int in_h = atoi(argv[4]);
    int framecnt = 100;

    // You can find encoder by name (list in allcodecs.c, search the corresponding name) or by codec_id.
    //AVCodec* pCodec = avcodec_find_encoder(codec_id);
    AVCodec* pCodec = avcodec_find_encoder_by_name(codec_name);
    if (!pCodec) {
        printf("Codec not found\n");
        return -1;
    }

    AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        printf("Could not allocate video codec context\n");
        return -1;
    }

    pCodecCtx->bit_rate = 800000;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->codec_id = AV_CODEC_ID_H264;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    pCodecCtx->framerate.num = 15;
    pCodecCtx->framerate.den = 1;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 15; // should be matched with the pts of AVFrame
    pCodecCtx->gop_size = 150;
    pCodecCtx->keyint_min = 150;
    pCodecCtx->max_b_frames = 0;
    pCodecCtx->has_b_frames = 0;
    pCodecCtx->qmin = 20;
    pCodecCtx->qmax = 40;
    pCodecCtx->profile = 66;
    pCodecCtx->flags2 |= AV_CODEC_FLAG2_FAST;

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec\n");
        return -1;
    }

    AVFrame* pFrame = av_frame_alloc();
    if (!pFrame) {
        printf("Could not allocate video frame\n");
        return -1;
    }

    pFrame->format = pCodecCtx->pix_fmt;
    pFrame->width = pCodecCtx->width;
    pFrame->height = pCodecCtx->height;

    int ret = av_frame_get_buffer(pFrame, 0);
    if (ret < 0) {
        printf("Could not allocate the video frame data\n");
        return -1;
    }

    //input raw data
    FILE* fp_in = fopen(filename_in, "rb");
    if (!fp_in) {
        printf("Could not open %s\n", filename_in);
        return -1;
    }

    //Output bitstream
    FILE* fp_out = fopen(filename_out, "wb");
    if (!fp_out) {
        printf("Could not open %s\n", filename_out);
        return -1;
    }

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        printf("Could not allocate packet\n");
        return -1;
    }

    //Encode
    int y_size = pCodecCtx->width * pCodecCtx->height;
    for (int i = 0; i < framecnt; ++i) {
        /* make sure the frame data is writable */
        ret = av_frame_make_writable(pFrame);
        if (ret < 0) {
            printf("error in frame make writable\n");
            return -1;
        }
        //Read raw YUV data
        if (fread(pFrame->data[0], 1, y_size, fp_in)     <= 0 ||
            fread(pFrame->data[1], 1, y_size / 4, fp_in) <= 0 ||
            fread(pFrame->data[2], 1, y_size / 4, fp_in) <= 0)
            return -1;
        else if (feof(fp_in)) {
            break;
        }
        pFrame->pts = i;
        encode(pCodecCtx, pFrame, pkt, fp_out);
    }

    encode(pCodecCtx, NULL, pkt, fp_out);


    fclose(fp_out);
    avcodec_free_context(&pCodecCtx);
    av_frame_free(&pFrame);
    av_packet_free(&pkt);

    return 0;
}
