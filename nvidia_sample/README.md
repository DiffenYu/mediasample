# How to prepare the env
- Download Nvidia VideoCodec SDK via website: https://developer.nvidia.com/nvidia-video-codec-sdk/download, unzip it and put it in the directory you want.
- Modify the include_directories in CMakeLists.txt to the path of the VideoCodec SDK you put.
[VideoCodecSdK历史版本](https://developer.nvidia.com/video-codec-sdk-archive)

# How to config and build via vscode task
- command + shift + p, then search "Tasks: Run Task", it will show cmake and build options.
- choose cmake to do cmake config
- choose build to build the example
