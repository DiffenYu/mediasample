# How to build the video codec sdk
- build ffmpeg in mediasample
- export the LD_LIBRARY_PATH to the ffmpeg lib path

install the ffmpeg and dev via apt
```
sudo apt install ffmpeg libavcodec-dev libavformat-dev libavutil-dev
```

# How to run the gl related sample on linux
- add prefix refer to https://download.nvidia.com/XFree86/Linux-x86_64/435.17/README/primerenderoffload.html
```
__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./AppDecGL -i xxx -gpu 0
```

