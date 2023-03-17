#include <opencv2/videoio.hpp>
#include <chrono>
#include <ctime>
#include <iostream>
#include <string>

#include "process_single_threaded.h"

using namespace cv;
using namespace std;

int main(int argc, char *argv[]) {
	cout << "vhs-deshaker 1.0.0" << endl << endl;
	if (argc != 3 && argc != 4) {
		cerr << "Usage: vhs-deshaker <input-file> <output-file> [<framerate>]" << endl;
		return 1;
	}

	string input_file = argv[1];
	string output_file = argv[2];
	double framerate = (argc == 4) ? stod(argv[3]) : -1;

	if (input_file == output_file) {
		cerr << "ERROR: Input file matches output file." << endl;
		return 1;
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
	} else {
		fps = framerate;
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

	process_single_threaded(videoCapture, videoWriter);

	end = chrono::system_clock::now();
	long elapsed_milliseconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	time_t start_time = chrono::system_clock::to_time_t(start);
	time_t end_time = chrono::system_clock::to_time_t(end);

	cout << "Started at  " << ctime(&start_time);
	cout << "Finished at " << ctime(&end_time);
	cout << "Elapsed time: " << elapsed_milliseconds << " milliseconds" << endl;

	return 0;
}
