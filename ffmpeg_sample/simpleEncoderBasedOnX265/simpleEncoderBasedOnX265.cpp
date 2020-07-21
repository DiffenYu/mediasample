#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "x265.h"

int main()
{
    FILE* fp_src = fopen("../clips/bbc_640x480_374.yuv", "rb");
    FILE* fp_dst = fopen("./bbc_out.h265", "wb");

    if (fp_src == NULL || fp_dst == NULL) {
        printf("Error open file\n");
        return -1;
    }

    int frame_num = 300;
    int csp = X265_CSP_I420;
    int width = 640;
    int height = 480;

    x265_param* pParam = x265_param_alloc();
    x265_param_default(pParam);
    pParam->bRepeatHeaders = 1;
    pParam->internalCsp = csp;
    pParam->sourceWidth = width;
    pParam->sourceHeight = height;
    pParam->fpsNum = 25;
    pParam->fpsDenom = 1;

    x265_encoder* pHandle = x265_encoder_open(pParam);
    if (pHandle == NULL) {
        printf("x265_encoder_opern error\n");
        return -1;
    }

    int y_size = pParam->sourceWidth * pParam->sourceHeight;
    x265_picture* pPic_in = x265_picture_alloc();
    x265_picture_init(pParam, pPic_in);
    char* buff = NULL;
    switch (csp) {
    case X265_CSP_I420:
        buff = (char*)malloc(y_size * 3);
        pPic_in->planes[0] = buff;
        pPic_in->planes[1] = buff + y_size;
        pPic_in->planes[2] = buff + y_size * 5 / 4;
        pPic_in->stride[0] = width;
        pPic_in->stride[1] = width / 2;
        pPic_in->stride[2] = width / 2;
        break;
    default:
        printf("Colorspace not supported\n");
        return -1;
    }

    if (frame_num == 0) {
        fseek(fp_src, 0, SEEK_END);
        switch (csp) {
        case X265_CSP_I420:
            frame_num = ftell(fp_src) / (y_size * 3);
            break;
        default:
            printf("Colorspace not supported\n");
            return -1;
        }
        fseek(fp_src, 0, SEEK_SET);
    }

    int ret;
    x265_nal* pNals = NULL;
    uint32_t iNal = 0;

    for (int i = 0; i < frame_num; ++i) {
        switch (csp) {
        case X265_CSP_I420:
            fread(pPic_in->planes[0], 1, y_size, fp_src);
            fread(pPic_in->planes[1], 1, y_size / 4, fp_src);
            fread(pPic_in->planes[2], 1, y_size / 4, fp_src);
            break;
        default:
            printf("Colorspace not supported\n");
            return -1;
        }

        ret = x265_encoder_encode(pHandle, &pNals, &iNal, pPic_in, NULL);
        printf("Succeed encode %5d frames\n", i);

        for (int j = 0; j < iNal; ++j) {
            fwrite(pNals[j].payload, 1, pNals[j].sizeBytes, fp_dst);
        }

    }

    while(1) {
        ret = x265_encoder_encode(pHandle, &pNals, &iNal, NULL, NULL);
        if (ret == 0) {
            break;
        }
        printf("Flush 1 frame.\n");

        for (int j = 0; j < iNal; ++j){
            fwrite(pNals[j].payload, 1, pNals[j].sizeBytes, fp_dst);
        }
    }

    x265_encoder_close(pHandle);
    x265_picture_free(pPic_in);
    x265_param_free(pParam);
    free(buff);
    fclose(fp_src);
    fclose(fp_dst);

    return 0;
}
