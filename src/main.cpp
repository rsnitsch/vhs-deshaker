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

#include "process_single_threaded.h"

using namespace cv;
using namespace std;

class StdoutVideoWriter : public VideoWriter {
  public:
    StdoutVideoWriter() : VideoWriter() {
#if 0
        // For debugging
        ofile.open("temp/StdoutVideoWriter.dat", std::ios::binary);
#endif
    }

    void write(InputArray image) override {
        cv::Mat frame = image.getMat();
        assert(!frame.empty());
        assert(frame.type() == CV_8UC3);
        int count = frame.cols * frame.rows * 3;
        auto written = fwrite(frame.data, 1, count, stdout);

#if 0
        // For debugging
        ofile.write((const char *)frame.data, (size_t)frame.cols * frame.rows * 3);
#endif

        if (written != count) {
            assert(false);
            throw std::runtime_error("fwrite has not written all data");
        }

        totalWritten += written;
    }

    void release() override { fflush(stdout); }

    bool isOpened() const override { return true; }

    long getTotalWritten() const { return totalWritten; }

  private:
    long totalWritten = 0;
#if 0
    std::ofstream ofile; // For debugging
#endif
};

// TODO: Add --col-range commandline parameter
// TODO: Replace the positional framerate parameter with --framerate option
int main(int argc, char *argv[]) {
#ifndef _WIN32
    putenv((char *)"OPENCV_FFMPEG_LOGLEVEL=-8");
#else
    _putenv("OPENCV_FFMPEG_LOGLEVEL=-8");
#endif

    if (argc != 3 && argc != 4) {
        cerr << "vhs-deshaker 1.0.2" << endl << endl;
        cerr << "Usage: vhs-deshaker <input-file> <output-file> [<framerate>]" << endl;
        return 1;
    }

    string input_file = argv[1];
    string output_file = argv[2];

    if (output_file != "stdout") {
        cout << "vhs-deshaker 1.0.2" << endl << endl;
    }

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

    if (output_file == "stdout") {
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

    if (output_file != "stdout") {
        cerr << "Processing file " << input_file << " ..." << endl;
    }
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

        if (output_file != "stdout") {
            cout << "Using same framerate as in input file: " << fps << endl;
        }
    } else {
        fps = framerate;
        if (output_file != "stdout") {
            cout << "Using the user-specified framerate: " << fps << endl;
        }
    }

    int fourcc = VideoWriter::fourcc('H', 'F', 'Y', 'U');
    cv::Size frameSize(videoCapture.get(CAP_PROP_FRAME_WIDTH), videoCapture.get(CAP_PROP_FRAME_HEIGHT));
    bool isColor = true;
    VideoWriter *videoWriter = nullptr;
    if (output_file == "stdout") {
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
    process_single_threaded(videoCapture, *videoWriter, colRange, output_file == "stdout");
    delete videoWriter;
    videoWriter = nullptr;
    fclose(stdout);

    end = chrono::system_clock::now();
    long elapsed_milliseconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    time_t start_time = chrono::system_clock::to_time_t(start);
    time_t end_time = chrono::system_clock::to_time_t(end);

    if (output_file != "stdout") {
        cout << "Started at  " << ctime(&start_time);
        cout << "Finished at " << ctime(&end_time);
        cout << "Elapsed time: " << elapsed_milliseconds << " milliseconds" << endl;
    }

    return 0;
}
