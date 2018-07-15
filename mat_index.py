# Python script to create the index containing frame-number and timestamp
import os
import sys
import argparse
# PyAV - wrapper for FFMPEG
import av
# Local imports
from config import PROC_BASE


def process(video_fname, imdb_key):
    """Generate the matidx file.
    """

    # create mat-idx template
    matidx_fname = os.path.join(PROC_BASE, imdb_key, imdb_key + '.matidx')
    if os.path.exists(matidx_fname):
        print 'matidx for %s already exists' %imdb_key
        return
    else:
        # check directory exists
        out_dir = os.path.join(PROC_BASE, imdb_key)
        if not os.path.isdir(out_dir):
            os.makedirs(out_dir)
        fid = open(matidx_fname, 'w')

    # open the video
    try:
        container = av.open(video_fname)
    except:
        RuntimeError('Failed to open the video!')

    # video stream time-base
    v_time_base = container.streams.video[0].time_base
    real_v_time_base = 1.0 * v_time_base.numerator / v_time_base.denominator

    # run for each frame, and get info
    for fn, frame in enumerate(container.decode(video=0)):
        ts = frame.pts * real_v_time_base
        fid.write('%d %.3f\n' %(fn, ts))

    fid.close()
    print 'Completed writing to', matidx_fname


parser = argparse.ArgumentParser(description='Process video file inputs')
parser.add_argument('video_fname', type=str, help='Video file path')
parser.add_argument('imdb_key', type=str, help='IMDb key')

if __name__ == '__main__':
    args = parser.parse_args()
    assert args.imdb_key.startswith('tt'), 'Invalid IMDb key'

    print 'Running for %s\n%s' %(args.imdb_key, args.video_fname)
    process(args.video_fname, args.imdb_key)

