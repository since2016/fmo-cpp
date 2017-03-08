#include "explorer-impl.hpp"
#include "include-opencv.hpp"

namespace fmo {
    namespace {
        inline cv::Point toCv(Pos p) { return {p.x, p.y}; }
    }

    void Explorer::Impl::visualize() {
        mVisCache.resize(Format::GRAY, mCfg.dims);
        mVisualized.resize(Format::BGR, mCfg.dims);
        cv::Mat cache = mVisCache.wrap();
        cv::Mat result = mVisualized.wrap();

        if (!mIgnoredLevels.empty()) {
            // cover the visualization image with the highest-resolution difference image
            cv::resize(mIgnoredLevels[0].image.wrap(), cache,
                       cv::Size{mCfg.dims.width, mCfg.dims.height}, 0, 0, cv::INTER_NEAREST);
        } else {
            cv::resize(mLevel.image1.wrap(), cache, cv::Size{mCfg.dims.width, mCfg.dims.height}, 0,
                       0, cv::INTER_NEAREST);
        }

        // convert to color
        cv::cvtColor(cache, result, cv::COLOR_GRAY2BGR);

        // draw strips
        auto kpIt = begin(mStrips);
        {
            auto& level = mLevel;
            int halfWidth = level.step / 2;
            for (int i = 0; i < level.numStrips; i++, kpIt++) {
                auto kp = *kpIt;
                cv::Point p1{kp.x - halfWidth, kp.y - kp.halfHeight};
                cv::Point p2{kp.x + halfWidth, kp.y + kp.halfHeight};
                cv::rectangle(result, p1, p2, cv::Scalar(0xFF, 0x88, 0x88));
            }
        }

        // draw trajectories
        for (auto& traj : mTrajectories) {
            Component* comp = &mComponents[traj.first];

            // connect components in the trajectory with lines
            while (comp->next != Component::NO_COMPONENT) {
                Component* next = &mComponents[comp->next];
                Strip& s1 = mStrips[comp->last];
                Strip& s2 = mStrips[next->first];
                cv::Point p1{s1.x, s1.y};
                cv::Point p2{s2.x, s2.y};
                cv::line(result, p1, p2, cv::Scalar(0xFF, 0x88, 0x88));
                comp = next;
            }
        }

        // draw rejected objects
        for (auto* traj : mRejected) {
            auto bounds = findBounds(*traj);
            cv::rectangle(result, toCv(bounds.min), toCv(bounds.max), cv::Scalar(0x00, 0x00, 0x00));
        }

        // draw accepted objects
        for (auto* traj : mObjects) {
            auto bounds = findBounds(*traj);
            cv::rectangle(result, toCv(bounds.min), toCv(bounds.max), cv::Scalar(0x00, 0x00, 0xFF));
        }
    }
}
