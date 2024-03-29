#pragma once

#include <opencv2/core.hpp>
#include <vector>

#include "ProcessingParameters.h"

/**
 * Applies VHS deshaking to a single frame. The pure black on the left- and right-hand side of the frame is used
 * to realign the rows so that they start at the same x-position / column. This fixes mild to medium cases
 * of horizontal shaking and distortions caused by lack of TBC.
 *
 * @param input The input frame (BGR).
 * @param parameters See ProcessingParameters.h.
 * @param grayBuffer1 A Mat that can be reused as buffer for the grayscale version of the input frame (left border).
 * @param grayBuffer1 A Mat that can be reused as buffer for the grayscale version of the input frame (right border).
 * @param line_starts_buffer A vector that can be reused as buffer to store line starts.
 * @param line_ends_buffer A vector that can be reused as buffer to store line ends.
 * @param out The corrected output frame (BGR).
 */
void correct_frame(cv::Mat &input, const ProcessingParameters &parameters, cv::Mat &grayBuffer1, cv::Mat &grayBuffer2,
                   std::vector<int> &line_starts_buffer, std::vector<int> &line_ends_buffer, cv::Mat &out);

/**
 * Draw line starts into an image frame for debugging purposes.
 *
 * @param img The image frame to draw into (BGR).
 * @param line_starts The line starts to draw.
 * @param color The color to use for drawing.
 * @param x_offset An x-offset to apply to the line starts. Can be used to draw line ends instead of line starts.
 */
void draw_line_starts(cv::Mat &img, const std::vector<int> line_starts, const cv::Vec3b &color, int x_offset);
