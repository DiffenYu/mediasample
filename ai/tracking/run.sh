#!/bin/bash -ex
#You can download the test clip from https://github.com/spmallick/learnopencv/tree/master/tracking/videos
#type: tracker types: BOOSTING, MIL, KCF, TLD, MEDIANFLOW, GOTURN, MOSSE, CSRT
./build/tracking --input="./videos/chaplin.mp4" --type="BOOSTING"
