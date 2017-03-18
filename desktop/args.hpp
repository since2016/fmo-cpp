#ifndef FMO_DESKTOP_ARGS_HPP
#define FMO_DESKTOP_ARGS_HPP

#include "parser.hpp"

/// Processes and verifies command-line arguments.
struct Args {
    /// Read arguments from the command line. Throws exceptions if there are any errors.
    Args(int argc, char** argv);

    std::vector<std::string> inputs; ///< paths to video files to use as inputs
    std::vector<std::string> gts;    ///< paths to ground truth text files, enables evaluation
    int camera = -1;                 ///< camera ID to use as input
    std::string recordDir;           ///< directory to save recording to
    bool pauseFn = false;            ///< pause when a false negative is encountered
    bool pauseFp = false;            ///< pause when a false positive is encountered
    bool pauseRg = false;            ///< pause when a regression is encountered
    bool pauseIm = false;            ///< pause when an improvement is encountered
    std::string evalDir;             ///< path to output evaluation results file
    std::string baseline;            ///< path to previously saved results file, enables comparison
    int frame = -1;                  ///< frame number to pause at
    int wait = -1;                   ///< frame time in milliseconds
    bool headless = false;           ///< don't draw GUI unless the playback is paused
    bool demo = false;               ///< force demo visualizer
    bool debug = false;              ///< force debug visualizer
    bool help = false;               ///< display help

private:
    void validate() const;

    Parser mParser;
};

#endif // FMO_DESKTOP_ARGS_HPP
