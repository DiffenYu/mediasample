#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include <fstream>
#include <iostream>
#include <nvcuvid.h>
#include <vector>

// 检查 CUDA 函数返回值的宏
#define CHECK_CU_RESULT(result)                                             \
  if (result != CUDA_SUCCESS) {                                             \
    const char *errStr;                                                     \
    cuGetErrorName(result, &errStr);                                        \
    std::cerr << "CUDA Error: " << errStr << " at " << __FILE__ << ":"      \
              << __LINE__ << std::endl;                                     \
    exit(-1);                                                               \
  }

// 检查 cuvid 函数返回值的宏
#define CHECK_CUVID_RESULT(result)                                          \
  if (result != CUDA_SUCCESS) {                                             \
    const char *errStr;                                                     \
    cuGetErrorName(result, &errStr);                                        \
    std::cerr << "cuvid Error: " << errStr << " at " << __FILE__ << ":"     \
              << __LINE__ << std::endl;                                     \
    exit(-1);                                                               \
  }

class NvidiaDecoder {
public:
  NvidiaDecoder(const std::string &inputFile, const std::string &outputFile);
  ~NvidiaDecoder();
  void Decode();

private:
  static int CUDAAPI HandleVideoSequence(void *pUserData,
                                         CUVIDEOFORMAT *pVideoFormat);
  static int CUDAAPI HandlePictureDecode(void *pUserData,
                                         CUVIDPICPARAMS *pPicParams);
  static int CUDAAPI HandlePictureDisplay(void *pUserData,
                                          CUVIDPARSERDISPINFO *pDispInfo);

  cudaVideoCodec DetectCodec(const std::string &fileName);

  CUcontext cuContext;
  CUvideodecoder decoder;
  CUvideoparser parser;

  std::string inputFile;
  std::string outputFile;
  std::ofstream yuvOutputFile;

  int videoWidth;
  int videoHeight;
};

cudaVideoCodec NvidiaDecoder::DetectCodec(const std::string& fileName) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << fileName << std::endl;
        return cudaVideoCodec_NumCodecs;
    }

    uint8_t buffer[5] = {0};
    file.read(reinterpret_cast<char*>(buffer), sizeof(buffer));

    if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x00 && buffer[3] == 0x01) {
        uint8_t nal_unit_type = (buffer[4] >> 1) & 0x3F; 
        if (nal_unit_type >= 32 && nal_unit_type <= 34) {
            std::cout << "Detected HEVC stream" << std::endl;
            file.clear();                 // 清除 EOF 或错误标志
            file.seekg(0, std::ios::beg); // 返回文件开头
            return cudaVideoCodec_HEVC;
        }
    }

    if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x00 && buffer[3] == 0x01) {
        uint8_t nal_unit_type = buffer[4] & 0x1F;
        if (nal_unit_type == 7 || nal_unit_type == 5) {
            std::cout << "Detected H.264 stream" << std::endl;
            file.clear();                 // 清除 EOF 或错误标志
            file.seekg(0, std::ios::beg); // 返回文件开头
            return cudaVideoCodec_H264;
        }
    }

    // 未能检测到协议
    file.clear();
    file.seekg(0, std::ios::beg);
    std::cerr << "Unknown codec, defaulting to NumCodecs" << std::endl;

    return cudaVideoCodec_NumCodecs;
}

NvidiaDecoder::NvidiaDecoder(const std::string &inputFile,
                             const std::string &outputFile)
    : inputFile(inputFile), outputFile(outputFile), videoWidth(0),
      videoHeight(0), decoder(nullptr), parser(nullptr) {

  CHECK_CU_RESULT(cuInit(0));

  CUdevice cuDevice;
  CHECK_CU_RESULT(cuDeviceGet(&cuDevice, 0));
  CHECK_CU_RESULT(cuCtxCreate(&cuContext, 0, cuDevice));

  yuvOutputFile.open(outputFile, std::ios::binary);
  cudaVideoCodec codecType = DetectCodec(inputFile);

  CUVIDPARSERPARAMS parserParams = {};
  parserParams.CodecType = codecType;
  parserParams.ulMaxNumDecodeSurfaces = 1;
  parserParams.ulErrorThreshold = 0;
  parserParams.pUserData = this;
  parserParams.pfnSequenceCallback = HandleVideoSequence;
  parserParams.pfnDecodePicture = HandlePictureDecode;
  parserParams.pfnDisplayPicture = HandlePictureDisplay;

  CHECK_CUVID_RESULT(cuvidCreateVideoParser(&parser, &parserParams));
}

NvidiaDecoder::~NvidiaDecoder() {
  if (decoder) {
    CHECK_CUVID_RESULT(cuvidDestroyDecoder(decoder));
  }
  if (parser) {
    CHECK_CUVID_RESULT(cuvidDestroyVideoParser(parser));
  }
  yuvOutputFile.close();
  CHECK_CU_RESULT(cuCtxDestroy(cuContext));
}

int CUDAAPI NvidiaDecoder::HandleVideoSequence(void *pUserData,
                                               CUVIDEOFORMAT *pVideoFormat) {
  std::cout << "HandleVideoSequence called" << std::endl;
  NvidiaDecoder *decoder = static_cast<NvidiaDecoder *>(pUserData);

  decoder->videoWidth = pVideoFormat->coded_width;
  decoder->videoHeight = pVideoFormat->coded_height;

  if (decoder->decoder) {
    CHECK_CUVID_RESULT(cuvidDestroyDecoder(decoder->decoder));
  }

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

  CHECK_CUVID_RESULT(cuvidCreateDecoder(&decoder->decoder, &decodeInfo));

  return 1;
}

int CUDAAPI NvidiaDecoder::HandlePictureDecode(void *pUserData,
                                               CUVIDPICPARAMS *pPicParams) {
  std::cout << "HandlePictureDecode called" << std::endl;
  NvidiaDecoder *decoder = static_cast<NvidiaDecoder *>(pUserData);
  CHECK_CUVID_RESULT(cuvidDecodePicture(decoder->decoder, pPicParams));
  return 1;
}

int CUDAAPI NvidiaDecoder::HandlePictureDisplay(
    void *pUserData, CUVIDPARSERDISPINFO *pDispInfo) {
  std::cout << "HandlePictureDisplay called" << std::endl;
  NvidiaDecoder *decoder = static_cast<NvidiaDecoder *>(pUserData);

  CUdeviceptr d_frame;
  unsigned int pitch;
  CUVIDPROCPARAMS videoProcessingParams = {};
  CHECK_CUVID_RESULT(cuvidMapVideoFrame(decoder->decoder, pDispInfo->picture_index, &d_frame,
                     &pitch, &videoProcessingParams));

  unsigned char *pFrame =
      new unsigned char[pitch * decoder->videoHeight * 3 /
                        2];

  CUDA_MEMCPY2D copyParams = {};
  copyParams.srcMemoryType = CU_MEMORYTYPE_DEVICE;
  copyParams.srcDevice = d_frame;
  copyParams.srcPitch = pitch;
  copyParams.dstMemoryType = CU_MEMORYTYPE_HOST;
  copyParams.dstHost = pFrame;
  copyParams.dstPitch = pitch;
  copyParams.WidthInBytes = decoder->videoWidth;
  copyParams.Height = decoder->videoHeight;

  CHECK_CU_RESULT(cuMemcpy2D(&copyParams));

  copyParams.srcDevice =
      d_frame + decoder->videoHeight * pitch;
  copyParams.dstHost = pFrame + decoder->videoHeight * pitch;
  copyParams.Height = decoder->videoHeight / 2;

  CHECK_CU_RESULT(cuMemcpy2D(&copyParams));

  for (int i = 0; i < decoder->videoHeight; ++i) {
    decoder->yuvOutputFile.write(
        reinterpret_cast<const char *>(pFrame + i * pitch),
        decoder->videoWidth);
  }

  for (int i = 0; i < decoder->videoHeight / 2; ++i) {
    decoder->yuvOutputFile.write(
        reinterpret_cast<const char *>(pFrame + (decoder->videoHeight * pitch) +
                                       i * pitch),
        decoder->videoWidth);
  }

  std::cout << decoder->videoHeight << "x" << decoder->videoWidth
            << ", pitch:" << pitch << std::endl;

  CHECK_CUVID_RESULT(cuvidUnmapVideoFrame(decoder->decoder, d_frame));
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
  while (inputFileStream.read(reinterpret_cast<char *>(buffer.data()),
                              buffer.size()) ||
         inputFileStream.gcount()) {
    CUVIDSOURCEDATAPACKET packet = {};
    packet.payload_size = inputFileStream.gcount();
    packet.payload = buffer.data();
    packet.flags = CUVID_PKT_ENDOFSTREAM;

    std::cout << "Parsing packet with size: " << packet.payload_size << std::endl;
    CHECK_CUVID_RESULT(cuvidParseVideoData(parser, &packet));
  }
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>"
              << std::endl;
    return -1;
  }

  NvidiaDecoder decoder(argv[1], argv[2]);
  decoder.Decode();

  return 0;
}