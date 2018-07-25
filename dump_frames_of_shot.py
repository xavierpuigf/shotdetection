import os
import pdb
import argparse
import numpy as np
# wrapper for FFMPEG through OpenCV
import cv2
# Local imports


def read_videvents(fname):
    """Read and parse where shot boundaries occur
    """

    assert os.path.exists(fname), 'Shot boundaries file does not exist!'

    # read events file
    with open(fname, 'r') as fid:
        events = fid.readlines()
    # parse, starting from second line and get all frame-numbers
    events = [int(line.split()[0]) for line in events[1:] if line.strip()]

    return events


def main(video_name, video_fname):
    """Load videvents, convert, loop through frames, and write images to disk, etc.
    """

    # get list of events
    videvents = read_videvents(os.path.join(args.base_dir, video_name, video_name + '.videvents'))
    # open the video
    video = cv2.VideoCapture(video_fname)
    assert video.isOpened(), 'Failed to open the video!'

    # create a list of frames to write
    frames_to_write = {'begin': [], 'mid': [], 'end': []}

    begin = list(videvents)  # make copy
    begin.insert(0, 0)  # add the 0th frame at the first position
    frames_to_write['begin'] =  begin

    frames_to_write['end'] = (np.array(videvents) - 1).tolist()  # get last-frames of each shot
    frames_to_write['end'].append(int(video.get(cv2.CAP_PROP_FRAME_COUNT) - 1))  # add last frame of video

    # midframes
    frames_to_write['mid'] = (np.floor((np.array(frames_to_write['begin']) +  np.array(frames_to_write['end']))/2)).tolist()
    
    # create folder mappings and the folders
    dir_map = {'begin': 'start_frame', 'mid': 'mid_frame', 'end': 'last_frame'}
    for dirname in dir_map.values():
        os.makedirs(os.path.join(args.base_dir, video_name, dirname))

    # start writing frames of each shot
    print(len(frames_to_write['mid']))
    print(len(frames_to_write['begin']))
    print(len(frames_to_write['end']))
    fn = 0
    while True:
        ret, frame = video.read()
        if not ret:
            break

        for key in frames_to_write.keys():
            if fn in frames_to_write[key]:
                shot = frames_to_write[key].index(fn)
                im_filename = os.path.join(args.base_dir, video_name, dir_map[key], 'shot_%04d_fn_%06d.jpg' %(shot, fn))
                cv2.imwrite(im_filename, frame)

        fn += 1


parser = argparse.ArgumentParser(description='Process video file inputs')
parser.add_argument('--video_fname', type=str, help='Video file path')
parser.add_argument('--imdb_key', type=str, help='IMDb key')
parser.add_argument('--base_dir', type=str, help='Base directory')

if __name__ == '__main__':
    # get arguments
    args = parser.parse_args()
    if args.imdb_key is not None:
        assert args.imdb_key.startswith('tt'), 'Invalid IMDb key'
        movie_name = args.imdb_key
    else:
        movie_name = '.'.join(args.video_fname.split('/')[-1].split('.')[:-1])
    # if video_fname has .720p.mp4 at the end, look for using full-res video without the .720p
    fname = args.video_fname
    if fname.endswith('.720p.mp4') and os.path.exists(fname.replace('.720p.mp4', '.mp4')):
        fname = fname.replace('.720p.mp4', '.mp4')

    print("Working on", movie_name)
    print("Video:", fname)
    main(movie_name, fname)


