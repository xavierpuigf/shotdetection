# Python script to convert DFD features into shot boundaries
# Replicates dfd_to_videoevents.m

import os
import sys
import numpy as np
import argparse
import skimage.morphology as morph


import pdb

params = {'diffthresh': 7,
          'filter_lengths': 11,
          'proximity': 8}


def fn_ts_mapping(movie_name):
    """Load matidx to get the mapping between frame-number to timestamp
    """

    # check matidx file is there!
    matidx_fname = os.path.join(args.base_dir, movie_name, movie_name + '.matidx')
    assert os.path.exists(matidx_fname), 'matidx file not found, cannot use timestamps!'

    # read and parse matidx file
    with open(matidx_fname, 'r') as fid:
        data = fid.readlines()
    fn_ts = {int(line.split()[0]) : float(line.split()[1]) for line in data if line.strip()}

    return fn_ts


def convert_dfds_to_video_events(dfdims, params):
    """Perform filtering, and peak finding
    """

    # convert dfds into an "image"
    dfdims = np.expand_dims(np.array(dfdims), 0)
    # line structure element
    selem = morph.rectangle(1, params['filter_lengths'])
    # modified top-hat
    ocdfd = morph.opening(morph.closing(dfdims, selem), selem)
    mtdfd = dfdims - np.minimum(ocdfd, dfdims)

    # make peaks sharper by diffing twice
    doublediff_mtdfd = np.diff(np.diff(mtdfd))
    use = np.abs(doublediff_mtdfd.squeeze())

    # find peaks
    sharpchange = np.where(use > params['diffthresh'])[0].tolist()
    changeloc = []
    k = 0
    while k < len(sharpchange):
        flag = k
        a = sharpchange[k]
        while (sharpchange[k] - a) < params['proximity']:
            k += 1
            if k == len(sharpchange):
                break

        changeloc.append(int(round(np.mean(sharpchange[flag:k]))))

    changeloc = [c + 2 for c in changeloc]
    # add one to compensate for the double diff
    # and one more to compensate for k being 0 indexed

    return changeloc


def write_video_events(movie_name, events_list):
    """Write the list of video-events to file
    """

    # get the frame-timestamp mapping
    fn_ts = fn_ts_mapping(movie_name)
    
    # prepare videvents filename
    events_fname = os.path.join(args.base_dir, movie_name, movie_name + '.videvents')
    if os.path.exists(events_fname):
        print('Shot boundary file already exists')
        sys.exit(0)

    else:
        fid = open(events_fname, 'w')

    # start writing
    fid.write('CVHCI_VIDEOEVENTS_V1\n')
    for fn in events_list:
        fid.write('%d %.3f %s\n' %(fn, fn_ts[fn], 'CUT'))

    fid.close()

parser = argparse.ArgumentParser(description='Generate videoevents')
parser.add_argument('--base_dir', type=str, help='Base directory')
parser.add_argument('--dfd_path', type=str, help='Path to dfd file')
parser.add_argument('--imdb_key', type=str, help='IMDb key')


if __name__ == '__main__':
    
    args = parser.parse_args()
    dfd_fname = args.dfd_path

    assert os.path.exists(dfd_fname), 'DFD file does not exist at ' + dfd_name

    with open(dfd_fname, 'r') as fid:
        dfd_info = fid.readlines()
    # dfd files contain: frame-number feature
    dfdims = [float(line.strip().split(' ')[1]) for line in dfd_info if line.strip()]

    print('Finding locations of shot change...', end=" ")
    changeloc = convert_dfds_to_video_events(dfdims, params)
    print('Success')

    if args.imdb_key is not None:
        movie_name = args.imdb_key
    else:
        movie_name = '.'.join(args.dfd_path.split('/')[-1].split('.')[:-1])
    print('Writing to file...', end=" ")
    write_video_events(movie_name, changeloc)
    print('Success')


