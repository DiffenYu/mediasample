#!/bin/bash -ex
# video file input
# You can download the test video clip from https://github.com/spmallick/learnopencv/tree/master/OpenPose
# You need also download the pose network
./build/yolo3detection --input=./sample_video.mp4 --num_frames=10 --show=true 
#./build/yolo3detection --input=./run.mp4 --num_frames=10 --show=true 
# camera input & show 
#./build/OpenPoseVideo --mode=gpu --cameraid=0 --num_frames=1000 --show=true --width=656 --height=368
