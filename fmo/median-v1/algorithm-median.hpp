#ifndef FMO_ALGORITHM_MEDIAN_V1_HPP
#define FMO_ALGORITHM_MEDIAN_V1_HPP

#include <fmo/agglomerator.hpp>
#include <fmo/algebra.hpp>
#include <fmo/algorithm.hpp>
#include <fmo/decimator.hpp>
#include <fmo/stats.hpp>
#include <fmo/strip.hpp>

namespace fmo {
    struct MedianV1 final : public Algorithm {
        virtual ~MedianV1() override = default;

        /// Initializes all caches. Creates as many decimation levels as needed to process images
        /// with specified dimensions. The following calls to setInputSwap() will require that the
        /// format and dimensions match the format and dimensions provided here.
        MedianV1(const Config& cfg, Format format, Dims dims);

        /// To be called every frame, providing the next image for processing. The processing will
        /// take place during the call and might take a long time. The input is received by swapping
        /// the contents of the provided input image with an internal buffer.
        virtual void setInputSwap(Image&) override;

        /// Visualizes the result of detection, returning an image that is useful for debugging
        /// algorithm behavior. The returned image will have BGR format and the same dimensions as
        /// the input image.
        virtual const Image& getDebugImage() override;

        /// Determines whether a new object has been found as a result of analyzing the last frame
        /// during a call to setInputSwap(). When this method returns true, the methods
        /// getObjectBounds() and getObjectDetails() may be called to get more information about the
        /// detected object.
        virtual bool haveObject() const override;

        /// Provides the bounding box that encloses the detected object. Use the haveObject() method
        /// first to check if an object has been detected in this frame.
        virtual Bounds getObjectBounds() const override { return Bounds{}; }

        /// Provides detailed information about the detected object, including a list of object
        /// pixels. Use the haveObject() method first to check if an object has been detected in
        /// this frame.
        virtual void getObjectDetails(ObjectDetails&) const override;

    private:
        // structures

        /// Special values used instead of indices.
        enum Special : int16_t {
            UNTOUCHED = 0, ///< not processed
            TOUCHED = 1,   ///< processed
            END = -1,      ///< not an index, e.g. a strip is the last in its component
        };

        /// Connected component data.
        struct Component {
            enum Status : int16_t {
                NOT_PROCESSED,
                GOOD,
                TOO_FEW_STRIPS,
                SMALL_STRIP_AREA,
                WAY_TOO_LARGE,
                SMALL_ASPECT,
            };

            Component(int16_t aFirst) : first(aFirst), status(NOT_PROCESSED) {}

            int16_t first; ///< index of the first strip in component
            Status status; ///< describes the reason why a component was discarded.
        };

        /// Object data.
        struct Object {
            Pos center = {0, 0};         ///< midpoint
            Pos endL, endR;              ///< endpoints
            float area;                  ///< area of convex hull
            NormVector direction;        ///< principal direction
            float halfLen[2];            ///< half of length, [0] - principal direction
            float aspect;                ///< aspect ratio (1 or greater)
            int16_t prev = Special::END; ///< matched component from the previous frame
            bool selected = false;       ///< considered a fast-moving object?
        };

        /// A potential connection between objects from consequent frames.
        struct Match {
            float score;
            int16_t objects[2];
        };

        // methods

        /// Decimates the input image until it is below a set height; saves the source image and the
        /// decimated image.
        void swapAndDecimateInput(Image& in);

        /// Calculates the per-pixel median of the last three frames to obtain the background.
        /// Creates a binary difference image of background vs. the latest image.
        void computeBinDiff();

        /// Detects strips by iterating over the pixels in the image. Creates connected components
        /// by joining strips together.
        void findComponents();

        /// Selects interesting components and calculates their various properties.
        void findObjects();

        /// Interconnects similar components from the current frame and the previous one.
        void matchObjects();

        /// Selects the objects that appear to be fast-moving throughout the last three frames.
        void selectObjects();

        // data

        const Config mCfg; ///< configuration received upon construction

        struct {
            Image image;  ///< latest source image
            int frameNum; ///< the number of images received so far
        } mSourceLevel;

        struct {
            int pixelSizeLog2; ///< processing-level pixel size compared to source level, log2
            Image inputs[3];   ///< input images decimated to processing resolution, 0 - newest
            Image background;  ///< median of the last three inputs
            Image binDiff;     ///< binary difference image, latest image vs. background
        } mProcessingLevel;

        struct {
            std::vector<std::unique_ptr<Image>> decimated; ///< cached decimation steps
            Image inputConverted;       ///< latest processing input converted to BGR
            Image diffConverted;        ///< latest diff converted to BGR
            Image diffScaled;           ///< latest diff rescaled to source dimensions
            Image visualized;           ///< debug visualization
            std::vector<Pos16> upper;   ///< series of points at the top of a component
            std::vector<Pos16> lower;   ///< series of points at the bottom of a component
            std::vector<Pos16> temp;    ///< general points temporary
            std::vector<Match> matches; ///< for keeping scores when matching objects
        } mCache;

        Decimator mDecimator;               ///< decimation tool that handles any image format
        Differentiator mDiff;               ///< for creating the binary difference image
        StripGen mStripGen;                 ///< for finding strips in the difference image
        std::vector<Strip> mStrips;         ///< detected strips, ordered by x coordinate
        std::vector<int16_t> mNextStrip;    ///< indices of the next strip in component
        std::vector<Component> mComponents; ///< connected components
        std::vector<Object> mObjects[3];    ///< objects, 0 - newest
    };
}

#endif // FMO_ALGORITHM_MEDIAN_V1_HPP
