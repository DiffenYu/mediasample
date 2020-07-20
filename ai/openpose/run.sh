#!/bin/bash -ex
# video file input
# You can download the test video clip from https://github.com/spmallick/learnopencv/tree/master/OpenPose
# You need also download the pose network
./build/openpose --mode=cpu --input=./sample_video.mp4 --num_frames=10 --show=true --width=656 --height=368
# camera input & show 
#./build/OpenPoseVideo --mode=gpu --cameraid=0 --num_frames=1000 --show=true --width=656 --height=368
