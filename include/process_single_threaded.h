#pragma once
#include <opencv2/videoio.hpp>

/**
 * Applies the VHS deshaking algorithm (correct_frame function) to all frames of a video.
 *
 * @param videoCapture input video frames are read from this object
 * @param videoWriter output video frames are written to this object
 * @param colRange see correct_frame function
 * @param no_output if true, no output is written to stdout; otherwise progress is printed for each 1000 frames
 */
void process_single_threaded(cv::VideoCapture &videoCapture, cv::VideoWriter &videoWriter, const int colRange, bool no_output);
