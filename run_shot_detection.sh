video_path="$1"
out_dir="$2"
moviefile=$(basename "$video_path")
out_dfd_file="${out_dir}/${moviefile}.dfd"
./ShotDetect "$video_path" "$out_dfd_file"
python mat_index.py --video_fname "$video_path" --base_dir "$out_dir"
python dfd_to_videoevents.py --base_dir "$out_dir" --dfd_path "$out_dfd_file"
python dump_frames_of_shot.py --base_dir "$out_dir" --video_fname "$video_path"
