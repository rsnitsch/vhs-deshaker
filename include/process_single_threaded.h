#pragma once
#include "ProcessingParameters.h"
#include <opencv2/videoio.hpp>

/**
 * Applies the VHS deshaking algorithm (correct_frame function) to all frames of a video.
 *
 * @param videoCapture input video frames are read from this object
 * @param videoWriter output video frames are written to this object
 * @param parameters see ProcessingParameters.h
 * @param print_progress if true, progress is printed for each 1000 frames
 */
void process_single_threaded(cv::VideoCapture &videoCapture, cv::VideoWriter &videoWriter, const ProcessingParameters &parameters,
                             bool print_progress);
