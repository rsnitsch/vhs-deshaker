#pragma once

struct ProcessingParameters {
    static const int DEFAULT_COL_RANGE = -1;
    static const int DEFAULT_TARGET_LINE_START = -1;
    static const int DEFAULT_PURE_BLACK_WIDTH = 8;
    static const int DEFAULT_PURE_BLACK_THRESHOLD = 50;
    static const int DEFAULT_MIN_LINE_START_SEGMENT_LENGTH = 15;
    static const int DEFAULT_LINE_START_SMOOTHING_KERNEL_SIZE = 51;

    /*
     * @brief The number of columns to the left and right of the video frames that are used for the line-start detection.
     *
     * This value must be big enough to find the majority of line starts in the frame. For example, for mild cases of shaking/distortion a
     * value of 2 * PURE_BLACK_START_COLUMN is sufficient, where PURE_BLACK_START_COLUMN is the column
     * where the pure black starts in your digitized VHS video.
     */
    int colRange = DEFAULT_COL_RANGE;

    // the target column where the video frame rows should be aligned to.
    int targetLineStart = DEFAULT_TARGET_LINE_START;

    // the (expected/ideal) width of the pure black area in the left- and right-hand borders.
    int pureBlackWidth = DEFAULT_PURE_BLACK_WIDTH;

    // the threshold for the pure black detection, a value between 0-255.
    int pureBlackThreshold = DEFAULT_PURE_BLACK_THRESHOLD;

    // the denoising removes all line starts unless they're part of a continuous line-starts segment at least this long.
    int minLineStartSegmentLength = DEFAULT_MIN_LINE_START_SEGMENT_LENGTH;

    // line starts are smoothed with a normalized blurring filter of this size.
    int lineStartSmoothingKernelSize = DEFAULT_LINE_START_SMOOTHING_KERNEL_SIZE;
};
