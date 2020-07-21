#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "x264.h"

int main()
{
    printf("This is a simple encoder based on x264\n");

    FILE* fp_src = fopen("./bbc_640x480_374.yuv", "rb");
    FILE* fp_dst = fopen("./bbc_out.h264", "wb");

    if (fp_src == NULL || fp_dst == NULL) {
        printf("Error open file\n");
        return -1;
    }

    int frame_num = 300;
    int csp = X264_CSP_I420;
    int width = 640;
    int height = 480;

    x264_param_t* pParam = (x264_param_t*)malloc(sizeof(x264_param_t));
    ;
    x264_param_default(pParam);

    pParam->i_width = width;
    pParam->i_height = height;
    pParam->i_csp = csp;
    x264_param_apply_profile(pParam, x264_profile_names[5]);

    x264_t* pHandle = x264_encoder_open(pParam);

    x264_picture_t* pPic_out = (x264_picture_t*)malloc(sizeof(x264_picture_t));
    x264_picture_init(pPic_out);

    x264_picture_t* pPic_in = (x264_picture_t*)malloc(sizeof(x264_picture_t));
    x264_picture_alloc(pPic_in, csp, pParam->i_width, pParam->i_height);

    int iNal = 0;
    x264_nal_t* pNals = NULL;

    int y_size = pParam->i_width * pParam->i_height;
    if (frame_num == 0) {
        fseek(fp_src, 0, SEEK_END);
        switch (csp) {
        case X264_CSP_I420:
            frame_num = ftell(fp_src) / (y_size * 3 / 2);
            break;
        case X264_CSP_I444:
            frame_num = ftell(fp_src) / (y_size * 3);
            break;
        default:
            printf("Color space not support\n");
            return -1;
        }
        fseek(fp_src, 0, SEEK_SET);
    }

    int frame_idx = 0;

    for (int i = 0; i < frame_num; ++i) {
        switch (csp) {
        case X264_CSP_I444:
            fread(pPic_in->img.plane[0], y_size, 1, fp_src);
            fread(pPic_in->img.plane[1], y_size, 1, fp_src);
            fread(pPic_in->img.plane[2], y_size, 1, fp_src);
            break;
        case X264_CSP_I420:
            fread(pPic_in->img.plane[0], y_size, 1, fp_src);
            fread(pPic_in->img.plane[1], y_size >> 2, 1, fp_src);
            fread(pPic_in->img.plane[2], y_size >> 2, 1, fp_src);
            break;
        default:
            printf("Colorspace Not Supported.\n");
            return -1;
        }
        pPic_in->i_pts = i;

        int ret = x264_encoder_encode(pHandle, &pNals, &iNal, pPic_in, pPic_out);
        if (ret < 0) {
            printf("Error.\n");
            return -1;
        }

        if (ret > 0) {
            printf("Succeed encode frame: %d\n", frame_idx);
            ++frame_idx;
            for (int j = 0; j < iNal; ++j) {
                fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
            }
        }
    }

    while (1) {
        int ret = x264_encoder_encode(pHandle, &pNals, &iNal, NULL, pPic_out);
        if (ret == 0)
            continue;
        if (ret > 0) {
            printf("Flush 1 frame: frame_idx = %d\n", frame_idx);
            for (int j = 0; j < iNal; ++j)
                fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
            ++frame_idx;
            if (frame_idx == frame_num)
                break;
        }
    }

    x264_picture_clean(pPic_in);
    x264_encoder_close(pHandle);
    pHandle = NULL;

    free(pPic_in);
    free(pPic_out);
    free(pParam);

    fclose(fp_src);
    fclose(fp_dst);

    return 0;
}
