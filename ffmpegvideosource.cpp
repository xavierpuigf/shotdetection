#include <iostream>
#include "ffmpegvideosource.hpp"
#include <opencv2/imgproc/imgproc.hpp>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include <string>
#include <memory>
using namespace std;


double inline normalize_ts(int64_t ts, AVRational time_base)
{
    return double(ts) * av_q2d(time_base);
}

int64_t inline denormalize_ts(double ts, AVRational time_base)
{
    return int64_t(ts * time_base.den) / time_base.num;
}

double inline normalize_ts(int64_t ts, int64_t time_base)
{
    return double(ts) / time_base;
}

AVStreamConverter::AVStreamConverter(PixelFormat src_format, int width, int height)
{
    int flags = 2; // 2 was taken from an example, apparently the flag is ignored in swscale anyway
    sws_ = sws_getContext(width, height, src_format, width, height,
                      PIX_FMT_RGB24, flags, NULL, NULL, NULL);
}

AVStreamConverter::~AVStreamConverter()
{
    sws_freeContext(sws_);
}

void  AVStreamConverter::frame_to_rgb24(const AVFrame* frame, unsigned char* rgb_data,
                int rgb_data_linesize)
{
    uint8_t* dst_data[3] = {(uint8_t*)(rgb_data), NULL, NULL};
    int dst_stride[3] = {rgb_data_linesize, 0, 0};
    sws_scale(sws_, frame->data, frame->linesize, 0, frame->height,
              dst_data, dst_stride);
}


double FFMPEGVideoSource::getTimestamp(int) const
    {
        if (pint->frame_dts == (int64_t) AV_NOPTS_VALUE){
            cout << "Could not find ANY timestamp. Please implement something..." << endl;
            return -1.;
        }
        else
            return normalize_ts(pint->frame_dts, pint->stream->time_base);
    }

double FFMPEGVideoSource::getStartTimestamp()
{
    if (pint->format->start_time == (int64_t) AV_NOPTS_VALUE){
        return 0.0;

    }
    else
        return normalize_ts(pint->format->start_time, AV_TIME_BASE);
}

FFMPEGVideoSource::FFMPEGVideoSource(const std::string& filename){
    pint = new Impl;
    pint->filename = filename;
    AVDictionary* options = NULL;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    AVFormatContext* format = avformat_alloc_context();
    av_register_all();
    avcodec_register_all();


    if (avformat_open_input(&format, filename.c_str(), NULL, &options) != 0) {
        cout << "could not open file" << endl;
        return;
    }
    if (options) av_dict_free(&options);

    if (avformat_find_stream_info(format, NULL) < 0) {
        cout <<"Could not retrieve stream info from file" <<endl;
        return;
    }
    // Obtain stream info
    
    int stream_index = -1;
    for (int i = 0; i < format->nb_streams; i++){
        if (format->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            stream_index = i;
        }
        //cout << format->streams[i]->codec->codec_type << endl;
    }   
    if (stream_index == -1){
        cout << "No video stream found" << endl;
        return;
    }
    AVStream *stream = format->streams[stream_index];
    AVCodecContext *codec = stream -> codec;
    if (avcodec_open2(codec, avcodec_find_decoder(codec->codec_id), NULL) < 0) {
        cout << "Failed to open decoder for stream #"<<stream_index<<" in file"<<endl;
        return;
    }

    av_init_packet(&pint->avpkt);
    pint->stream = stream;
    pint->format = format;
    pint->stream_index = stream_index;
    pint->cur_frame = avcodec_alloc_frame();
    pint->converter = new AVStreamConverter(pint->stream->codec->pix_fmt,
                    pint->stream->codec->width, pint->stream->codec->height);
    pint->img = cv::Mat(cv::Size(codec->width, codec->height), CV_8UC3);
    cout << "Init source" << endl;
}

FFMPEGVideoSource::~FFMPEGVideoSource()
    {
        if (pint->avpkt.data)
            av_free_packet(&pint->avpkt);
        if (pint->cur_frame)
            av_free(pint->cur_frame);
        if (pint->stream)
            avcodec_close(pint->stream->codec);
        if (pint->format)

            avformat_close_input(&(pint->format));
        delete pint;
}

CvSize FFMPEGVideoSource::getSize(){
    int w = pint->stream->codec->width;
    int h = pint->stream->codec->height;
    return cvSize(w, h);
}

double FFMPEGVideoSource::getAspectRatio(){
    int w = pint->stream->codec->width;
    int h = pint->stream->codec->height; 
    return (double) w/ (double) h;
}


int FFMPEGVideoSource::getNumberOfFrames(){
    return pint->stream->nb_frames;
}


int FFMPEGVideoSource::getFrameNumber(){
    return pint->stream->codec->frame_number;
}

bool FFMPEGVideoSource::getNextFrame(){
    int got_pic = 0;
    // Free packet from last frame
    if (pint->avpkt.data){
        av_free_packet(&pint->avpkt);
    }
    int64_t timestamp = (int64_t)AV_NOPTS_VALUE;
    while (av_read_frame(pint->format, &pint->avpkt) >= 0){
        // make sure we are decoding from the right stream
        
        if (pint->avpkt.stream_index == pint->stream_index){
            
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 23, 0)
            int res = avcodec_decode_video2(pint->stream->codec, pint->cur_frame,
                &got_pic, &pint->avpkt);
#else
            int res = avcodec_decode_video(pint->stream->codec, pint->cur_frame,
                &got_pic, pint->avpkt.data, pint->avpkt.size);
#endif
            if (res < 0){
                std::cerr << "ERROR: Error decoding video frame!" << std::endl;
                return false;
            }
                
            if (got_pic) {
                timestamp = pint->avpkt.dts;
                break;
            }
            
        }
        
        av_free_packet(&pint->avpkt);
    }
    if (!got_pic and (pint->stream->codec->codec->capabilities & CODEC_CAP_DELAY)){
        AVPacket avpkt;
        av_init_packet(&avpkt);
        avpkt.data = 0;
        avpkt.size = 0;
        // flush decoder
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 23, 0)
        if (avcodec_decode_video2(pint->stream->codec, pint->cur_frame,
                &got_pic, &pint->avpkt) < 0)
#else
        if (avcodec_decode_video(pint->stream->codec, pint->cur_frame,
                &got_picture, pint->avpkt.data, pint->avpkt.size) < 0)
#endif
        {
            std::cerr << "ERROR: Error decoding video frame!" << std::endl;
            return false;
        }   
    }
    if (got_pic){
        if (timestamp != (int64_t)AV_NOPTS_VALUE)
        {
            pint->frame_dts = timestamp;
        }
        else
        {
            int64_t delta = denormalize_ts(1.0 / av_q2d(pint->stream->r_frame_rate), pint->stream->time_base);
            pint->frame_dts += delta;
        }
    }
    return got_pic != 0;
}

void FFMPEGVideoSource::printImage(){
    cout << pint->cur_frame->data[0] << endl;
}
cv::Mat FFMPEGVideoSource::getImage(int channel){
    pint->converter->frame_to_rgb24(pint->cur_frame, pint->img.ptr(), pint->img.step);
    return pint->img;
}

bool FFMPEGVideoSource::getFrame(int frame)
{
    double ts = getStartTimestamp() + frame / av_q2d(pint->stream->r_frame_rate);
    return getFrameAt(ts);
}

bool FFMPEGVideoSource::getFrameAt(double timestamp)
{
    int64_t seek_target = denormalize_ts(timestamp, pint->stream->time_base);
    if (av_seek_frame(pint->format, pint->stream_index, seek_target, AVSEEK_FLAG_ANY) < 0){
        cout <<"Error while seeking"<<endl;
        return false;
    }
    avcodec_flush_buffers(pint->stream->codec);
    return getNextFrame();
}

        // double  getLength(); not needed
        
           
           