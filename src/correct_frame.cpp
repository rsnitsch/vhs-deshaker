#include "correct_frame.h"
#include <opencv2/imgproc.hpp>

// #define ENABLE_VISUALIZATIONS
#ifdef ENABLE_VISUALIZATIONS
#include <opencv2/highgui.hpp>
#endif

using std::vector;

// Internal helper methods.
void get_raw_line_starts(const cv::Mat &gray, const cv::Mat &sobelX, vector<int> &line_starts, int direction);
void denoise_line_starts(vector<int> &line_starts, vector<int> &segment_sizes);
void merge_line_starts_adv(const vector<int> &line_starts1, const vector<int> &line_starts2, vector<int> &segment_sizes1,
                           vector<int> &segment_sizes2, vector<int> &merged, int &merged_from_starts_count, int &merged_from_ends_count);
void fill_gaps_in_line_starts(vector<int> &line_starts);
bool extrapolate_line_starts(vector<int> &line_starts);
void interpolate_line_starts(vector<int> &line_starts);

const int MISSING = INT_MIN;
const int DIRECTION_LEFT_TO_RIGHT = 1;
const int DIRECTION_RIGHT_TO_LEFT = -1;

// TARGET_LINE_START is the expected(ideal) width of the
// left- and right-hand black borders in a decent digitized VHS video.
const int TARGET_LINE_START = 8;

void correct_frame(cv::Mat &input, const int colRange, cv::Mat &grayBuffer1, cv::Mat &grayBuffer2, cv::Mat &sobelBuffer1,
                   cv::Mat &sobelBuffer2, vector<int> &line_starts_buffer, vector<int> &line_ends_buffer, cv::Mat &out) {
    out.create(input.size(), input.type());

    cv::cvtColor(input.colRange(0, colRange), grayBuffer1, cv::COLOR_BGR2GRAY);
    cv::Sobel(grayBuffer1, sobelBuffer1, CV_32F, 1, 0);
    assert(sobelBuffer1.cols == colRange);

    cv::cvtColor(input.colRange(input.cols - colRange, input.cols), grayBuffer2, cv::COLOR_BGR2GRAY);
    cv::Sobel(grayBuffer2, sobelBuffer2, CV_32F, 1, 0);
    assert(sobelBuffer2.cols == colRange);

    vector<int> &line_starts = line_starts_buffer;
    vector<int> &line_ends = line_ends_buffer;
    get_raw_line_starts(grayBuffer1, sobelBuffer1, line_starts, DIRECTION_LEFT_TO_RIGHT);
    get_raw_line_starts(grayBuffer2, sobelBuffer2, line_ends, DIRECTION_RIGHT_TO_LEFT);

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

    fill_gaps_in_line_starts(line_starts);
    auto line_starts_gapfilled = line_starts;

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

    int x_offset = input.cols - 2 * TARGET_LINE_START;

#ifdef ENABLE_VISUALIZATIONS
    // Raw line starts: green.
    cv::Mat debug_image_line_starts_raw = input.clone();
    draw_line_starts(debug_image_line_starts_raw, line_starts_raw, color_for_line_starts, 0);
    draw_line_starts(debug_image_line_starts_raw, line_ends_raw, color_for_line_starts, x_offset);
    cv::namedWindow("1 - line_starts_raw");
    cv::imshow("1 - line_starts_raw", debug_image_line_starts_raw);
    waitKey = true;
#endif

#ifdef ENABLE_VISUALIZATIONS
    // After denoising: red.
    cv::Mat debug_image_line_starts_after_denoising = input.clone();
    draw_line_starts(debug_image_line_starts_after_denoising, line_starts_after_denoising, color_for_line_starts, 0);
    draw_line_starts(debug_image_line_starts_after_denoising, line_ends_after_denoising, color_for_line_starts, x_offset);
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
    cv::Mat debug_image_line_starts_gapfilled = input.clone();
    draw_line_starts(debug_image_line_starts_gapfilled, line_starts_gapfilled, cv::Vec3b(255, 0, 255), 0);
    draw_line_starts(debug_image_line_starts_gapfilled, line_starts_merged, color_for_line_starts, 0);
    cv::namedWindow("4 - line_starts_gapfilled");
    cv::imshow("4 - line_starts_gapfilled", debug_image_line_starts_gapfilled);
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

    // Use the line_start data obtained by the above code to shift the content of all rows of the frame
    // such that each row begins at TARGET_LINE_START.
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

#ifdef ENABLE_VISUALIZATIONS
    cv::namedWindow("6 - out");
    cv::imshow("6 - out", out);
    waitKey = true;
#endif

#ifdef ENABLE_VISUALIZATIONS
    if (waitKey) {
        cv::waitKey();
    }
#endif
}

void draw_line_starts(cv::Mat &img, const std::vector<int> line_starts, const cv::Vec3b &color, int x_offset) {
    assert(img.type() == CV_8UC3);
    for (int y = 0; y < img.size().height; ++y) {
        if (line_starts.at(y) != MISSING) {
            if ((line_starts.at(y) + x_offset) < 0) {
                // Negative line_start. Draw the entire row red.
                // TODO: Better visualization of negative line_starts.
                img.row(y) = cv::Vec3b(0, 0, 255);
            } else {
                img.at<cv::Vec3b>(y, line_starts.at(y) + x_offset) = color;
            }
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
 * @param gray          must be a ROI that contains either the left-hand columns or right-hand columns of a video frame
 * @param sobelX 	    the horizontal image gradient of the gray ROI
 * @param direction     indicates whether sobelX is based on the left-hand part of the video (use the constant DIRECTION_LEFT_TO_RIGHT) or
 * 	                    the right-hand part of the video (use constant DIRECTION_RIGHT_TO_LEFT).
 * @param line_starts   the determined line starts are saved in this list, i.e. line_starts.size() == sobelX.rows. The line
 * 	                    start data may be incomplete, i.e. for some rows it may be impossible to determine the start
 *                      position. The respective missing items in line_starts get assigned the special constant MISSING.
 */
void get_raw_line_starts(const cv::Mat &gray, const cv::Mat &sobelX, vector<int> &line_starts, int direction) {
    assert(direction == DIRECTION_LEFT_TO_RIGHT || direction == DIRECTION_RIGHT_TO_LEFT);
    line_starts.resize(sobelX.rows, MISSING);

    int x_start, x_step, x_stop;
    if (direction == DIRECTION_LEFT_TO_RIGHT) {
        x_step = 1;
        x_start = 0;
        x_stop = sobelX.cols;
    } else if (direction == DIRECTION_RIGHT_TO_LEFT) {
        x_step = -1;
        x_start = sobelX.cols - 1;
        x_stop = -1;
    } else {
        assert(false);
    }

    for (int y = 0; y < sobelX.rows; ++y) {
        line_starts[y] = MISSING;
        for (int x = x_start; x != x_stop; x += x_step) {
            // If there is no pure black at the edge, skip this row. The line start/end
            // cannot be determined.
            if (x == x_start && gray.at<uint8_t>(y, x) > 50) {
                break;
            }

            // double dX = cv::norm(next_pixel - pixel);
            double dX = abs(sobelX.at<float>(y, x));

            if (dX > 30) {
#if 0
				input.at<cv::Vec3b>(y, x) = cv::Vec3b(255, 255, 255);
#endif

#if 1
                if (direction == DIRECTION_LEFT_TO_RIGHT) {
                    line_starts[y] = x;
                } else if (direction == DIRECTION_RIGHT_TO_LEFT) {
                    int reference_point = (sobelX.cols - 2 * TARGET_LINE_START);
                    line_starts[y] = x - reference_point;
                }
#else
                // Old code that only worked because my colRange value (15) happened to be close to 2 * TARGET_LINE_START.
                // And TARGET_LINE_START equaled 8 for my videos. So 2 * TARGET_LINE_START was 16, which is almost the same
                // as colRange (15).
                line_starts[y] = x;
#endif

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
 * Fills in gaps in the line_start data by nearest-known-value extrapolation (for outer gaps)
 * and linear interpolation (for inner gaps).
 */
void fill_gaps_in_line_starts(vector<int> &line_starts) {
    bool no_outer_gaps_remain = extrapolate_line_starts(line_starts);
    if (no_outer_gaps_remain) {
        interpolate_line_starts(line_starts);
    }

#ifndef NDEBUG
    // All gaps must have been filled!
    // (Unless all line_starts are missing.)
    bool allMissing = true;
    bool someMissing = false;
    for (int i = 0; i < line_starts.size(); ++i) {
        if (line_starts.at(i) == MISSING) {
            someMissing = true;
        } else {
            allMissing = false;
        }
    }
    assert(allMissing || !someMissing);
#endif
}

/**
 * Fills in gaps in the line_start data at the beginning (top of frame) and end (bottom of frame)
 * by nearest-known-value replication.
 *
 * @returns true if outer gaps were filled (or no outer gaps were present in the first place),
 *          returns false if there are gaps but they could not be filled because no line_starts are
 *          known at all to extrapolate from.
 */
bool extrapolate_line_starts(vector<int> &line_starts) {
    // Gap at the beginning?
    if (line_starts.at(0) == MISSING) {
        // Find nearest known value.
        int first_known_value_index = -1;
        for (int i = 0; i < line_starts.size(); ++i) {
            if (line_starts.at(i) != MISSING) {
                first_known_value_index = i;
                break;
            }
        }

        if (first_known_value_index == -1) {
            // No known values at all. Nothing to do.
            return false;
        }

        // Fill gap.
        for (int i = first_known_value_index - 1; i >= 0; --i) {
            line_starts.at(i) = line_starts.at(first_known_value_index);
        }
    }

    // Gap at the end?
    if (line_starts.at(line_starts.size() - 1) == MISSING) {
        // Find nearest known value.
        int last_known_value_index = -1;
        for (int i = line_starts.size() - 1; i >= 0; --i) {
            if (line_starts.at(i) != MISSING) {
                last_known_value_index = i;
                break;
            }
        }

        if (last_known_value_index == -1) {
            // No known values at all. Nothing to do.
            return false;
        }

        // Fill gap.
        for (int i = last_known_value_index + 1; i < line_starts.size(); ++i) {
            line_starts.at(i) = line_starts.at(last_known_value_index);
        }
    }

    return true;
}

/**
 * Fills inner gaps in the line_start data via linear interpolation.
 *
 * @throws std::exception   if the line_starts data contains missing values at the beginning or end. Always use
 *                          extrapolate_line_starts first.
 */
void interpolate_line_starts(vector<int> &line_starts) {
    if (line_starts.at(0) == MISSING || line_starts.at(line_starts.size() - 1) == MISSING) {
        throw std::exception(
            "interpolate_line_starts cannot handle missing line_starts at the beginning or end. Always use extrapolate_line_starts first.");
    }

    const int SEARCHING_GAP = -2;
    int current_gap_begin = SEARCHING_GAP;
    int current_gap_end = SEARCHING_GAP;
    int known_value_before_gap = MISSING;
    int known_value_after_gap = MISSING;
    for (int i = 0; i < line_starts.size(); ++i) {
        if (current_gap_begin == SEARCHING_GAP) {
            // Searching for the start of a gap.
            if (line_starts.at(i) == MISSING) {
                // The beginning of a gap has been found.
                current_gap_begin = i;
            } else {
                known_value_before_gap = line_starts.at(i);
            }
        } else {
            // Searching for the end of the current gap.
            if (line_starts.at(i) != MISSING) {
                current_gap_end = i - 1;

                assert(known_value_before_gap != MISSING);
                known_value_after_gap = line_starts.at(i);

                // Linear interpolation.
                int x0 = current_gap_begin - 1;
                int x1 = i;
                int y0 = known_value_before_gap;
                int y1 = known_value_after_gap;
                int x_range = x1 - x0;
                int y_range = y1 - y0;

                for (int k = current_gap_begin; k <= current_gap_end; ++k) {
                    double x = k;
                    line_starts.at(k) = static_cast<int>(round(y0 + (x - x0) / x_range * y_range));
                }

                // Search next gaps.
                current_gap_begin = SEARCHING_GAP;
            }
        }
    }

#ifndef NDEBUG
    // All gaps must have been filled!
    for (int i = 0; i < line_starts.size(); ++i) {
        assert(line_starts.at(i) != MISSING);
    }
#endif
}
