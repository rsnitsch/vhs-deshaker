#include "process_single_threaded.h"
#include "correct_frame.h"

#include <iostream>
#include <opencv2/highgui.hpp>
#include <vector>

// #define ENABLE_DEBUGGING

void process_single_threaded(cv::VideoCapture &videoCapture, cv::VideoWriter &videoWriter, const int colRange) {
    int i = 0;
    int frame_count = videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
    cv::Mat img, corrected, grayBuffer, sobelBuffer1, sobelBuffer2;
    std::vector<int> line_starts, line_ends;
    while (videoCapture.grab()) {
        videoCapture.retrieve(img);

#ifdef ENABLE_DEBUGGING
        if (i > 10) {
#endif
            correct_frame(img, colRange, grayBuffer, sobelBuffer1, sobelBuffer2, line_starts, line_ends, corrected);

#ifdef ENABLE_DEBUGGING
            cv::namedWindow("Input");
            cv::imshow("Input", img);

            cv::namedWindow("Output");
            cv::imshow("Output", corrected);
#endif

            videoWriter.write(corrected);
            if (i >= 1000 && i % 1000 == 0) {
                std::cout << "Current frame: " << i << "/" << frame_count << std::endl;
            }

#ifdef ENABLE_DEBUGGING
            cv::waitKey();
        }
#endif

        ++i;
    }
}
