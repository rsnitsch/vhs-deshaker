#include <iostream>
#include <sstream>

class ConditionalOStream : public std::ostream {
  public:
    ConditionalOStream(std::ostream &targetStream, bool condition);

    template <typename T> ConditionalOStream &operator<<(const T &value) {
        if (condition_) {
            targetStream_ << value;
        }
        return *this;
    }

    // Handle manipulators like std::endl
    ConditionalOStream &operator<<(std::ostream &(*func)(std::ostream &));

  private:
    std::ostream &targetStream_;
    bool condition_;
    std::stringbuf buffer_;
};
