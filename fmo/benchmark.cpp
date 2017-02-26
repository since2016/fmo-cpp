#include <fmo/benchmark.hpp>
#include <fmo/image.hpp>
#include <fmo/processing.hpp>
#include <fmo/stats.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <random>

namespace fmo {
    namespace {
        void log(log_t logFunc, const char* cStr) { logFunc(cStr); }

        template <typename Arg1, typename... Args>
        void log(log_t logFunc, const char* format, Arg1 arg1, Args... args) {
            char buf[81];
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
            snprintf(buf, sizeof(buf), format, arg1, args...);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
            logFunc(buf);
        }
    }

    Registry& Registry::get() {
        static Registry instance;
        return instance;
    }

    void Registry::runAll(log_t logFunc, stop_t stopFunc) const {
        fmo::SectionStats stats;

        try {
            log(logFunc, "Benchmark started.\n");
            log(logFunc, "Num threads: %d\n", cv::getNumThreads());

            for (auto func : mFuncs) {
                stats.reset();
                bool updated = false;

                while (!updated && !stopFunc()) {
                    stats.start();
                    func.second();
                    updated = stats.stop();
                }

                if (stopFunc()) { throw std::runtime_error("stopped"); }

                auto q = stats.quantilesMs();
                log(logFunc, "%s: %.2f / %.1f / %.0f\n", func.first, q.q50, q.q95, q.q99);
            }

            log(logFunc, "Benchmark finished.\n\n");
        } catch (std::exception& e) { log(logFunc, "Benchmark interrupted: %s.\n\n", e.what()); }
    }

    Benchmark::Benchmark(const char* name, bench_t func) {
        auto& reg = Registry::get();
        reg.add(name, func);
    }

    namespace {
        struct {
            cv::Mat grayNoise;
            cv::Mat grayCircles;
            cv::Mat rect;

            cv::Mat out1;
            cv::Mat out2;
            cv::Mat out3;

            fmo::Image grayNoiseImage;
            std::vector<fmo::Image> outImageVec;

            std::mt19937 re{5489};
            using limits = std::numeric_limits<int>;
            std::uniform_int_distribution<int> uniform{limits::min(), limits::max()};
            std::uniform_int_distribution<int> randomGray{2, 254};
        } global;

        struct Init {
            static const int W = 1920;
            static const int H = 1080;

            static cv::Mat newGrayMat() { return {cv::Size{W, H}, CV_8UC1}; }

            Init() {

                {
                    global.grayNoise = newGrayMat();
                    auto* data = global.grayNoise.data;
                    auto* end = data + (W * H);

                    for (; data < end; data += sizeof(int)) {
                        *(int*)data = global.uniform(global.re);
                    }

                    // cv::imwrite("grayNoise.png", global.grayNoise);

                    global.grayNoiseImage.assign(fmo::Format::GRAY, {W, H}, global.grayNoise.data);
                }

                {
                    global.grayCircles = newGrayMat();
                    auto* data = global.grayCircles.data;

                    for (int r = 0; r < H; r++) {
                        int rmod = ((r + 128) % 256);
                        int dy = std::min(rmod, 256 - rmod);
                        int dy2 = dy * dy;
                        for (int c = 0; c < W; c++) {
                            int cmod = ((c + 128) % 256);
                            int dx = std::min(cmod, 256 - cmod);
                            int dx2 = dx * dx;
                            *data++ = (dx2 + dy2 < 10000) ? 0xFF : 0x00;
                        }
                    }

                    // cv::imwrite("grayCircles.png", global.grayCircles);
                }

                global.rect = cv::getStructuringElement(cv::MORPH_RECT, {3, 3});
            }
        };

        void init() { static Init once; }

        Benchmark FMO_UNIQUE_NAME{"cv::resize/NEAREST", []() {
                                      init();
                                      cv::resize(global.grayNoise, global.out1,
                                                 {Init::W / 2, Init::H / 2}, 0, 0,
                                                 cv::INTER_NEAREST);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::resize/AREA", []() {
                                      init();
                                      cv::resize(global.grayNoise, global.out1,
                                                 {Init::W / 2, Init::H / 2}, 0, 0, cv::INTER_AREA);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"fmo::pyramid (6 levels)", []() {
                                      init();
                                      fmo::pyramid(global.grayNoiseImage, global.outImageVec, 6);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::threshold", []() {
                                      init();
                                      cv::threshold(global.grayNoise, global.out1, 0x80, 0xFF,
                                                    cv::THRESH_BINARY);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::absdiff", []() {
                                      init();
                                      cv::absdiff(global.grayNoise, global.grayCircles,
                                                  global.out1);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::dilate", []() {
                                      init();
                                      cv::dilate(global.grayNoise, global.out1, global.rect);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::erode", []() {
                                      init();
                                      cv::erode(global.grayNoise, global.out1, global.rect);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::floodFill", []() {
                                      init();
                                      auto newVal = uchar(global.randomGray(global.re));
                                      cv::floodFill(global.grayCircles, cv::Point{0, 0}, newVal);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::connectedComponents", []() {
                                      init();
                                      cv::connectedComponents(global.grayCircles, global.out1);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::connectedComponentsWithStats", []() {
                                      init();
                                      cv::connectedComponentsWithStats(global.grayCircles,
                                                                       global.out1, global.out2,
                                                                       global.out3);
                                  }};
    }
}