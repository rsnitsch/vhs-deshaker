#pragma once

#include <opencv2/videoio.hpp>

class StdoutVideoWriter : public cv::VideoWriter {
  public:
    StdoutVideoWriter();

    void write(cv::InputArray image) override;

    void release() override;

    bool isOpened() const override;

    long getTotalWritten() const;

  private:
    long totalWritten = 0;
#if 0
    std::ofstream ofile; // For debugging
#endif
};
