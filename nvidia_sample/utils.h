#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <string>
#include "nvEncodeAPI.h"

void PrintH264Config(const NV_ENC_CONFIG_H264& h264Config);
void PrintHEVCConfig(const NV_ENC_CONFIG_HEVC& hevcConfig);
void PrintNVEncConfig(const NV_ENC_CONFIG& config, GUID codecGUID);
void PrintNVEncInitializeParams(const NV_ENC_INITIALIZE_PARAMS& initParams);
void PrintH264VUIParameters(const NV_ENC_CONFIG_H264_VUI_PARAMETERS& vuiParams);
void PrintHEVCVUIParameters(const NV_ENC_CONFIG_HEVC_VUI_PARAMETERS& vuiParams);

static inline bool operator==(const GUID &guid1, const GUID &guid2) {
  return !memcmp(&guid1, &guid2, sizeof(GUID));
}

static inline bool operator!=(const GUID &guid1, const GUID &guid2) {
  return !(guid1 == guid2);
}

// Define a custom hash function for GUID to use with unordered_map
struct GUIDHash {
    std::size_t operator()(const GUID& guid) const {
        const uint64_t* p = reinterpret_cast<const uint64_t*>(&guid);
        return p[0] ^ p[1];
    }
};

// Define a map to convert GUIDs to their string representations
std::unordered_map<GUID, std::string, GUIDHash> guidToStringMap = {
    {NV_ENC_CODEC_H264_GUID, "H.264"},
    {NV_ENC_CODEC_HEVC_GUID, "HEVC"},
    {NV_ENC_CODEC_AV1_GUID, "AV1"},
    {NV_ENC_PRESET_P1_GUID, "P1"},
    {NV_ENC_PRESET_P2_GUID, "P2"},
    {NV_ENC_PRESET_P3_GUID, "P3"},
    {NV_ENC_PRESET_P4_GUID, "P4"},
    {NV_ENC_PRESET_P5_GUID, "P5"},
    {NV_ENC_PRESET_P6_GUID, "P6"},
    {NV_ENC_PRESET_P7_GUID, "P6"},
    {NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID, "Auto"},
    {NV_ENC_H264_PROFILE_BASELINE_GUID, "H.264 Baseline"},
    {NV_ENC_H264_PROFILE_MAIN_GUID, "H.264 Main"},
    {NV_ENC_H264_PROFILE_HIGH_GUID, "H.264 High"},
    {NV_ENC_HEVC_PROFILE_MAIN_GUID, "HEVC Main"},
    {NV_ENC_HEVC_PROFILE_MAIN10_GUID, "HEVC Main10"},
    {NV_ENC_AV1_PROFILE_MAIN_GUID, "AV1 Main"}};

// Overload the << operator for GUID
std::ostream& operator<<(std::ostream& os, const GUID& guid) {
    auto it = guidToStringMap.find(guid);
    if (it != guidToStringMap.end()) {
        os << it->second;
    } else {
        os << "Unknown GUID";
    }
    return os;
}


void PrintNVEncInitializeParams(const NV_ENC_INITIALIZE_PARAMS& initParams) {
    std::cout << "NV_ENC_INITIALIZE_PARAMS:" << std::endl;
    std::cout << "version: " << initParams.version << std::endl;
    std::cout << "encodeGUID: " << initParams.encodeGUID << std::endl;
    std::cout << "presetGUID: " << initParams.presetGUID << std::endl;
    std::cout << "encodeWidth: " << initParams.encodeWidth << std::endl;
    std::cout << "encodeHeight: " << initParams.encodeHeight << std::endl;
    std::cout << "darWidth: " << initParams.darWidth << std::endl;
    std::cout << "darHeight: " << initParams.darHeight << std::endl;
    std::cout << "frameRateNum: " << initParams.frameRateNum << std::endl;
    std::cout << "frameRateDen: " << initParams.frameRateDen << std::endl;
    std::cout << "enableEncodeAsync: " << initParams.enableEncodeAsync << std::endl;
    std::cout << "enablePTD: " << initParams.enablePTD << std::endl;
    std::cout << "reportSliceOffsets: " << initParams.reportSliceOffsets << std::endl;
    std::cout << "enableSubFrameWrite: " << initParams.enableSubFrameWrite << std::endl;
    std::cout << "enableExternalMEHints: " << initParams.enableExternalMEHints << std::endl;
    std::cout << "enableWeightedPrediction: " << initParams.enableWeightedPrediction << std::endl;
    std::cout << "maxEncodeWidth: " << initParams.maxEncodeWidth << std::endl;
    std::cout << "maxEncodeHeight: " << initParams.maxEncodeHeight << std::endl;
    std::cout << "enableMEOnlyMode: " << initParams.enableMEOnlyMode << std::endl;
    std::cout << "enableOutputInVidmem: " << initParams.enableOutputInVidmem << std::endl;
    std::cout << "privDataSize: " << initParams.privDataSize << std::endl;
    std::cout << "privData: " << static_cast<void*>(initParams.privData) << std::endl;
    std::cout << "reserved: ";
    for (int i = 0; i < sizeof(initParams.reserved) / sizeof(initParams.reserved[0]); ++i) {
        std::cout << initParams.reserved[i] << " ";
    }
    std::cout << std::endl;

    if (initParams.encodeConfig) {
        PrintNVEncConfig(*initParams.encodeConfig, initParams.encodeGUID);
    } else {
        std::cout << "encodeConfig: nullptr" << std::endl;
    }
}

void PrintNVEncConfig(const NV_ENC_CONFIG& config, GUID codecGUID) {
    std::cout << "NV_ENC_CONFIG:" << std::endl;
    std::cout << "version: " << config.version << std::endl;
    std::cout << "profileGUID: " << config.profileGUID << std::endl;
    std::cout << "gopLength: " << config.gopLength << std::endl;
    std::cout << "frameIntervalP: " << config.frameIntervalP << std::endl;
    std::cout << "monoChromeEncoding: " << config.monoChromeEncoding << std::endl;
    std::cout << "frameFieldMode: " << config.frameFieldMode << std::endl;
    std::cout << "mvPrecision: " << config.mvPrecision << std::endl;

    std::cout << "rcParams:" << std::endl;
    std::cout << "    rateControlMode: " << config.rcParams.rateControlMode << std::endl;
    std::cout << "    maxBitRate: " << config.rcParams.maxBitRate << std::endl;
    std::cout << "    vbvBufferSize: " << config.rcParams.vbvBufferSize << std::endl;
    std::cout << "    vbvInitialDelay: " << config.rcParams.vbvInitialDelay << std::endl;
    std::cout << "    enableMinQP: " << config.rcParams.enableMinQP << std::endl;
    std::cout << "    enableMaxQP: " << config.rcParams.enableMaxQP << std::endl;
    std::cout << "    enableInitialRCQP: " << config.rcParams.enableInitialRCQP << std::endl;
    std::cout << "    enableAQ: " << config.rcParams.enableAQ << std::endl;
    std::cout << "    enableLookahead: " << config.rcParams.enableLookahead << std::endl;
    std::cout << "    enableTemporalAQ: " << config.rcParams.enableTemporalAQ << std::endl;
    std::cout << "    targetQuality: " << config.rcParams.targetQuality << std::endl;
    std::cout << "    targetQualityLSB: " << config.rcParams.targetQualityLSB << std::endl;
    std::cout << "    qpMapMode: " << config.rcParams.qpMapMode << std::endl;

    if (codecGUID == NV_ENC_CODEC_H264_GUID) {
        PrintH264Config(config.encodeCodecConfig.h264Config);
    } else if (codecGUID == NV_ENC_CODEC_HEVC_GUID) {
        PrintHEVCConfig(config.encodeCodecConfig.hevcConfig);
    }
}

void PrintH264Config(const NV_ENC_CONFIG_H264& h264Config) {
    std::cout << "H264 Configuration:" << std::endl;
    std::cout << "    idrPeriod: " << h264Config.idrPeriod << std::endl;
    std::cout << "    chromaFormatIDC: " << h264Config.chromaFormatIDC << std::endl;
    std::cout << "    maxNumRefFrames: " << h264Config.maxNumRefFrames << std::endl;
    std::cout << "    bdirectMode: " << h264Config.bdirectMode << std::endl;
    std::cout << "    entropyCodingMode: " << h264Config.entropyCodingMode << std::endl;
    std::cout << "    repeatSPSPPS: " << h264Config.repeatSPSPPS << std::endl;
    std::cout << "    enableIntraRefresh: " << h264Config.enableIntraRefresh << std::endl;
    std::cout << "    intraRefreshPeriod: " << h264Config.intraRefreshPeriod << std::endl;
    std::cout << "    intraRefreshCnt: " << h264Config.intraRefreshCnt << std::endl;
    std::cout << "    sliceMode: " << h264Config.sliceMode << std::endl;
    std::cout << "    sliceModeData: " << h264Config.sliceModeData << std::endl;
    std::cout << "    useConstrainedIntraPred: " << h264Config.useConstrainedIntraPred << std::endl;
    std::cout << "    disableDeblockingFilterIDC: " << h264Config.disableDeblockingFilterIDC << std::endl;
    std::cout << "    level: " << h264Config.level << std::endl;
    PrintH264VUIParameters(h264Config.h264VUIParameters);
}

void PrintHEVCConfig(const NV_ENC_CONFIG_HEVC& hevcConfig) {
    std::cout << "HEVC Configuration:" << std::endl;
    std::cout << "    idrPeriod: " << hevcConfig.idrPeriod << std::endl;
    std::cout << "    chromaFormatIDC: " << hevcConfig.chromaFormatIDC << std::endl;
    std::cout << "    maxNumRefFramesInDPB: " << hevcConfig.maxNumRefFramesInDPB << std::endl;
    std::cout << "    repeatSPSPPS: " << hevcConfig.repeatSPSPPS << std::endl;
    std::cout << "    enableIntraRefresh: " << hevcConfig.enableIntraRefresh << std::endl;
    std::cout << "    intraRefreshPeriod: " << hevcConfig.intraRefreshPeriod << std::endl;
    std::cout << "    intraRefreshCnt: " << hevcConfig.intraRefreshCnt << std::endl;
    std::cout << "    maxCUSize: " << hevcConfig.maxCUSize << std::endl;
    std::cout << "    minCUSize: " << hevcConfig.minCUSize << std::endl;
    std::cout << "    useConstrainedIntraPred: " << hevcConfig.useConstrainedIntraPred << std::endl;
    std::cout << "    level: " << hevcConfig.level << std::endl;
    std::cout << "    tier: " << hevcConfig.tier << std::endl;
    PrintHEVCVUIParameters(hevcConfig.hevcVUIParameters);

}

void PrintH264VUIParameters(const NV_ENC_CONFIG_H264_VUI_PARAMETERS& vuiParams) {
    std::cout << "NV_ENC_CONFIG_H264_VUI_PARAMETERS:" << std::endl;
    std::cout << "    overscanInfoPresentFlag: " << vuiParams.overscanInfoPresentFlag << std::endl;
    std::cout << "    videoSignalTypePresentFlag: " << vuiParams.videoSignalTypePresentFlag << std::endl;
    std::cout << "    videoFormat: " << static_cast<int>(vuiParams.videoFormat) << std::endl;
    std::cout << "    videoFullRangeFlag: " << vuiParams.videoFullRangeFlag << std::endl;
    std::cout << "    colourDescriptionPresentFlag: " << vuiParams.colourDescriptionPresentFlag << std::endl;
    std::cout << "    colourPrimaries: " << static_cast<int>(vuiParams.colourPrimaries) << std::endl;
    std::cout << "    transferCharacteristics: " << static_cast<int>(vuiParams.transferCharacteristics) << std::endl;
    std::cout << "    colourMatrix: " << static_cast<int>(vuiParams.colourMatrix) << std::endl;
    std::cout << "    chromaSampleLocationTop: " << vuiParams.chromaSampleLocationTop << std::endl;
    std::cout << "    chromaSampleLocationBot: " << vuiParams.chromaSampleLocationBot << std::endl;
    std::cout << "    bitstreamRestrictionFlag: " << vuiParams.bitstreamRestrictionFlag << std::endl;
}

void PrintHEVCVUIParameters(const NV_ENC_CONFIG_HEVC_VUI_PARAMETERS& vuiParams) {
    std::cout << "NV_ENC_CONFIG_HEVC_VUI_PARAMETERS:" << std::endl;
    std::cout << "    overscanInfoPresentFlag: " << vuiParams.overscanInfoPresentFlag << std::endl;
    std::cout << "    videoSignalTypePresentFlag: " << vuiParams.videoSignalTypePresentFlag << std::endl;
    std::cout << "    videoFormat: " << static_cast<int>(vuiParams.videoFormat) << std::endl;
    std::cout << "    videoFullRangeFlag: " << vuiParams.videoFullRangeFlag << std::endl;
    std::cout << "    colourDescriptionPresentFlag: " << vuiParams.colourDescriptionPresentFlag << std::endl;
    std::cout << "    colourPrimaries: " << static_cast<int>(vuiParams.colourPrimaries) << std::endl;
    std::cout << "    transferCharacteristics: " << static_cast<int>(vuiParams.transferCharacteristics) << std::endl;
    std::cout << "    colourMatrix: " << static_cast<int>(vuiParams.colourMatrix) << std::endl;
    std::cout << "    chromaSampleLocationTop: " << vuiParams.chromaSampleLocationTop << std::endl;
    std::cout << "    chromaSampleLocationBot: " << vuiParams.chromaSampleLocationBot << std::endl;
    std::cout << "    bitstreamRestrictionFlag: " << vuiParams.bitstreamRestrictionFlag << std::endl;
}
