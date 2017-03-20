#include "../include-opencv.hpp"
#include "explorer.hpp"
#include <fmo/processing.hpp>

namespace fmo {
    void ExplorerV1::createLevelPyramid(Image& input) {
        const Mat* prevLevelImage;

        {
            auto& level = mSourceLevel;
            level.image2.swap(level.image3);
            level.image1.swap(level.image2);
            input.swap(level.image1);
            prevLevelImage = &level.image1;
        }

        for (auto& level : mIgnoredLevels) {
            mDecimator(*prevLevelImage, level.image);
            prevLevelImage = &level.image;
        }

        {
            auto& level = mLevel;
            level.image2.swap(level.image3);
            level.image1.swap(level.image2);
            mDecimator(*prevLevelImage, level.image1);
            prevLevelImage = &level.image1;
        }
    }

    void ExplorerV1::preprocess() { preprocess(mLevel); }

    void ExplorerV1::preprocess(ProcessedLevel& level) {
        // calculate difference image
        if (mFrameNum >= 2) {
            level.diff1.swap(level.diff2);
            fmo::absdiff(level.image1, level.image2, level.diff1);
            fmo::greater_than(level.diff1, level.diff1, DIFF_THRESH);
        }

        // combine difference images to create the preprocessed image
        if (mFrameNum >= 3) {
            cv::Mat diff1Mat = level.diff1.wrap();
            cv::Mat diff2Mat = level.diff2.wrap();
            cv::Mat preprocessedMat = level.preprocessed.wrap();
            cv::bitwise_or(diff1Mat, diff2Mat, preprocessedMat);
        }
    }
}
