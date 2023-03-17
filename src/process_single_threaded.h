#pragma once
#include <opencv2/videoio.hpp>

void process_single_threaded(cv::VideoCapture &videoCapture, cv::VideoWriter &videoWriter);
