Generate screen capture test clip via CLI on MacOS platform.
```bash
ffmpeg -f avfoundation -i 1 -r 30 -s 640x480  -pix_fmt yuv420p 640x480_screen_i420.yuv
```

Generate camera capture test clip via CLI on MacOS platform.
```bash
ffmpeg -f avfoundation -s 640x480 -pix_fmt nv12 -r 30 -i 0 640x480_camera_nv12.yuv

# i420(yuv420p) is special, you need to convert from nv12 to yuv420p.
ffmpeg  -s 640x480 -pix_fmt nv12 -i 640x480_camera_nv12.yuv -pix_fmt yuv420p -s 640x480 640x480_camera_i420.yuv
```

Play the raw yuv clip via ffplay.
```bash
ffplay -s 640x480 -pix_fmt yuv420p 640x480_screen_i420.yuv
ffplay -s 640x480 -pix_fmt nv12 640x480_screen_i420.yuv
```


