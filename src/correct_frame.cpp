#include "correct_frame.h"
#include <opencv2/imgproc.hpp>

// #define ENABLE_VISUALIZATIONS
#ifdef ENABLE_VISUALIZATIONS
#include <opencv2/highgui.hpp>
#endif

using std::vector;

// Internal helper methods.
void get_raw_line_starts(const cv::Mat &sobelX, vector<int> &line_starts, int direction);
void denoise_line_starts(vector<int> &line_starts, vector<int> &segment_sizes);
void merge_line_starts_adv(const vector<int> &line_starts1, const vector<int> &line_starts2, vector<int> &segment_sizes1,
                           vector<int> &segment_sizes2, vector<int> &merged, int &merged_from_starts_count, int &merged_from_ends_count);
void interpolate_line_starts(vector<int> &line_starts);

const int MISSING = -1;
const int DIRECTION_LEFT_TO_RIGHT = 1;
const int DIRECTION_RIGHT_TO_LEFT = -1;

void correct_frame(cv::Mat &input, const int colRange, cv::Mat &grayBuffer, cv::Mat &sobelBuffer1, cv::Mat &sobelBuffer2,
                   vector<int> &line_starts_buffer, vector<int> &line_ends_buffer, cv::Mat &out) {
    out.create(input.size(), input.type());

    cv::Mat &gray = grayBuffer;
    cv::cvtColor(input.colRange(0, colRange), gray, cv::COLOR_BGR2GRAY);
    cv::Sobel(gray, sobelBuffer1, CV_32F, 1, 0);

    cv::cvtColor(input.colRange(input.cols - colRange, input.cols), gray, cv::COLOR_BGR2GRAY);
    cv::Sobel(gray, sobelBuffer2, CV_32F, 1, 0);

    vector<int> &line_starts = line_starts_buffer;
    vector<int> &line_ends = line_ends_buffer;
    get_raw_line_starts(sobelBuffer1, line_starts, DIRECTION_LEFT_TO_RIGHT);
    get_raw_line_starts(sobelBuffer2, line_ends, DIRECTION_RIGHT_TO_LEFT);

    auto line_starts_raw = line_starts;
    auto line_ends_raw = line_ends;
    vector<int> segment_sizes_start, segment_sizes_end;
    denoise_line_starts(line_starts, segment_sizes_start);
    denoise_line_starts(line_ends, segment_sizes_end);

    auto line_starts_after_denoising = line_starts;
    auto line_ends_after_denoising = line_ends;

    // merge_line_starts(line_starts, line_ends, line_starts);
    int merged_from_starts_count = 0;
    int merged_from_ends_count = 0;
    merge_line_starts_adv(line_starts, line_ends, segment_sizes_start, segment_sizes_end, line_starts, merged_from_starts_count,
                          merged_from_ends_count);
    auto line_starts_merged = line_starts;

    interpolate_line_starts(line_starts);
    auto line_starts_interpolated = line_starts;

    cv::Mat line_starts_mat(line_starts);
    cv::blur(line_starts_mat, line_starts_mat, cv::Size(1, 51));

#ifdef ENABLE_VISUALIZATIONS
    bool waitKey = false;
    cv::Vec3b color_for_line_starts(255, 255, 0);
#endif

#if 0
    cv::namedWindow("0 - sobelBuffer1 (from left side)");
    cv::imshow("0 - sobelBuffer1 (from left side)", sobelBuffer1 / 255.0);
    cv::namedWindow("0 - sobelBuffer2 (from right side)");
    cv::imshow("0 - sobelBuffer2 (from right side)", sobelBuffer2 / 255.0);
    waitKey = true;
#endif

#ifdef ENABLE_VISUALIZATIONS
    // Raw line starts: green.
    cv::Mat debug_image_line_starts_raw = input.clone();
    draw_line_starts(debug_image_line_starts_raw, line_starts_raw, color_for_line_starts, 0);
    draw_line_starts(debug_image_line_starts_raw, line_ends_raw, color_for_line_starts, input.cols - colRange);
    cv::namedWindow("1 - line_starts_raw");
    cv::imshow("1 - line_starts_raw", debug_image_line_starts_raw);
    waitKey = true;
#endif

#ifdef ENABLE_VISUALIZATIONS
    // After denoising: red.
    cv::Mat debug_image_line_starts_after_denoising = input.clone();
    draw_line_starts(debug_image_line_starts_after_denoising, line_starts_after_denoising, color_for_line_starts, 0);
    draw_line_starts(debug_image_line_starts_after_denoising, line_ends_after_denoising, color_for_line_starts, input.cols - colRange);
    cv::namedWindow("2 - line_starts_after_denoising");
    cv::imshow("2 - line_starts_after_denoising", debug_image_line_starts_after_denoising);
    waitKey = true;
#endif

#ifdef ENABLE_VISUALIZATIONS
    // After merging: yellow.
    cv::Mat debug_image_line_starts_merged = input.clone();
    draw_line_starts(debug_image_line_starts_merged, line_starts_merged, color_for_line_starts, 0);
    cv::namedWindow("3 - line_starts_merged");
    cv::imshow("3 - line_starts_merged", debug_image_line_starts_merged);
    waitKey = true;
#endif

#ifdef ENABLE_VISUALIZATIONS
    // After interpolating: cyan.
    cv::Mat debug_image_line_starts_interpolated = input.clone();
    draw_line_starts(debug_image_line_starts_interpolated, line_starts_interpolated, cv::Vec3b(255, 0, 255), 0);
    draw_line_starts(debug_image_line_starts_interpolated, line_starts_merged, color_for_line_starts, 0);
    cv::namedWindow("4 - line_starts_interpolated");
    cv::imshow("4 - line_starts_interpolated", debug_image_line_starts_interpolated);
    waitKey = true;
#endif

#ifdef ENABLE_VISUALIZATIONS
    // FINAL (after smoothing): blue.
    cv::Mat debug_image_line_starts_smoothed = input.clone();
    draw_line_starts(debug_image_line_starts_smoothed, line_starts, cv::Vec3b(255, 0, 255), 0);
    cv::namedWindow("5 - line_starts_final (smoothed)");
    cv::imshow("5 - line_starts_final (smoothed)", debug_image_line_starts_smoothed);
    waitKey = true;
#endif

#ifdef ENABLE_VISUALIZATIONS
    if (waitKey) {
        cv::waitKey();
    }
#endif

    // Use the line_start data obtained by the above code to shift the content of all rows of the frame
    // such that each row begins at TARGET_LINE_START. TARGET_LINE_START is the expected (ideal) width of the
    // left- and right-hand black borders in a decent digitized VHS video.
    const int TARGET_LINE_START = 8;

    for (int y = 0; y < input.rows; ++y) {
        cv::Mat input_row = input.row(y);
        cv::Mat output_row = out.row(y);
        int line_start = line_starts.at(y);

        if (line_start == MISSING) {
            input_row.copyTo(output_row);
        } else {
            // Uncomment the following line to test the gap filling code below.
            // If there are no white gaps at the sides of the output video, it's fine.
            // output_row = cv::Scalar(255, 255, 255);

            int shift = TARGET_LINE_START - line_start;
            if (shift > 0) {
                // By shifting the line, we create a gap on one side of the line. This gap must be filled with zeroes.
                memset(output_row.data, 0, shift * 3);

                memcpy(output_row.data + shift * 3, input_row.data, (input_row.cols - shift) * 3);
            } else if (shift < 0) {
                int abs_shift = -shift;

                // By shifting the line, we create a gap on one side of the line. This gap must be filled with zeroes.
                memset(output_row.data + (output_row.cols - abs_shift) * 3, 0, abs_shift * 3);

                memcpy(output_row.data, input_row.data + abs_shift * 3, (input_row.cols - abs_shift) * 3);
            } else {
                assert(shift == 0);
                memcpy(output_row.data, input_row.data, input_row.cols * 3);
            }
        }
    }
}

void draw_line_starts(cv::Mat &img, const std::vector<int> line_starts, const cv::Vec3b &color, int x_offset) {
    assert(img.type() == CV_8UC3);
    for (int y = 0; y < img.size().height; ++y) {
        if (line_starts.at(y) != MISSING) {
            img.at<cv::Vec3b>(y, line_starts.at(y) + x_offset) = color;
        }
    }
}

/**
 * Scans all rows in sobelX to find the positions where the black border meets the actual video content. sobelX = horizontal image gradient.
 *
 * These positions are referred to as (raw) line_starts. They are used to cleanup horizontal shaking by
 * un-shifting/re-aligning all rows of the frame such that all lines start at the same position.
 *
 * In a perfect (non-shaking) VHS video the line starts would already be aligned perfectly at a fixed location,
 * e.g. all lines would start at X = 8 and end at X = W - 8 (where 8 is the size of the black border to the left
 * and right).
 *
 * @param sobelX 	  must be a ROI that contains either the left-hand columns or right-hand columns of a video frame's horizontal
 * 					  image gradient
 * @param direction   indicates whether sobelX is based on the left-hand part of the video (use the constant DIRECTION_LEFT_TO_RIGHT) or
 * 					  the right-hand part of the video (use constant DIRECTION_RIGHT_TO_LEFT).
 * @param line_starts the determined line starts are saved in this list, i.e. line_starts.size() == sobelX.rows. The line
 * 					  start data may be incomplete, i.e. for some rows it may be impossible to determine the start
 * 					  position. The respective missing items in line_starts get assigned the special constant MISSING.
 */
void get_raw_line_starts(const cv::Mat &sobelX, vector<int> &line_starts, int direction) {
    assert(direction == DIRECTION_LEFT_TO_RIGHT || direction == DIRECTION_RIGHT_TO_LEFT);
    line_starts.resize(sobelX.rows, MISSING);

    int x_start, x_step, x_stop;
    if (direction == 1) {
        x_step = 1;
        x_start = 0;
        x_stop = sobelX.cols;
    } else {
        x_step = -1;
        x_start = sobelX.cols - 1;
        x_stop = -1;
    }
    for (int y = 0; y < sobelX.rows; ++y) {
        line_starts[y] = MISSING;
        for (int x = x_start; x != x_stop; x += x_step) {
            // double dX = cv::norm(next_pixel - pixel);
            double dX = abs(sobelX.at<float>(y, x));

            if (dX > 30) {
#if 0
				input.at<cv::Vec3b>(y, x) = cv::Vec3b(255, 255, 255);
#endif
                line_starts[y] = x;
                break;
            }
        }

#if 0
		if (y == 0) {
			cout << input_row << endl;
		}
#endif
    }

#if 0
	cout << cv::Mat(line_starts).t() << endl;
#endif
}

/**
 * Cleans up / denoises the line_starts. Compact segments of subsequent (i.e. neighboring) line_starts
 * are only kept if they are at least MIN_SEGMENT_LENGTH rows long.
 */
void denoise_line_starts(vector<int> &line_starts, vector<int> &segment_sizes) {
    segment_sizes.resize(line_starts.size(), 0);
    const int MIN_SEGMENT_LENGTH = 15;

    int current_segment_begin = -1;
    for (int i = 0; i < line_starts.size(); ++i) {
        if (current_segment_begin == -1 && line_starts.at(i) != MISSING) {
            // Start a new segment.
            current_segment_begin = i;
        } else if (current_segment_begin != -1) {
            if (line_starts.at(i) == MISSING || abs(line_starts.at(i) - line_starts.at(current_segment_begin)) >= 2) {
                // New segment starts here.
                int current_segment_length = i - current_segment_begin;

                if (current_segment_length < MIN_SEGMENT_LENGTH) {
                    // The previous segment was short. Discard it. It cannot be trusted.
                    for (int k = current_segment_begin; k < i; ++k) {
                        line_starts.at(k) = MISSING;
                    }
                } else {
                    // Save segment length.
                    for (int k = current_segment_begin; k < i; ++k) {
                        segment_sizes.at(k) = current_segment_length;
                    }
                }

                // Start new segment.
                if (line_starts.at(i) != MISSING) {
                    current_segment_begin = i;
                } else {
                    current_segment_begin = -1;
                }
            }
        }
    }
}

/**
 * Combines line_starts derived from analysis of left-hand side of video with line_starts (line_ends) from
 * right-hand side of video. When both left-hand and right-hand line_start data is available for a certain row,
 * then this function takes into account the length of the segments to decide which side wins. line_starts from
 * longer compact segments of line_starts are considered more reliable and therefore take precedence.
 */
void merge_line_starts_adv(const vector<int> &line_starts1, const vector<int> &line_starts2, vector<int> &segment_sizes1,
                           vector<int> &segment_sizes2, vector<int> &merged, int &merged_from_starts_count, int &merged_from_ends_count) {
    assert(line_starts1.size() == line_starts2.size());
    merged.resize(line_starts1.size(), MISSING);

    for (int i = 0; i < line_starts1.size(); ++i) {
        if (line_starts1[i] != MISSING) {
            if (line_starts2[i] != MISSING) {
                if (segment_sizes1[i] > segment_sizes2[i]) {
                    merged_from_starts_count++;
                    merged[i] = line_starts1[i];
                } else if (segment_sizes1[i] < segment_sizes2[i]) {
                    merged_from_ends_count++;
                    merged[i] = line_starts2[i];
                } else {
                    merged[i] = (line_starts1[i] + line_starts2[i]) / 2;
                }
            } else {
                merged[i] = line_starts1[i];
            }
        } else if (line_starts2[i] != MISSING) {
            merged[i] = line_starts2[i];
        }
    }
}

/**
 * Fills in gaps in the line_start data via linear interpolation (for inner gaps) and by extrapolating
 * via nearest-known-value replication (for outer gaps at the top/bottom of a frame).
 *
 * @todo Debug this function. It seems to be buggy. Interpolated segments seem to
 *       start too far (most of the times only 1px) on the left-hand side. The subsequent smoothing fixes
 *       this in most cases, but it should be investigated anyway. Also, the code could be refactored,
 *       it is a bit messy.
 */
void interpolate_line_starts(vector<int> &line_starts) {
    // Constant used to indicate that we are currently searching for the beginning of a gap segment.
    const int SEARCHING_GAP = -2;
    int current_gap_begin = SEARCHING_GAP;
    for (int i = 0; i < line_starts.size(); ++i) {
        if (current_gap_begin == SEARCHING_GAP && line_starts.at(i) == MISSING) {
            // The beginning of a gap has been found.
            current_gap_begin = i;
        } else if (current_gap_begin != SEARCHING_GAP) {
            if (line_starts.at(i) != MISSING || i == line_starts.size() - 1) {
                // The current gap ends here, either because we found a line_start or because we reached the end of the
                // line_starts vector (i.e. the bottom row of the frame).

                // For interpolation, we need to determine the last known line_start values from before the gap
                // and from after the gap.
                double line_start_after = MISSING;
                double line_start_before = MISSING;
                if (i != line_starts.size() - 1) {
                    line_start_after = line_starts.at(i);
                }
                if (current_gap_begin != 0) {
                    line_start_before = line_starts.at(current_gap_begin - 1);
                }

                double interpolated = 0;
                if (line_start_after != MISSING && line_start_before != MISSING) {
                    // Line_start values from before *and* after the current gap are known.
                    // This means it is an inner gap and we can interpolate the missing values.
                    int x0 = current_gap_begin - 1;
                    int x1 = i;
                    int y0 = line_start_before;
                    int y1 = line_start_after;
                    int x_range = i - (current_gap_begin - 1);
                    int y_range = line_start_after - line_start_before;

                    for (int k = current_gap_begin; k < i; ++k) {
                        // Linear interpolation.
                        double x = k;
                        line_starts.at(k) = static_cast<int>(round(y0 + (x - x0) / x_range * y_range));
                    }
                } else {
                    // This is an outer gap. We can only extrapolate.
                    if (line_start_after != MISSING) {
                        // The outer gap is towards the top of the frame.
                        interpolated = line_start_after;
                    } else if (line_start_before != MISSING) {
                        // The outer gap is towards the bottom of the frame.
                        interpolated = line_start_before;
                    } else {
                        // Neither a line_start value from before the gap is known nor a line_start value from after the gap.
                        // This means that no line_start data is available for this frame at all. Neither interpolation nor
                        // extrapolation is possible.
                        assert(i == line_starts.size() - 1);
                        return;
                    }

                    // Extrapolate by simply replicating the closest known line_start value.
                    // Like BORDER_REPLICATE in OpenCV.
                    interpolated = round(interpolated);
                    int interpolated_int = static_cast<int>(interpolated);
                    for (int k = current_gap_begin; k < i; ++k) {
                        line_starts.at(k) = interpolated_int;
                    }
                }

                // Search for new segment.
                current_gap_begin = SEARCHING_GAP;
            }
        }
    }
}
