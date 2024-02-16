#include <chrono>
#include <cstdio>
#include <cstdlib> // putenv / setenv
#include <ctime>
#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include <opencv2/core/utils/logger.hpp>
#include <opencv2/videoio.hpp>
#include <string>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "ConditionalOStream.h"
#include "ProcessingParameters.h"
#include "StdoutVideoWriter.h"
#include "process_single_threaded.h"

using namespace cv;
namespace chrono = std::chrono;
using std::cerr;
using std::endl;
using std::ifstream;
using std::invalid_argument;
using std::out_of_range;
using std::stod;
using std::string;

// TODO: Add --col-range commandline parameter
// TODO: Replace the positional framerate parameter with --framerate option
int main(int argc, char *argv[]) {
    const string VERSION = "2.0.0";

    cxxopts::Options options("vhs-deshaker", "vhs-deshaker " + VERSION + "\nFix horizontal shaking in digitized VHS videos\n");
    // clang-format off
    options.add_options()
        //("d,debug", "Enable debugging")
        ("i,input", "Input video", cxxopts::value<std::string>())
        ("o,output", "Output video", cxxopts::value<std::string>())
        ("f,framerate", "Enforce this framerate for the output video", cxxopts::value<double>())
        ("c,colrange", "Column range, -1 = use double the value given by -w", cxxopts::value<int>()->default_value(std::to_string(ProcessingParameters::DEFAULT_COL_RANGE)))
        ("t,target-line-start", "Target line start, -1 = use same value as given by -w", cxxopts::value<int>()->default_value(std::to_string(ProcessingParameters::DEFAULT_TARGET_LINE_START)))
        ("w,pure-black-width", "Pure black area width", cxxopts::value<int>()->default_value(std::to_string(ProcessingParameters::DEFAULT_PURE_BLACK_WIDTH)))
        ("p,pure-black-threshold", "Pure black threshold", cxxopts::value<int>()->default_value(std::to_string(ProcessingParameters::DEFAULT_PURE_BLACK_THRESHOLD)))
        ("m,min-line-start-segment-length", "Min line start segment length", cxxopts::value<int>()->default_value(std::to_string(ProcessingParameters::DEFAULT_MIN_LINE_START_SEGMENT_LENGTH)))
        ("k,line-start-smoothing-kernel-size", "Line start smoothing kernel size", cxxopts::value<int>()->default_value(std::to_string(ProcessingParameters::DEFAULT_LINE_START_SMOOTHING_KERNEL_SIZE)))
        ("h,help", "Print usage");
    // clang-format on

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    } catch (const cxxopts::exceptions::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    if (argc == 1 || result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    // Check that input and output files are specified.
    if (result.count("input") == 0) {
        std::cerr << "ERROR: Input file must be specified with -i" << std::endl;
        return 1;
    }
    if (result.count("output") == 0) {
        std::cerr << "ERROR: Output file must be specified with -o" << std::endl;
        return 1;
    }

    // Check that each option is specified once at most.
    if (result.count("input") > 1) {
        std::cerr << "ERROR: Only one input file can be specified" << std::endl;
        return 1;
    }
    if (result.count("output") > 1) {
        std::cerr << "ERROR: Only one output file can be specified" << std::endl;
        return 1;
    }
    if (result.count("framerate") > 1) {
        std::cerr << "ERROR: Framerate can only be specified once" << std::endl;
        return 1;
    }
    if (result.count("colrange") > 1) {
        std::cerr << "ERROR: Column range can only be specified once" << std::endl;
        return 1;
    }
    if (result.count("target-line-start") > 1) {
        std::cerr << "ERROR: Target line start can only be specified once" << std::endl;
        return 1;
    }
    if (result.count("pure-black-width") > 1) {
        std::cerr << "ERROR: Pure black width can only be specified once" << std::endl;
        return 1;
    }
    if (result.count("pure-black-threshold") > 1) {
        std::cerr << "ERROR: Pure black threshold can only be specified once" << std::endl;
        return 1;
    }
    if (result.count("min-line-start-segment-length") > 1) {
        std::cerr << "ERROR: Min line start segment length can only be specified once" << std::endl;
        return 1;
    }
    if (result.count("line-start-smoothing-kernel-size") > 1) {
        std::cerr << "ERROR: Line start smoothing kernel size can only be specified once" << std::endl;
        return 1;
    }

    // Check that the framerate is a positive number.
    double framerate = -1;
    if (result.count("framerate") > 0) {
        framerate = result["framerate"].as<double>();

        if (framerate <= 0) {
            cerr << "ERROR: Invalid framerate (must be a positive number)" << endl;
            return 1;
        }
    }

    // Check that pure black threshold is in the range 1-254.
    if (result.count("pure-black-threshold") > 0) {
        int pure_black_threshold = result["pure-black-threshold"].as<int>();

        if (pure_black_threshold < 1 || pure_black_threshold > 254) {
            cerr << "ERROR: Invalid pure black threshold (must be in the range 1-254)" << endl;
            return 1;
        }
    }

    // Print warning if lineStartSmoothingKernelSize is an even number (it will be forced to be odd by adding 1).
    if (result.count("line-start-smoothing-kernel-size") > 0) {
        int line_start_smoothing_kernel_size = result["line-start-smoothing-kernel-size"].as<int>();

        if (line_start_smoothing_kernel_size % 2 == 0) {
            cerr << "WARNING: line-start-smoothing-kernel-size is an even number. It will be forced to be odd by adding 1." << endl;
        }
    }

    // Fill the processing parameters with the values from the commandline options.
    ProcessingParameters parameters;

    parameters.colRange = result["colrange"].as<int>();
    parameters.targetLineStart = result["target-line-start"].as<int>();
    parameters.pureBlackWidth = result["pure-black-width"].as<int>();
    parameters.pureBlackThreshold = result["pure-black-threshold"].as<int>();
    parameters.minLineStartSegmentLength = result["min-line-start-segment-length"].as<int>();
    parameters.lineStartSmoothingKernelSize = result["line-start-smoothing-kernel-size"].as<int>() | 0x1;

    if (parameters.colRange == -1) {
        parameters.colRange = 2 * parameters.pureBlackWidth;
    }

    if (parameters.targetLineStart == -1) {
        parameters.targetLineStart = parameters.pureBlackWidth;
    }

#ifndef _WIN32
    putenv((char *)"OPENCV_FFMPEG_LOGLEVEL=-8");
#else
    _putenv("OPENCV_FFMPEG_LOGLEVEL=-8");
#endif

    string input_file = result["input"].as<string>();
    string output_file = result["output"].as<string>();

    bool piping_to_stdout = (output_file == "stdout");
    ConditionalOStream cout(std::cout, !piping_to_stdout);
    cout << "vhs-deshaker " << VERSION << endl << endl;

    if (piping_to_stdout) {
        cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
    }

    if (input_file == output_file) {
        cerr << "ERROR: Input file matches output file." << endl;
        return 1;
    }

    // Check if the input file exists and can be opened.
    {
        ifstream input_file_stream(input_file);
        if (!input_file_stream.good()) {
            cerr << "ERROR: Input file cannot be opened." << endl;
            return 1;
        }
        input_file_stream.close();
    }

    cout << "Processing file " << input_file << " ..." << endl;
    VideoCapture videoCapture(input_file);
    if (!videoCapture.isOpened()) {
        cerr << "Could not open input file" << endl;
        return 1;
    }

    double fps = -1;
    if (framerate <= 0) {
        fps = videoCapture.get(CAP_PROP_FPS);
        if (fps <= 0) {
            cerr << "Could not get framerate from input file. Please provide a framerate manually." << endl;
            return 1;
        }
    } else {
        fps = framerate;
    }

    int fourcc = VideoWriter::fourcc('H', 'F', 'Y', 'U');
    cv::Size frameSize(videoCapture.get(CAP_PROP_FRAME_WIDTH), videoCapture.get(CAP_PROP_FRAME_HEIGHT));
    bool isColor = true;
    VideoWriter *videoWriter = nullptr;
    if (piping_to_stdout) {
        videoWriter = new StdoutVideoWriter();
#ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
#endif
    } else {
        videoWriter = new VideoWriter(output_file, fourcc, fps, frameSize, isColor);
    }
    if (!videoWriter->isOpened()) {
        cerr << "Could not create video writer" << endl;
        return 1;
    }

    // Print summary of processing parameters.
    cout << "Processing parameters:" << endl;
    if (framerate == -1) {
        cout << "  Frame rate:                       " << fps << " (same as input)" << endl;
    } else {
        cout << "  Frame rate:                       " << fps << " (specified by user)" << endl;
    }
    cout << "  Column range:                     " << parameters.colRange << endl;
    cout << "  Target line start:                " << parameters.targetLineStart << endl;
    cout << "  Pure black width:                 " << parameters.pureBlackWidth << endl;
    cout << "  Pure black threshold:             " << parameters.pureBlackThreshold << endl;
    cout << "  Min line start segment length:    " << parameters.minLineStartSegmentLength << endl;
    cout << "  Line start smoothing kernel size: " << parameters.lineStartSmoothingKernelSize << endl;

    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();

    try {
        process_single_threaded(videoCapture, *videoWriter, parameters, !piping_to_stdout);
    } catch (const std::exception &e) {
        cerr << "ERROR: " << e.what() << endl;
        return 1;
    }
    delete videoWriter;
    videoWriter = nullptr;

    end = chrono::system_clock::now();
    long elapsed_milliseconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    time_t start_time = chrono::system_clock::to_time_t(start);
    time_t end_time = chrono::system_clock::to_time_t(end);

    cout << "Started at  " << ctime(&start_time);
    cout << "Finished at " << ctime(&end_time);
    cout << "Elapsed time: " << elapsed_milliseconds << " milliseconds" << endl;

    return 0;
}
