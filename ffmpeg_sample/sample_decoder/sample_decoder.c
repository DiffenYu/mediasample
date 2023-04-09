#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>

#define INBUF_SIZE 4096
static int64_t timestamp = 0;

static void write_yuv_to_file(AVFrame* frame, FILE* fp_out) {
    if (fp_out) {
        for (int color_idx = 0; color_idx < 3; color_idx++) {
            int width = color_idx == 0 ? frame->width : frame->width / 2;
            int height = color_idx == 0 ? frame->height: frame->height/ 2;
            uint8_t* ptr = frame->data[color_idx];
            for (int ih = 0; ih < height; ih++) {
                if (fwrite(ptr, 1, width, fp_out) <= 0) {
                    fprintf(stderr, "Error write to file\n");
                    exit(1);
                }
                ptr += frame->linesize[color_idx];
            }
            fflush(fp_out);
        }


        //Write raw YUV data
        /*int y_size = frame->width * frame->height;*/
        /*if (fwrite(frame->data[0], 1, y_size, fp_out    ) <= 0 ||*/
            /*fwrite(frame->data[1], 1, y_size / 4, fp_out) <= 0 ||*/
            /*fwrite(frame->data[2], 1, y_size / 4, fp_out) <= 0) {*/
            /*fprintf(stderr, "Error write to file\n");*/
            /*exit(1);*/
        /*}*/
        /*fflush(fp_out);*/
    }
}

static void write_nv12_to_file(AVFrame* frame, FILE* fp_out) {
    if (fp_out) {
        for (int color_idx = 0; color_idx < 2; color_idx++) {
            int width = frame->width;
            int height = color_idx == 0 ? frame->height: frame->height/ 2;
            uint8_t* ptr = frame->data[color_idx];
            for (int ih = 0; ih < height; ih++) {
                if (fwrite(ptr, 1, width, fp_out) <= 0) {
                    fprintf(stderr, "Error write to file\n");
                    exit(1);
                }
                ptr += frame->linesize[color_idx];
            }
            fflush(fp_out);
        }
    }
}

static void write_yuv_to_separate_file(AVFrame* frame, char* filename) {
    FILE* fp_out = fopen(filename, "wb");
    if (!fp_out) {
        fprintf(stderr, "Error open file %s\n", filename);
        exit(1);
    }
    uint8_t** data = frame->data;
    int* linesize = frame->linesize;
    for (int color_idx = 0; color_idx < 3; color_idx++) {
        int width = color_idx == 0 ? frame->width : frame->width / 2;
        int height = color_idx == 0 ? frame->height : frame->height / 2;
        for (int ih = 0; ih < height; ih++) {
            if (fwrite(data[color_idx], 1, width, fp_out) <= 0) {
                fprintf(stderr, "Error write to file\n");
                exit(1);
            }
            data[color_idx] += linesize[color_idx];
        }
        fflush(fp_out);
    }
    fclose(fp_out);
}

static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,
                   FILE* fp_out)
{
    char buf[1024];
    char* filename = "dump_out";
    int ret;

    /*dec_ctx->reordered_opaque = timestamp++;*/

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        }
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding, ret = %d\n", ret);
            exit(1);
        }

        printf("saving frame %3d\n", dec_ctx->frame_number);
        printf("output format is %s\n", av_get_pix_fmt_name(frame->format));
        fflush(stdout);
        /*printf("frame->format = %d, frame->width = %d, frame->height = %d, frame->linesize[0] = %d\n", frame->format, frame->width, frame->height, frame->linesize[0]);*/
        /*printf("frame->linesize[1] = %d, frame->linesize[2] = %d\n", frame->linesize[1], frame->linesize[2]);*/
        /*printf("frame->key_frame = %d\n", frame->key_frame);*/

        snprintf(buf, sizeof(buf), "%s_%dx%d_%d.yuv", filename, frame->width, frame->height, dec_ctx->frame_number);
        /*write_yuv_to_separate_file(frame, buf);*/
        if (frame->format == AV_PIX_FMT_YUV420P) {
            write_yuv_to_file(frame, fp_out);
        } else if (frame->format == AV_PIX_FMT_NV12) {
            write_nv12_to_file(frame, fp_out);
        }
        av_frame_unref(frame);

        /*if (dec_ctx->frame_number >= 5) {*/
            /*fclose(fp_out);*/
            /*exit(0);*/
        /*}*/
    }
}

int main(int argc, char **argv)
{
    av_log_set_level(AV_LOG_DEBUG);
    const char *filename, *filename_out;
    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *c= NULL;
    FILE *f;
    AVFrame *frame;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t   data_size;
    int ret;
    AVPacket *pkt;

    if (argc <= 3) {
        fprintf(stderr, "Usage: %s <input file> <output file> <decoder implementation[sw|cuvid|qsv]\n"
                "And check your input file is raw .h264 file.\n", argv[0]);
        exit(0);
    }
    filename    = argv[1];
    filename_out = argv[2];
    const char* codec_name = "h264";
    if (strcmp(argv[3], "sw") == 0) {
        codec_name = "h264";
    } else if (strcmp(argv[3], "cuvid") == 0) {
        codec_name = "h264_cuvid";
    } else if (strcmp(argv[3], "qsv") == 0) {
        codec_name = "h264_qsv";
    }
    FILE* fp_out = fopen(filename_out, "wb");
    if (!fp_out) {
        printf("Could not open %s\n", filename_out);
        exit(1);
    }


    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    /*codec = avcodec_find_decoder(AV_CODEC_ID_H264);*/
    codec = avcodec_find_decoder_by_name(codec_name);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "parser not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
    printf("pixel format  = %s\n", av_get_pix_fmt_name(c->pix_fmt));


    f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Could not allocate packet\n");
        exit(1);
    }

    while (!feof(f)) {
        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, f);
        if (!data_size)
            break;

        /* use the parser to split the data into frames */
        data = inbuf;
        while (data_size > 0) {
            ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
                                   data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                exit(1);
            }
            data      += ret;
            data_size -= ret;

            if (pkt->size)  {
                decode(c, frame, pkt, fp_out);
            }
        }
    }

    /* flush the decoder */
    decode(c, frame, NULL, fp_out);

    fclose(f);
    fclose(fp_out);

    av_parser_close(parser);
    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}
