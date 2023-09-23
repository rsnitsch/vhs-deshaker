#include <chrono>
#include <cstdio>
#include <cstdlib> // putenv / setenv
#include <ctime>
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
    const string VERSION = "1.0.3";

#ifndef _WIN32
    putenv((char *)"OPENCV_FFMPEG_LOGLEVEL=-8");
#else
    _putenv("OPENCV_FFMPEG_LOGLEVEL=-8");
#endif

    if (argc != 3 && argc != 4) {
        cerr << "vhs-deshaker " << VERSION << endl << endl;
        cerr << "Usage: vhs-deshaker <input-file> <output-file> [<framerate>]" << endl;
        return 1;
    }

    string input_file = argv[1];
    string output_file = argv[2];

    bool piping_to_stdout = (output_file == "stdout");
    ConditionalOStream cout(std::cout, !piping_to_stdout);
    cout << "vhs-deshaker " << VERSION << endl << endl;

    double framerate = -1;
    if (argc == 4) {
        try {
            size_t pos = -1;
            framerate = stod(argv[3], &pos);

            // Make sure that the entire string is parsed as a valid number.
            if (pos != strlen(argv[3])) {
                cerr << "ERROR: Invalid framerate (not a number): " << argv[3] << endl;
                return 1;
            }

            if (framerate <= 0) {
                cerr << "ERROR: Invalid framerate (must be a positive number)" << endl;
                return 1;
            }
        } catch (const invalid_argument &e) {
            cerr << "ERROR: Invalid framerate (not a number): " << e.what() << endl;
            return 1;
        } catch (const out_of_range &e) {
            cerr << "ERROR: Invalid framerate (number is out of range): " << e.what() << endl;
            return 1;
        }
    }

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

        cout << "Using same framerate as in input file: " << fps << endl;
    } else {
        fps = framerate;
        cout << "Using the user-specified framerate: " << fps << endl;
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

    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();

    int colRange = 15;
    process_single_threaded(videoCapture, *videoWriter, colRange, !piping_to_stdout);
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
