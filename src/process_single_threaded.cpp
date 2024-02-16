#include "process_single_threaded.h"
#include "correct_frame.h"

#include <iostream>
#include <opencv2/highgui.hpp>
#include <vector>

// #define ENABLE_DEBUGGING
// #define ENABLE_DRAW_FRAME_NUMBER

#ifdef ENABLE_DRAW_FRAME_NUMBER
#include <opencv2/imgproc.hpp>
#endif

void process_single_threaded(cv::VideoCapture &videoCapture, cv::VideoWriter &videoWriter, const ProcessingParameters &parameters,
                             bool print_progress) {
    int i = 0;
    int frame_count = videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
    cv::Mat img, corrected, grayBuffer1, grayBuffer2;
    std::vector<int> line_starts, line_ends;
    while (videoCapture.grab()) {
        bool ret = videoCapture.retrieve(img);
        assert(ret);
        assert(!img.empty());

#ifdef ENABLE_DEBUGGING
        if (i > 10) {
#endif

#ifdef ENABLE_DRAW_FRAME_NUMBER
            cv::putText(img, std::to_string(i), cv::Point(img.cols / 2, 200), cv::FONT_HERSHEY_SIMPLEX, 5, cv::Scalar(255, 255, 255), 3,
                        cv::LINE_AA);
#endif
            correct_frame(img, parameters, grayBuffer1, grayBuffer2, line_starts, line_ends, corrected);

#ifdef ENABLE_DEBUGGING
            cv::namedWindow("Input");
            cv::imshow("Input", img);

            cv::namedWindow("Output");
            cv::imshow("Output", corrected);
#endif

            videoWriter.write(corrected);
            if (print_progress && i >= 1000 && i % 1000 == 0) {
                std::cout << "Current frame: " << i << "/" << frame_count << std::endl;
            }

#ifdef ENABLE_DEBUGGING
            cv::waitKey();
        }
#endif

        ++i;
    }
}
