# Code for shot detection shotdetection
Compile using
```
cmake .
make
```

Extract .dfd file with frame features used for shot detection
```
./ShotDetection input_file.mp4 output_file.dfd
```
Create mat_index containing frame-number and timestamp
```
python mat_index.py --video_fname input_file.mp4
```
Create file with shot changes form .dfd
```
python dfd_to_videoevents.py --video_fname input_file.mp4 --dfd_path output_file.dfd
```
Dump frames for every file
```
python dump_frames_of_shot.py --video_fname input_file.mp4 --dfd_path output_file.dfd
```


## Licence and citations
The code is based on `CVHCI/Okapi` library for processing of images.
