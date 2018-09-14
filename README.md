# Shot detection
This repo provides code to perform video shot detection and extract middle, beginning and end frames for each shot.

## How to run
Compile using
```
cmake .
make
```

Extract the movie shots running
```
sh run_shot_detection.sh {input_movie_file} {output_folder}
```

The code will create a folder `{output_folder}/{input_movie_file}` containing the start, middle and last frame for every shot and a file `{output_folder}/{input_movie_file}/{input_movie_file}.videvents` with the frame and timestamp of every start of shot.

You can test the code using test video
```
wget http://wednesday.csail.mit.edu/frames/tools/video_tools/test_vid/short.mp4
mkdir output
sh run_shot_detection.sh short.mp4 output/
```

## How it works
Extracts the movie .dfd file, capturing the difference between motion compensated frames.
```
./ShotDetection input_file.mp4 output_file.dfd
```
Creates mat_index containing frame-number and timestamp mappings
```
python mat_index.py --video_fname input_file.mp4
```
Create file with shot changes from .dfd
```
python dfd_to_videoevents.py --video_fname input_file.mp4 --dfd_path output_file.dfd
```
Given the shot changes, dump the first middle and last frame for every shot.
```
python dump_frames_of_shot.py --video_fname input_file.mp4 --dfd_path output_file.dfd
```


## Licence and citations
The code is based on `CVHCI/Okapi` library for processing of images.
