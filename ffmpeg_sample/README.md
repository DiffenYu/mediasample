Generate screen capture test clip via CLI on MacOS platform.
```bash
ffmpeg -f avfoundation -i 1 -r 30 -s 640x480  -pix_fmt yuv420p 640x480_screen_i420.yuv
```

Generate camera capture test clip via CLI on MacOS platform.
```bash
ffmpeg -f avfoundation -s 640x480 -pix_fmt nv12 -r 30 -i 0 640x480_camera_nv12.yuv

# i420(yuv420p) is special, you need to convert from nv12 to yuv420p.
ffmpeg -s 640x480 -pix_fmt nv12 -i 640x480_camera_nv12.yuv -pix_fmt yuv420p -s 640x480 640x480_camera_i420.yuv
```
Convert clip from nv12 to bgra
```bash
ffmpeg -s 640x480 -pix_fmt nv12 -i 640x480_camera_nv12.yuv -vf format=bgra -pix_fmt bgra -f rawvideo 640x480_camera_bgra.raw
```

Play the raw yuv clip via ffplay.
```bash
ffplay -video_size 640x480 -pixel_format yuv420p 640x480_screen_i420.yuv
ffplay -video_size 640x480 -pixel_format nv12 640x480_screen_i420.yuv
```
Play the raw bgra clip via ffplay.
```bash
ffplay -f rawvideo -video_size 640x480 -pixel_format bgra 640x480_camera_bgra.raw
```


