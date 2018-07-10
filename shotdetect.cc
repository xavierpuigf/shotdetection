#include <iostream>
#include <fstream>
#include <memory>
#include <limits>
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
//#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include "ffmpegvideosource.hpp"
#include "programoptions.hpp"
using namespace std;




static inline unsigned int block_diff(const uchar* A, int Astep,
                                      const uchar* B, int Bstep, int block_size)
{
    unsigned int d = 0;
    int x;
    for (int row = 0; row < block_size; ++row, A += Astep, B += Bstep) {
        for (x = 0; x <= block_size - 4; x += 4)
            d += std::abs(A[x] - B[x])     + std::abs(A[x+1] - B[x+1]) +
                 std::abs(A[x+2] - B[x+2]) + std::abs(A[x+3] - B[x+3]);
        for (; x < block_size; ++x)
            d += std::abs(A[x] - B[x]);
    }
    return d;
}

int dfd(const cv::Mat &prev, const cv::Mat &current,
           int block_size, int block_step, int search_range)
{
    if (prev.size() != current.size())
        throw std::runtime_error("Input images must be of the same size");

    // TODO check type
    int Astep = current.step;
    int Bstep = prev.step;
    int diff = 0;

    int num_blocks = 0;

    #ifdef _OPENMP
    #pragma omp parallel for reduction(+ : diff)
    #endif
    for (int y0 = 0; y0 <= prev.rows - block_size + 1; y0 += block_step) {
        for (int x0 = 0; x0 <= prev.cols - block_size + 1; x0 += block_step) {
            ++num_blocks;
            const uchar *A = current.ptr(y0) + x0;
            unsigned int min_diff = std::numeric_limits<unsigned int>::max();

            for (int y = y0 - search_range; y <= y0 + search_range; ++y) {
                if (y < 0 || y + block_size > prev.rows)
                    continue;

                for (int x = x0 - search_range; x <= x0 + search_range; ++x) {
                    if (x < 0 || x + block_size > prev.cols)
                        continue;

                    const uchar *B = prev.ptr(y) + x;
                    min_diff = std::min(min_diff, block_diff(A, Astep, B, Bstep, block_size));
                }
            }
            diff += min_diff;
        }
    }

    return diff;
}

class ShotBoundaryDetector
{
public:
    virtual bool isShotBoundary(const cv::Mat &prev, const cv::Mat &current,
                                int fn_prev, int fn_current,
                                double ts_prev, double ts_current,
                                double *conf = NULL) = 0;

    virtual ~ShotBoundaryDetector() {}
};

class DFDShotBoundaryDetector : public ShotBoundaryDetector
{
public:
    DFDShotBoundaryDetector(double threshold = 2e8, int block_size = 16,
                            int block_step = -1, int search_range = -1)
        : threshold_(threshold), block_size_(block_size),
        block_step_(block_step), search_range_(search_range_)
    {
        if (block_step_ < 0)
            block_step_ = block_size_;
        if (search_range < 0)
            search_range_ = block_size_;
    }

    virtual bool isShotBoundary(const cv::Mat &prev, const cv::Mat &current,
                                int fn_prev, int fn_current,
                                double ts_prev, double ts_current,
                                double *conf = NULL)
    {
        (void) fn_prev; (void) fn_current; (void) ts_prev; (void) ts_current;

        double d = dfd(prev, current, block_size_, block_step_, search_range_);
        d /= (prev.cols * prev.rows);

        if (conf != NULL)
            *conf = d;
        return d > threshold_;
    }

    virtual ~DFDShotBoundaryDetector() {}

private:
    double threshold_;
    int block_size_;
    int block_step_;
    int search_range_;
};

struct VideoEvent
{
    int frame_num;
};

class VideoEventList : public std::vector<VideoEvent>
{
public:
    void saveToFile(const string &filename)
    {
        // TODO
    }
};

void shotdetect(const std::string &mode,
               const std::string &input,
               const std::string &output,
               int start = -1, int end = -1, int threshold = 2e8)
{
    if (mode == "dfd") {

        FFMPEGVideoSource* vid_src = new FFMPEGVideoSource(input);
        if (!vid_src){
            char error_msg[100];
            sprintf(error_msg, "Could not open input %s", input.c_str());
            throw std::runtime_error(error_msg);

        }
        DFDShotBoundaryDetector shotdetector(threshold, 16);
        VideoEventList eventlist;

        if (start < 0)
            start = 0;
        if (end < 0)
            end = vid_src->getNumberOfFrames() - 1;

        // TODO: tests are working without this but we should include it...
        // Check why this fails
        // if (!vid_src->getFrame(start)){
        //     char error_msg[100];
        //     sprintf(error_msg, "Invalid start frame: %d", start);
        //     throw std::runtime_error(error_msg);
        // }
        vid_src->getNextFrame();
        ofstream ofstr(output.c_str());
        cv::Mat prev, current;
        cv::cvtColor(vid_src->getImage(), current, cv::COLOR_RGB2GRAY);
        prev = current;
        while (vid_src->getNextFrame() && vid_src->getFrameNumber() <= end) {
            prev = current.clone();
            cv::Mat inp_image = vid_src->getImage();
            cv::cvtColor(inp_image, current, cv::COLOR_RGB2GRAY);
            double diff = -1;
            bool sb = shotdetector.isShotBoundary(prev, current, -1, -1, -1, -1, &diff);
            int fn = vid_src->getFrameNumber()-1;
            stringstream ss;
            ss << fn;
            string img_name = "frames/img"+string(ss.str())+".jpg";
            cv::imwrite(img_name, current);
            ofstr << vid_src->getFrameNumber()-1 << " " << diff << endl;
            cout << vid_src->getFrameNumber()-1 << "/" << end << " " << vid_src->getTimestamp() << " " << diff << endl;

            if (sb) {
                VideoEvent ve;
                ve.frame_num = vid_src->getFrameNumber();
                eventlist.push_back(ve);
            }
        }

        // eventlist.saveToFile(output);
    }
}

int main(int argc, char *argv[])
{
    string opt_mode, opt_input, opt_output;
    int opt_start, opt_end, opt_threshold;

    try {
        // define command line options
        ProgramOptions *all_options = new ProgramOptions("CVHCI Shot Detection");
        all_options->input = argv[1];
        if (argc > 2){
            all_options->output = argv[2];
        }
        opt_mode = all_options->mode;
        opt_input = all_options->input;
        opt_output = all_options->output;
        opt_start = all_options->start;
        opt_end = all_options->end;
        opt_threshold = all_options->threshold;

        // bool opt_help = all_options->arguments.count("-h") || all_options->arguments.count("input") == 0;

        // if (opt_help) {
        //     cout << "CVHCI Shot Detection" << endl;
        //     cout << all_options << endl;
        //     return 0;
        // }

        // main function
        cout << "mode: " << opt_mode << endl;
        cout << "input: " << opt_input << endl;
        cout << "output: " << opt_output << endl;
        shotdetect(opt_mode, opt_input, opt_output, opt_start, opt_end, opt_threshold);
    }
    catch (std::exception &e) {
        cerr << "Caught exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}

