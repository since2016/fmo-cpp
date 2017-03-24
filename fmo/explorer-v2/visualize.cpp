#include "../include-opencv.hpp"
#include "explorer.hpp"
#include <fmo/processing.hpp>

namespace fmo {
    namespace {
        // inline cv::Point toCv(Pos p) { return {p.x, p.y}; }
        const cv::Scalar inactiveStripsColor{0x20, 0x20, 0x20};
        const cv::Scalar stripsColor{0xC0, 0x00, 0x00};
        const cv::Scalar rejectedStripsColor{0x00, 0x00, 0xC0};
        const cv::Scalar trajectoriesColor{0x00, 0xC0, 0xC0};
        const cv::Scalar rejectedColor{0x80, 0x80, 0x80};
        const cv::Scalar acceptedColor{0xC0, 0x00, 0x00};
    }

    void ExplorerV2::visualize() {
        // cover the visualization image with the latest input image
        copy(mSourceLevel.image1, mCache.visColor, Format::BGR);
        cv::Mat result = mCache.visColor.wrap();

        // scale the current diff to source size
        {
            mCache.visDiffGray.resize(Format::GRAY, mCfg.dims);
            cv::Size cvSize{mCfg.dims.width, mCfg.dims.height};
            cv::resize(mLevel.preprocessed.wrap(), mCache.visDiffGray.wrap(), cvSize, 0, 0,
                       cv::INTER_NEAREST);
            copy(mCache.visDiffGray, mCache.visDiffColor, Format::BGR);
        }

        // blend diff with input image
        cv::addWeighted(mCache.visDiffColor.wrap(), 0.5, result, 0.5, 0, result);

        // draw strips
        int halfWidth = mLevel.step / 2;
        {
            auto strip = begin(mStrips);
            for (int i = 0; i < mLevel.numStrips; i++, strip++) {
                cv::Point p1{strip->pos.x - halfWidth, strip->pos.y - strip->halfHeight};
                cv::Point p2{strip->pos.x + halfWidth, strip->pos.y + strip->halfHeight};
                cv::rectangle(result, p1, p2, inactiveStripsColor);
            }
        }

        // draw clusters
        for (auto& cluster : mClusters) {
            const cv::Scalar* color = &stripsColor;

            if (cluster.isInvalid()) {
                if (cluster.whyInvalid() == Cluster::TOO_FEW_STRIPS) {
                    // draw clusters with too few strips with a special color
                    color = &rejectedStripsColor;
                }
                else {
                    // don't draw other kinds of invalid clusters
                    continue;
                }
            }

            auto* strip = &mStrips[cluster.l.strip];
            while (true) {
                // draw strip
                {
                    cv::Point p1{strip->pos.x - halfWidth, strip->pos.y - strip->halfHeight};
                    cv::Point p2{strip->pos.x + halfWidth, strip->pos.y + strip->halfHeight};
                    cv::rectangle(result, p1, p2, *color);
                }

                if (strip->special == Strip::END) {
                    break;
                } else {
                    auto* next = &mStrips[strip->special];

                    // draw an interconnection if needed
                    if (!Strip::inContact(*strip, *next, mLevel.step)) {
                        cv::Point p1{strip->pos.x + halfWidth, strip->pos.y};
                        cv::Point p2{next->pos.x - halfWidth, next->pos.y};
                        cv::line(result, p1, p2, trajectoriesColor);
                    }

                    strip = next;
                }
            }
        }

        // // draw trajectories
        // for (auto& traj : mTrajectories) {
        //     Component* comp = &mComponents[traj.first];
        //
        //     // connect components in the trajectory with lines
        //     while (comp->next != Component::NO_COMPONENT) {
        //         Component* next = &mComponents[comp->next];
        //         Strip& s1 = mStrips[comp->last];
        //         Strip& s2 = mStrips[next->first];
        //         cv::Point p1{s1.x, s1.y};
        //         cv::Point p2{s2.x, s2.y};
        //         cv::line(result, p1, p2, stripsColor);
        //         comp = next;
        //     }
        // }
        //
        // // draw rejected objects
        // for (auto* traj : mRejected) {
        //     auto bounds = findBounds(*traj);
        //     cv::rectangle(result, toCv(bounds.min), toCv(bounds.max), rejectedColor);
        // }
        //
        // // draw accepted objects
        // for (auto* traj : mObjects) {
        //     auto bounds = findBounds(*traj);
        //     cv::rectangle(result, toCv(bounds.min), toCv(bounds.max), acceptedColor);
        // }
    }
}
