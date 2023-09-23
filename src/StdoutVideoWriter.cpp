#include "StdoutVideoWriter.h"

StdoutVideoWriter::StdoutVideoWriter() {
#if 0
        // For debugging
        ofile.open("temp/StdoutVideoWriter.dat", std::ios::binary);
#endif
}

void StdoutVideoWriter::write(cv::InputArray image) {
    cv::Mat frame = image.getMat();
    assert(!frame.empty());
    assert(frame.type() == CV_8UC3);
    int count = frame.cols * frame.rows * 3;
    auto written = fwrite(frame.data, 1, count, stdout);

#if 0
        // For debugging
        ofile.write((const char *)frame.data, (size_t)frame.cols * frame.rows * 3);
#endif

    if (written != count) {
        assert(false);
        throw std::runtime_error("fwrite has not written all data");
    }

    totalWritten += written;
}

void StdoutVideoWriter::release() { fflush(stdout); }

bool StdoutVideoWriter::isOpened() const { return true; }

long StdoutVideoWriter::getTotalWritten() const { return totalWritten; }