#include <iostream>
#include <fstream>
#include <cuda.h>
#include <nvcuvid.h>
#include <vector>
#include <cuda_runtime_api.h>
#include <cuda_runtime.h>

class NvidiaDecoder {
public:
    NvidiaDecoder(const std::string& inputFile, const std::string& outputFile);
    ~NvidiaDecoder();
    void Decode();

private:
    static int CUDAAPI HandleVideoSequence(void* pUserData, CUVIDEOFORMAT* pVideoFormat);
    static int CUDAAPI HandlePictureDecode(void* pUserData, CUVIDPICPARAMS* pPicParams);
    static int CUDAAPI HandlePictureDisplay(void* pUserData, CUVIDPARSERDISPINFO* pDispInfo);

    CUcontext cuContext;
    CUvideodecoder decoder;
    CUvideoparser parser;

    std::string inputFile;
    std::string outputFile;
    std::ofstream yuvOutputFile;

    int videoWidth;
    int videoHeight;
};

NvidiaDecoder::NvidiaDecoder(const std::string& inputFile, const std::string& outputFile)
    : inputFile(inputFile), outputFile(outputFile), videoWidth(0), videoHeight(0) {

    cuInit(0);
    CUdevice cuDevice;
    cuDeviceGet(&cuDevice, 0);
    cuCtxCreate(&cuContext, 0, cuDevice);

    yuvOutputFile.open(outputFile, std::ios::binary);

    CUVIDPARSERPARAMS parserParams = {};
    parserParams.CodecType = cudaVideoCodec_H264;  // 支持多种格式可根据输入文件动态设置
    parserParams.ulMaxNumDecodeSurfaces = 1;
    parserParams.ulErrorThreshold = 0;
    parserParams.pUserData = this;
    parserParams.pfnSequenceCallback = HandleVideoSequence;
    parserParams.pfnDecodePicture = HandlePictureDecode;
    parserParams.pfnDisplayPicture = HandlePictureDisplay;

    cuvidCreateVideoParser(&parser, &parserParams);
}

NvidiaDecoder::~NvidiaDecoder() {
    if (decoder) {
        cuvidDestroyDecoder(decoder);
    }
    if (parser) {
        cuvidDestroyVideoParser(parser);
    }
    yuvOutputFile.close();
    cuCtxDestroy(cuContext);
}

int CUDAAPI NvidiaDecoder::HandleVideoSequence(void* pUserData, CUVIDEOFORMAT* pVideoFormat) {
    NvidiaDecoder* decoder = static_cast<NvidiaDecoder*>(pUserData);

    decoder->videoWidth = pVideoFormat->coded_width;
    decoder->videoHeight = pVideoFormat->coded_height;

    CUVIDDECODECREATEINFO decodeInfo = {};
    decodeInfo.CodecType = pVideoFormat->codec;
    decodeInfo.ulWidth = pVideoFormat->coded_width;
    decodeInfo.ulHeight = pVideoFormat->coded_height;
    decodeInfo.ulNumDecodeSurfaces = 1;
    decodeInfo.ChromaFormat = pVideoFormat->chroma_format;
    decodeInfo.OutputFormat = cudaVideoSurfaceFormat_NV12;
    decodeInfo.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave;
    decodeInfo.ulTargetWidth = decodeInfo.ulWidth;
    decodeInfo.ulTargetHeight = decodeInfo.ulHeight;
    decodeInfo.ulNumOutputSurfaces = 1;

    CUresult result = cuvidCreateDecoder(&decoder->decoder, &decodeInfo);
    if (result != CUDA_SUCCESS) {
        std::cerr << "Failed to create video decoder" << std::endl;
        return 0;
    }
    return 1;
}

int CUDAAPI NvidiaDecoder::HandlePictureDecode(void* pUserData, CUVIDPICPARAMS* pPicParams) {
    NvidiaDecoder* decoder = static_cast<NvidiaDecoder*>(pUserData);
    cuvidDecodePicture(decoder->decoder, pPicParams);
    return 1;
}

int CUDAAPI NvidiaDecoder::HandlePictureDisplay(void* pUserData, CUVIDPARSERDISPINFO* pDispInfo) {
    NvidiaDecoder* decoder = static_cast<NvidiaDecoder*>(pUserData);

    CUdeviceptr d_frame;
    unsigned int pitch;
    CUVIDPROCPARAMS videoProcessingParams = {};
    cuvidMapVideoFrame(decoder->decoder, pDispInfo->picture_index, &d_frame, &pitch, &videoProcessingParams);

    // 分配 YUV NV12 格式的主机内存 (Y 平面 + UV 平面)
    unsigned char* pFrame = new unsigned char[pitch * decoder->videoHeight * 3 / 2]; // NV12 包含 1.5 倍的高度 (Y + UV)

    // 拷贝 Y 平面
    CUDA_MEMCPY2D copyParams = {};
    copyParams.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    copyParams.srcDevice = d_frame;
    copyParams.srcPitch = pitch;
    copyParams.dstMemoryType = CU_MEMORYTYPE_HOST;
    copyParams.dstHost = pFrame;
    copyParams.dstPitch = pitch;
    copyParams.WidthInBytes = decoder->videoWidth;
    copyParams.Height = decoder->videoHeight;

    // 拷贝 Y 平面
    cuMemcpy2D(&copyParams);

    // 拷贝 UV 平面 (高度为 videoHeight / 2)
    copyParams.srcDevice = d_frame + decoder->videoHeight * pitch; // UV 平面起始地址
    copyParams.dstHost = pFrame + decoder->videoHeight * pitch;
    copyParams.Height = decoder->videoHeight / 2; // UV 平面高度

    // 拷贝 UV 平面
    cuMemcpy2D(&copyParams);

    // 保存 Y 平面 (YUV 文件中前 decoder->videoHeight 行)
    for (int i = 0; i < decoder->videoHeight; ++i) {
        decoder->yuvOutputFile.write(reinterpret_cast<const char*>(pFrame + i * pitch), decoder->videoWidth);
    }

    // 保存 UV 平面 (交错的 UV 数据，YUV 文件中后 decoder->videoHeight / 2 行)
    for (int i = 0; i < decoder->videoHeight / 2; ++i) {
        decoder->yuvOutputFile.write(reinterpret_cast<const char*>(pFrame + (decoder->videoHeight * pitch) + i * pitch), decoder->videoWidth);
    }

    std::cout << decoder->videoHeight << "x" << decoder->videoWidth << ", pitch:" << pitch << std::endl;

    cuvidUnmapVideoFrame(decoder->decoder, d_frame);
    delete[] pFrame;

    return 1;
}

void NvidiaDecoder::Decode() {
    std::ifstream inputFileStream(inputFile, std::ios::binary);

    if (!inputFileStream.is_open()) {
        std::cerr << "Failed to open input file" << std::endl;
        return;
    }

    std::vector<uint8_t> buffer(1024 * 1024);
    while (inputFileStream.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || inputFileStream.gcount()) {
        CUVIDSOURCEDATAPACKET packet = {};
        packet.payload_size = inputFileStream.gcount();
        packet.payload = buffer.data();
        packet.flags = CUVID_PKT_ENDOFSTREAM;

        cuvidParseVideoData(parser, &packet);
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return -1;
    }

    NvidiaDecoder decoder(argv[1], argv[2]);
    decoder.Decode();

    return 0;
}