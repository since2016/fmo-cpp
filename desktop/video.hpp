#ifndef FMO_DESKTOP_VIDEO_HPP
#define FMO_DESKTOP_VIDEO_HPP

#include <fmo/common.hpp>
#include <fmo/region.hpp>
#include <memory>
#include <string>

namespace cv {
    class Mat;
    class VideoCapture;
    class VideoWriter;
}

struct VideoInput {
    VideoInput() = delete;
    VideoInput(const VideoInput&) = delete;
    VideoInput& operator=(const VideoInput&) = delete;
    VideoInput(VideoInput&&);
    VideoInput& operator=(VideoInput&&);

    static VideoInput makeFromCamera(int camId);
    static VideoInput makeFromFile(const std::string& filename);

    fmo::Dims dims() const { return mDims; }
    float fps() const { return mFps; }
    fmo::Region receiveFrame();

private:
    VideoInput(std::unique_ptr<cv::VideoCapture>&& cap);

    // data
    std::unique_ptr<cv::Mat> mMat;
    std::unique_ptr<cv::VideoCapture> mCap;
    fmo::Dims mDims;
    float mFps;
};

struct VideoOutput {
    VideoOutput() = delete;
    VideoOutput(const VideoOutput&) = delete;
    VideoOutput& operator=(const VideoOutput&) = delete;
    VideoOutput(VideoOutput&&);
    VideoOutput& operator=(VideoOutput&&);

    static VideoOutput makeFile(const std::string& filename, fmo::Dims dims, float fps);
    static VideoOutput makeInDirectory(const std::string& dir, fmo::Dims dims, float fps);

    void sendFrame(const fmo::Mat& frame);

private:
    VideoOutput(std::unique_ptr<cv::VideoWriter>&& writer, fmo::Dims dims);

    // data
    std::unique_ptr<cv::VideoWriter> mWriter;
    fmo::Dims mDims;
};

#endif // FMO_DESKTOP_VIDEO_HPP