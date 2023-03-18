#pragma once

#include <opencv2/core.hpp>
#include <vector>

void correct_frame(cv::Mat &input, cv::Mat &grayBuffer, cv::Mat &sobelBuffer1, cv::Mat &sobelBuffer2, std::vector<int> &line_starts_buffer,
                   std::vector<int> &line_ends_buffer, cv::Mat &out);

void draw_line_starts(cv::Mat &img, const std::vector<int> line_starts, const cv::Vec3b &color, int x_offset);
