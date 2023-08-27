#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <opencv2/videoio.hpp>
#include <string>

#include "process_single_threaded.h"

using namespace cv;
using namespace std;

// TODO: Add --col-range commandline parameter
// TODO: Replace the positional framerate parameter with --framerate option
int main(int argc, char *argv[]) {
    cout << "vhs-deshaker 1.0.1" << endl << endl;
    if (argc != 3 && argc != 4) {
        cerr << "Usage: vhs-deshaker <input-file> <output-file> [<framerate>]" << endl;
        return 1;
    }

    string input_file = argv[1];
    string output_file = argv[2];
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
    VideoWriter videoWriter(output_file, fourcc, fps, frameSize, isColor);
    if (!videoWriter.isOpened()) {
        cerr << "Could not create video writer" << endl;
        return 1;
    }

    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();

    int colRange = 15;
    process_single_threaded(videoCapture, videoWriter, colRange);

    end = chrono::system_clock::now();
    long elapsed_milliseconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    time_t start_time = chrono::system_clock::to_time_t(start);
    time_t end_time = chrono::system_clock::to_time_t(end);

    cout << "Started at  " << ctime(&start_time);
    cout << "Finished at " << ctime(&end_time);
    cout << "Elapsed time: " << elapsed_milliseconds << " milliseconds" << endl;

    return 0;
}
