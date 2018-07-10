
#ifndef OKAPI_VIDEOIO_FFMPEGVIDEOSOURCE_HPP_
#define OKAPI_VIDEOIO_FFMPEGVIDEOSOURCE_HPP_


#include <string>
#include <memory>
#include <string>
#include <opencv2/imgproc/imgproc.hpp>
extern "C" {
 #include "libavcodec/avcodec.h"
 #include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
using namespace std;

/** \addtogroup videoio */
//@{

/** Video reader class based on FFMPEG
 *
 * \attention Note that although seeking is supported by this class,
 * depending on the video format, seeking is most porbably not accurate,
 * i.e. you will end up on a different frame than the one you specified
 * and worse, the frame numbers might not be accurate afterwards.
 *
 * \attention So if you need accurate seeking, consider using the
 * FFMPEGIndexedVideoSource class
 */


class AVStreamConverter
{
    public:

        AVStreamConverter(PixelFormat src_format, int width, int height);

        ~AVStreamConverter();

        void frame_to_rgb24(const AVFrame* frame, unsigned char* rgb_data,
                            int rgb_data_linesize);

    protected:
        struct SwsContext* sws_;
};
class FFMPEGVideoSource
{
    public:
        /** Constructor
         * \param[in] filename Filename of the video
         * \param[in] arc Mode for aspect ratio correction
         */
        FFMPEGVideoSource(const std::string& filename);
        /// Destructor
        ~FFMPEGVideoSource();
        CvSize  getSize();
        // (w/h)
        double  getAspectRatio();
        int     getChannels() { return 1; }
        bool    getNextFrame();
        cv::Mat getImage(int channel = 0);
        double  getTimestamp(int channel = 0) const;

        int     getFrameNumber();
        int     getNumberOfFrames();
        double  getStartTimestamp(); 
        // double  getLength(); not needed 
        bool    getFrame(int frame);
        bool    getFrameAt(double timestamp);
        void    printImage();

    protected:
        struct Impl {
            std::string filename;
            AVStream *stream;
            AVCodecContext *codec;
            AVPacket avpkt;
            AVFrame *cur_frame;
            AVFormatContext *format;
            AVStreamConverter *converter;
            cv::Mat img;
            int stream_index;
            int64_t frame_dts;

        };
        string ffmpegErrorStr(int result);
        Impl* pint;     ///< Private implementation details
};



    //@}


#endif
