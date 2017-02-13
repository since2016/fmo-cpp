#include <algorithm>
#include <fmo/assert.hpp>
#include <fmo/image.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace fmo {
    namespace {
        /// Get the number of bytes of data that an image requires, given its format and dimensions.
        size_t getNumBytes(Format format, Dims dims) {
            size_t result = static_cast<size_t>(dims.width) * static_cast<size_t>(dims.height);

            switch (format) {
            case Format::BGR:
                result *= 3;
                break;
            case Format::GRAY:
                break;
            case Format::YUV420SP:
                result = (result * 3) / 2;
                break;
            default:
                throw std::runtime_error("getNumBytes: unsupported format");
            }

            return result;
        }

        /// Convert the actual dimensions to the size that is used by OpenCV. OpenCV considers YUV
        /// 4:2:0 SP images 1.5x taller.
        cv::Size getCvSize(Format format, Dims dims) {
            cv::Size result{dims.width, dims.height};

            switch (format) {
            case Format::BGR:
            case Format::GRAY:
                break;
            case Format::YUV420SP:
                result.height = (result.height * 3) / 2;
                break;
            default:
                throw std::runtime_error("getCvSize: unsupported format");
            }

            return result;
        }

        /// Convert the size used by OpenCV to the actual dimensions. OpenCV considers YUV 4:2:0 SP
        /// images 1.5x taller.
        Dims getDims(Format format, cv::Size size) {
            Dims result{size.width, size.height};

            switch (format) {
            case Format::BGR:
            case Format::GRAY:
                break;
            case Format::YUV420SP:
                result.height = (result.height * 2) / 3;
                break;
            default:
                throw std::runtime_error("getImageSize: unsupported format");
            }

            return result;
        }

        /// Get the Mat data type used by OpenCV that corresponds to the format.
        int getCvType(Format format) {
            switch (format) {
            case Format::BGR:
                return CV_8UC3;
            case Format::GRAY:
            case Format::YUV420SP:
                return CV_8UC1;
            default:
                throw std::runtime_error("getCvType: unsupported format");
            }
        }
    }

    Image::Image(const std::string& filename, Format format) {
        cv::Mat mat;

        switch (format) {
        case Format::BGR:
            mat = cv::imread(filename, cv::IMREAD_COLOR);
            break;
        case Format::GRAY:
            mat = cv::imread(filename, cv::IMREAD_GRAYSCALE);
            break;
        default:
            throw std::runtime_error("reading image: unsupported format");
            break;
        }

        if (mat.data == nullptr) { throw std::runtime_error("failed to open image"); }

        FMO_ASSERT(mat.isContinuous(), "reading image: not continuous")
        FMO_ASSERT(mat.type() == getCvType(format), "reading image: unexpected mat type");
        Dims dims = getDims(format, mat.size());
        size_t bytes = mat.elemSize() * mat.total();
        FMO_ASSERT(getNumBytes(format, dims) == bytes, "reading image: unexpected size");
        mData.resize(bytes);
        std::copy(mat.data, mat.data + mData.size(), mData.data());
        mFormat = format;
        mDims = dims;
    }

    void Image::assign(Format format, Dims dims, const uint8_t* data) {
        size_t bytes = getNumBytes(format, dims);
        mData.resize(bytes);
        mDims = dims;
        mFormat = format;
        std::copy(data, data + bytes, mData.data());
    }

    void Image::resize(Format format, Dims dims) {
        size_t bytes = getNumBytes(format, dims);
        mData.resize(bytes);
        mDims = dims;
        mFormat = format;
    }

    cv::Mat Image::wrap() { return {getCvSize(mFormat, mDims), getCvType(mFormat), mData.data()}; }

    cv::Mat Image::wrap() const {
        auto* ptr = const_cast<uint8_t*>(mData.data());
        return {getCvSize(mFormat, mDims), getCvType(mFormat), ptr};
    }

    Region::Region(Format format, Pos pos, Dims dims, uint8_t* data, size_t rowStep)
        : Mat(format, dims), mPos(pos), mData(data), mRowStep(rowStep) {}

    void Region::resize(Format format, Dims dims) {
        if (dims.width > mDims.width || dims.height > mDims.height) {
            throw std::runtime_error("a region must not grow in size");
        }

        mFormat = format;
        mDims = dims;
    }

    cv::Mat Region::wrap() {
        return {getCvSize(mFormat, mDims), getCvType(mFormat), mData, mRowStep};
    }

    cv::Mat Region::wrap() const {
        return {getCvSize(mFormat, mDims), getCvType(mFormat), mData, mRowStep};
    }

    void copy(const Mat& src, Mat& dst) {
        dst.resize(src.format(), src.dims());
        cv::Mat srcMat = src.wrap();
        cv::Mat dstMat = dst.wrap();
        srcMat.copyTo(dstMat);
        FMO_ASSERT(dstMat.data == dst.data(), "copy: dst buffer reallocated");
    }

    void convert(const Mat& src, Mat& dst, Format format) {
        const auto srcFormat = src.format();
        const auto dstFormat = format;

        if (srcFormat == dstFormat) {
            // no format change -- just copy
            copy(src, dst);
            return;
        }

        if (&src == &dst) {
            if (srcFormat == dstFormat) {
                // same instance and no format change: no-op
                return;
            }

            if (srcFormat == Format::YUV420SP && dstFormat == Format::GRAY) {
                // same instance and converting YUV420SP to GRAY: easy case
                dst.resize(Format::GRAY, dst.dims());
                return;
            }

            // same instance: convert into a new, temporary Image, then move into dst
            Image temp;
            convert(src, temp, dstFormat);
            dst = std::move(temp);
            return;
        }

        enum { ERROR = -1 };
        int code = ERROR;

        dst.resize(dstFormat, src.dims()); // this is why we check for same instance
        cv::Mat srcMat = src.wrap();
        cv::Mat dstMat = dst.wrap();

        if (srcFormat == Format::BGR) {
            if (dstFormat == Format::GRAY) { code = cv::COLOR_BGR2GRAY; }
        } else if (srcFormat == Format::GRAY) {
            if (dstFormat == Format::BGR) { code = cv::COLOR_GRAY2BGR; }
        } else if (srcFormat == Format::YUV420SP) {
            if (dstFormat == Format::BGR) {
                code = cv::COLOR_YUV420sp2BGR;
            } else if (dstFormat == Format::GRAY) {
                code = cv::COLOR_YUV420sp2GRAY;
            }
        }

        if (code == ERROR) {
            throw std::runtime_error("convert: failed to perform color conversion");
        }

        cv::cvtColor(srcMat, dstMat, code);

        FMO_ASSERT(dstMat.data == dst.data(), "convert: dst buffer reallocated");
    }
}
