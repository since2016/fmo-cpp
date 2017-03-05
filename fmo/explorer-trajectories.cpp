#include "explorer-impl.hpp"
#include <algorithm>
#include <cmath>
#include <fmo/algebra.hpp>

namespace fmo {
    void Explorer::Impl::findTrajectories() {
        mTrajectories.clear();

        int numComponents = int(mComponents.size());
        // not tested: it is assumed that the components are sorted by the x coordinate of the
        // leftmost strip

        for (int i = 0; i < numComponents; i++) {
            Component& me = mComponents[i];

            if (me.trajectory == Component::NO_TRAJECTORY) {
                // if the component does not have a trajectory yet, add a new one
                me.trajectory = int16_t(mTrajectories.size());
                mTrajectories.emplace_back(i);
            }

            Trajectory& myTrajectory = mTrajectories[me.trajectory];
            Strip& myFirst = mStrips[me.first];
            Strip& myLast = mStrips[me.last];
            int myWidth = myLast.x - myFirst.x;
            myTrajectory.maxWidth = int16_t(std::max(int(myTrajectory.maxWidth), myWidth));

            me.next = Component::NO_COMPONENT;
            for (int j = i + 1; j < numComponents; j++) {
                Component& candidate = mComponents[j];
                Strip& candFirst = mStrips[candidate.first];

                // condition: candidate must not be farther than max component width so far
                int dx = candFirst.x - myLast.x;
                if (dx > myTrajectory.maxWidth) break; // sorted by x => may end loop

                // condition: candidate must not be part of another trajectory
                if (candidate.trajectory != Component::NO_TRAJECTORY) continue;

                // condition: candidate must begin after this component has ended
                // condition: angle must not exceed ~63 degrees
                int dy = fmo::abs(candFirst.y - myLast.y);
                if (dy > 2 * dx) continue;

                // condition: candidate must have a consistent approximate height
                if (me.approxHalfHeight > 2 * candidate.approxHalfHeight ||
                    candidate.approxHalfHeight > 2 * me.approxHalfHeight)
                    continue;

                candidate.trajectory = me.trajectory;
                me.next = int16_t(j);
                break;
            }
        }
    }

    void Explorer::Impl::analyzeTrajectories() {
        for (auto& traj : mTrajectories) {
            // iterate over components, sum strips, find last component
            int numStrips = 0;
            Component* firstComp = &mComponents[traj.first];
            int lastIndex = traj.first;
            Component* lastComp = firstComp;
            while (true) {
                numStrips += lastComp->numStrips;
                if (lastComp->next == Component::NO_COMPONENT) break;
                lastIndex = lastComp->next;
                lastComp = &mComponents[lastIndex];
            }
            numStrips += lastComp->numStrips;

            // check that there's at least MIN_STRIPS strips
            if (numStrips < MIN_STRIPS) {
                traj.score = 0;
                continue;
            }

            // measure the distance from the first to the last strip and use it as score
            Strip& firstStrip = mStrips[firstComp->first];
            Strip& lastStrip = mStrips[lastComp->last];
            int dx = lastStrip.x - firstStrip.x;
            int dy = lastStrip.y - firstStrip.y;
            float dist = std::sqrt(float(dx * dx + dy * dy));
            traj.last = int16_t(lastIndex);
            traj.score = dist;
        }
    }
}
