#include <cstdint>
#include <cstring>
#include <cuda.h>
#include <cuda_runtime.h>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <nvEncodeAPI.h>
#include <vector>
#include "utils.h"

#define CUDA_API_CALL(func)                                                    \
  {                                                                            \
    CUresult status = (func);                                                  \
    if (status != CUDA_SUCCESS) {                                              \
      const char *errStr;                                                      \
      cuGetErrorName(status, &errStr);                                         \
      std::cerr << "CUDA API call failed at " << __FILE__ << ":" << __LINE__   \
                << " with error code " << status << " (" << errStr << ")"      \
                << std::endl;                                                  \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  }

#define NVENC_API_CALL(func)                                                   \
  {                                                                            \
    NVENCSTATUS status = (func);                                               \
    if (status != NV_ENC_SUCCESS) {                                            \
      std::cerr << "NVENC API call failed at " << __FILE__ << ":" << __LINE__  \
                << " with error code " << status << " ("                       \
                << GetNVEncErrorString(status) << ")" << std::endl;            \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  }

std::string GetNVEncErrorString(NVENCSTATUS status) {
  switch (status) {
  case NV_ENC_SUCCESS:
    return "Success";
  case NV_ENC_ERR_NO_ENCODE_DEVICE:
    return "No encode device";
  case NV_ENC_ERR_UNSUPPORTED_DEVICE:
    return "Unsupported device";
  case NV_ENC_ERR_INVALID_ENCODERDEVICE:
    return "Invalid encoder device";
  case NV_ENC_ERR_INVALID_DEVICE:
    return "Invalid device";
  case NV_ENC_ERR_DEVICE_NOT_EXIST:
    return "Device does not exist";
  case NV_ENC_ERR_INVALID_PTR:
    return "Invalid pointer";
  case NV_ENC_ERR_INVALID_EVENT:
    return "Invalid event";
  case NV_ENC_ERR_INVALID_PARAM:
    return "Invalid parameter";
  case NV_ENC_ERR_INVALID_CALL:
    return "Invalid call";
  case NV_ENC_ERR_OUT_OF_MEMORY:
    return "Out of memory";
  case NV_ENC_ERR_ENCODER_NOT_INITIALIZED:
    return "Encoder not initialized";
  case NV_ENC_ERR_UNSUPPORTED_PARAM:
    return "Unsupported parameter";
  case NV_ENC_ERR_LOCK_BUSY:
    return "Lock busy";
  case NV_ENC_ERR_NOT_ENOUGH_BUFFER:
    return "Not enough buffer";
  case NV_ENC_ERR_INVALID_VERSION:
    return "Invalid version";
  case NV_ENC_ERR_MAP_FAILED:
    return "Map failed";
  case NV_ENC_ERR_NEED_MORE_INPUT:
    return "Need more input";
  case NV_ENC_ERR_ENCODER_BUSY:
    return "Encoder busy";
  case NV_ENC_ERR_EVENT_NOT_REGISTERD:
    return "Event not registered";
  case NV_ENC_ERR_GENERIC:
    return "Generic error";
  case NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY:
    return "Incompatible client key";
  case NV_ENC_ERR_UNIMPLEMENTED:
    return "Unimplemented";
  case NV_ENC_ERR_RESOURCE_NOT_REGISTERED:
    return "Resource not registered";
  case NV_ENC_ERR_RESOURCE_NOT_MAPPED:
    return "Resource not mapped";
  default:
    return "Unknown error code";
  }
}


class NvEncoder {
public:
  NvEncoder(int width, int height, NV_ENC_BUFFER_FORMAT format, GUID codec)
      : width_(width), height_(height), format_(format), codec_(codec) {
    Initialize();
  }

  ~NvEncoder() { Cleanup(); }

  void EncodeFrame(const uint8_t *frameData, std::ofstream &outputFile);
  void GetEncodedData(std::ofstream &outputFile, bool outputdelay);
  void FlushFrame(std::ofstream &outputFile);

private:
  void Initialize();
  void Cleanup();
  void AllocateBuffers();

  int width_;
  int height_;
  NV_ENC_BUFFER_FORMAT format_;
  GUID codec_;
  void *nvencHandle_ = nullptr;
  CUcontext cuContext_ = nullptr;
  NV_ENCODE_API_FUNCTION_LIST nvencFuncs_ = {0};
  NV_ENC_INITIALIZE_PARAMS initializeParams_ = {0};
  NV_ENC_CONFIG encodeConfig_ = {0};
  std::vector<void *> inputSurfaces_;
  std::vector<void *> outputBitstreams_;
  uint32_t num_buffers_ = 0;
  int32_t output_delay_ = 0;
  int32_t num_to_send_ = 0;
  int32_t num_to_get_ = 0;
};

void NvEncoder::Initialize() {
  // Initialize CUDA
  CUDA_API_CALL(cuInit(0));
  CUdevice cuDevice;
  CUDA_API_CALL(cuDeviceGet(&cuDevice, 0));
  CUDA_API_CALL(cuCtxCreate(&cuContext_, 0, cuDevice));

  // Initialize NVENC
  nvencFuncs_.version = NV_ENCODE_API_FUNCTION_LIST_VER;
  NVENC_API_CALL(NvEncodeAPICreateInstance(&nvencFuncs_));

  NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS sessionParams = {};
  sessionParams.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
  sessionParams.device = cuContext_;
  sessionParams.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
  sessionParams.apiVersion = NVENCAPI_VERSION;

  NVENC_API_CALL(
      nvencFuncs_.nvEncOpenEncodeSessionEx(&sessionParams, &nvencHandle_));

  // Initialize encoder parameters
  initializeParams_.version = NV_ENC_INITIALIZE_PARAMS_VER;
  initializeParams_.encodeGUID = codec_;
  initializeParams_.presetGUID = NV_ENC_PRESET_P3_GUID;
  initializeParams_.encodeWidth = width_;
  initializeParams_.encodeHeight = height_;
  initializeParams_.darWidth = width_;
  initializeParams_.darHeight = height_;
  initializeParams_.frameRateNum = 30;
  initializeParams_.frameRateDen = 1;
  initializeParams_.enablePTD = 1;
  initializeParams_.reportSliceOffsets = 0;
  initializeParams_.enableSubFrameWrite = 0;
  initializeParams_.maxEncodeWidth = width_;
  initializeParams_.maxEncodeHeight = height_;
  initializeParams_.tuningInfo = NV_ENC_TUNING_INFO_HIGH_QUALITY;

  NV_ENC_PRESET_CONFIG presetConfig = {NV_ENC_PRESET_CONFIG_VER,
                                       {NV_ENC_CONFIG_VER}};
  NVENC_API_CALL(nvencFuncs_.nvEncGetEncodePresetConfigEx(
      nvencHandle_, codec_, NV_ENC_PRESET_P3_GUID,
      NV_ENC_TUNING_INFO_HIGH_QUALITY, &presetConfig));
  encodeConfig_ = presetConfig.presetCfg;

  initializeParams_.encodeConfig = &encodeConfig_;
  if (codec_ == NV_ENC_CODEC_H264_GUID) {
    initializeParams_.encodeConfig->encodeCodecConfig.h264Config.idrPeriod =
        initializeParams_.encodeConfig->gopLength;
  } else if (codec_ == NV_ENC_CODEC_HEVC_GUID) {
    initializeParams_.encodeConfig->encodeCodecConfig.hevcConfig.idrPeriod =
        initializeParams_.encodeConfig->gopLength;
  }

  PrintNVEncInitializeParams(initializeParams_);

  NVENC_API_CALL(
      nvencFuncs_.nvEncInitializeEncoder(nvencHandle_, &initializeParams_));

  AllocateBuffers();
}

void NvEncoder::Cleanup() {
  for (auto &surface : inputSurfaces_) {
    NVENC_API_CALL(nvencFuncs_.nvEncDestroyInputBuffer(nvencHandle_, surface))
  }
  for (auto &bitstream : outputBitstreams_) {
    NVENC_API_CALL(
        nvencFuncs_.nvEncDestroyBitstreamBuffer(nvencHandle_, bitstream))
  }
  if (nvencHandle_) {
    NVENC_API_CALL(nvencFuncs_.nvEncDestroyEncoder(nvencHandle_));
    nvencHandle_ = nullptr;
  }
  if (cuContext_) {
    CUDA_API_CALL(cuCtxDestroy(cuContext_));
    cuContext_ = nullptr;
  }
}

void NvEncoder::AllocateBuffers() {
  num_buffers_ = encodeConfig_.frameIntervalP + encodeConfig_.rcParams.lookaheadDepth + 1;
  std::cout << "frameIntervalP: " << encodeConfig_.frameIntervalP << std::endl;
  std::cout << "lookaheadDepth: " << encodeConfig_.rcParams.lookaheadDepth << std::endl;
  output_delay_ = num_buffers_ - 1;
  std::cout << "num_buffers_: " << num_buffers_ << std::endl;
  
  for (int i = 0; i < num_buffers_; ++i) {
    NV_ENC_CREATE_INPUT_BUFFER createInputBuffer = {
        NV_ENC_CREATE_INPUT_BUFFER_VER};
    createInputBuffer.width = width_;
    createInputBuffer.height = height_;
    createInputBuffer.bufferFmt = format_;
    NVENC_API_CALL(
        nvencFuncs_.nvEncCreateInputBuffer(nvencHandle_, &createInputBuffer));
    inputSurfaces_.push_back(createInputBuffer.inputBuffer);
  }

  for (int i = 0; i < num_buffers_; ++i) {
    NV_ENC_CREATE_BITSTREAM_BUFFER createBitstreamBuffer = {
        NV_ENC_CREATE_BITSTREAM_BUFFER_VER};
    NVENC_API_CALL(nvencFuncs_.nvEncCreateBitstreamBuffer(
        nvencHandle_, &createBitstreamBuffer));
    outputBitstreams_.push_back(createBitstreamBuffer.bitstreamBuffer);
  }
}

void NvEncoder::EncodeFrame(const uint8_t *frameData,
                            std::ofstream &outputFile) {
  uint32_t index = num_to_send_ % 3;

  // Copy frame data to input buffer
  NV_ENC_LOCK_INPUT_BUFFER lockInputBuffer = {NV_ENC_LOCK_INPUT_BUFFER_VER};
  lockInputBuffer.inputBuffer = inputSurfaces_[index];
  NVENC_API_CALL(
      nvencFuncs_.nvEncLockInputBuffer(nvencHandle_, &lockInputBuffer));
  memcpy(lockInputBuffer.bufferDataPtr, frameData, width_ * height_ * 3 / 2);
  NVENC_API_CALL(nvencFuncs_.nvEncUnlockInputBuffer(
      nvencHandle_, lockInputBuffer.inputBuffer));
  std::cout << "pitch: " << lockInputBuffer.pitch << std::endl;

  // Prepare parameters and encode frame
  NV_ENC_PIC_PARAMS picParams = {};
  picParams.version = NV_ENC_PIC_PARAMS_VER;
  picParams.inputWidth = width_;
  picParams.inputHeight = height_;
  picParams.inputPitch = width_;
  picParams.bufferFmt = format_;
  picParams.inputBuffer = inputSurfaces_[index];
  picParams.outputBitstream = outputBitstreams_[index];
  picParams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;

  std::cout << "Encoding frame with parameters:" << std::endl;
  std::cout << "  Input Width: " << picParams.inputWidth << std::endl;
  std::cout << "  Input Height: " << picParams.inputHeight << std::endl;
  std::cout << "  Input Pitch: " << picParams.inputPitch << std::endl;
  std::cout << "  Buffer Format: " << picParams.bufferFmt << std::endl;
  std::cout << "  Picture Struct: " << picParams.pictureStruct << std::endl;

  NVENCSTATUS status = nvencFuncs_.nvEncEncodePicture(nvencHandle_, &picParams);
  if (status == NV_ENC_SUCCESS || status == NV_ENC_ERR_NEED_MORE_INPUT ) {
    num_to_send_++;
    // Copy encoded data to output file
    GetEncodedData(outputFile, true);

  } else {
    std::cerr << "Error: Failed to encode frame" << std::endl;
    throw std::runtime_error("Failed to encode frame");
  }
}

void NvEncoder::GetEncodedData(std::ofstream &outputFile, bool outputdelay) {
  int32_t end = outputdelay ? num_to_send_ - output_delay_ : num_to_send_;
  for (; num_to_get_ < end; num_to_get_++) {
    int index = num_to_get_ % 3;

    NV_ENC_LOCK_BITSTREAM lockBitstreamData = {};
    lockBitstreamData.version = NV_ENC_LOCK_BITSTREAM_VER;
    lockBitstreamData.doNotWait = 0;
    lockBitstreamData.outputBitstream = outputBitstreams_[index];
    NVENC_API_CALL(
        nvencFuncs_.nvEncLockBitstream(nvencHandle_, &lockBitstreamData));

    if (lockBitstreamData.bitstreamBufferPtr == nullptr) {
      std::cerr << "Error: Bitstream buffer pointer is null" << std::endl;
      throw std::runtime_error("Bitstream buffer pointer is null");
    }

    if (lockBitstreamData.bitstreamSizeInBytes == 0) {
      std::cerr << "Error: Bitstream size is zero" << std::endl;
      throw std::runtime_error("Bitstream size is zero");
    }
    // std::cout << "before write, the size: " <<
    // lockBitstreamData.bitstreamSizeInBytes << std::endl;
    std::cout << "num_to_send_: " << num_to_send_
              << " num_to_get_: " << num_to_get_  
              << " total_frames: " << num_to_get_ + 1 << std::endl;
    outputFile.write(
        reinterpret_cast<char *>(lockBitstreamData.bitstreamBufferPtr),
        lockBitstreamData.bitstreamSizeInBytes);

    if (!outputFile.good()) {
      std::cerr << "Error: Failed to write to output file" << std::endl;
      throw std::runtime_error("Failed to write to output file");
    }

    NVENC_API_CALL(nvencFuncs_.nvEncUnlockBitstream(
        nvencHandle_, lockBitstreamData.outputBitstream));
  }
}

void NvEncoder::FlushFrame(std::ofstream &outputFile) {
  // send eos
  NV_ENC_PIC_PARAMS picParams = {};
  picParams.version = NV_ENC_PIC_PARAMS_VER;
  picParams.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
  NVENC_API_CALL(nvencFuncs_.nvEncEncodePicture(nvencHandle_, &picParams))

  GetEncodedData(outputFile, false);
}

void PrintUsage() {
  std::cout << "Usage: encoder [options]\n"
            << "Options:\n"
            << "  -i, --input FILE       Input file\n"
            << "  -o, --output FILE      Output file\n"
            << "  -w, --width WIDTH      Width\n"
            << "  -h, --height HEIGHT    Height\n"
            << "  -c, --codec CODEC      Codec (h264 or hevc)\n"
            << "  -f, --format FORMAT    Format (iyuv, nv12)\n"
            << "  --help                 Show this help message\n";
}

int main(int argc, char *argv[]) {
  int width = 1920;
  int height = 1080;
  std::string inputFile;
  std::string outputFile;
  NV_ENC_BUFFER_FORMAT format = NV_ENC_BUFFER_FORMAT_NV12;
  GUID codec = NV_ENC_CODEC_H264_GUID;

  static struct option long_options[] = {
      {"input", required_argument, nullptr, 'i'},
      {"output", required_argument, nullptr, 'o'},
      {"width", required_argument, nullptr, 'w'},
      {"height", required_argument, nullptr, 'h'},
      {"codec", required_argument, nullptr, 'c'},
      {"format", required_argument, nullptr, 'f'},
      {"help", no_argument, nullptr, 0},
      {nullptr, 0, nullptr, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "i:o:w:h:c:f:", long_options,
                            nullptr)) != -1) {
    switch (opt) {
    case 'i':
      inputFile = optarg;
      break;
    case 'o':
      outputFile = optarg;
      break;
    case 'w':
      width = std::stoi(optarg);
      break;
    case 'h':
      height = std::stoi(optarg);
      break;
    case 'c':
      if (std::string(optarg) == "h264") {
        codec = NV_ENC_CODEC_H264_GUID;
      } else if (std::string(optarg) == "hevc") {
        codec = NV_ENC_CODEC_HEVC_GUID;
      } else {
        std::cerr << "Unsupported codec: " << optarg << std::endl;
        return EXIT_FAILURE;
      }
      break;
    case 'f':
      if (std::string(optarg) == "iyuv") {
        format = NV_ENC_BUFFER_FORMAT_IYUV;
      } else if (std::string(optarg) == "nv12") {
        format = NV_ENC_BUFFER_FORMAT_NV12;
      } else {
        std::cerr << "Unsupported format: " << optarg << std::endl;
        return EXIT_FAILURE;
      }
      break;
    case 0:
      PrintUsage();
      return EXIT_SUCCESS;
    default:
      PrintUsage();
      return EXIT_FAILURE;
    }
  }

  if (inputFile.empty() || outputFile.empty()) {
    std::cerr << "Input and output files are required." << std::endl;
    PrintUsage();
    return EXIT_FAILURE;
  }

  std::ifstream inputFileStream(inputFile, std::ios::binary);
  std::ofstream outputFileStream(outputFile, std::ios::binary);

  if (!inputFileStream) {
    std::cerr << "Failed to open input file: " << inputFile << std::endl;
    return EXIT_FAILURE;
  }
  if (!outputFileStream) {
    std::cerr << "Failed to open output file: " << outputFile << std::endl;
    return EXIT_FAILURE;
  }

  // Allocate memory for frame data
  std::vector<uint8_t> frameData(width * height * 3 / 2);

  NvEncoder encoder(width, height, format, codec);

  while (inputFileStream.read(reinterpret_cast<char *>(frameData.data()),
                              frameData.size())) {
    encoder.EncodeFrame(frameData.data(), outputFileStream);
  }
  encoder.FlushFrame(outputFileStream);

  return EXIT_SUCCESS;
}
