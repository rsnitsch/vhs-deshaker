#include "ConditionalOStream.h"

ConditionalOStream::ConditionalOStream(std::ostream &targetStream, bool condition)
    : targetStream_(targetStream), condition_(condition), std::ostream(&buffer_) {}

ConditionalOStream &ConditionalOStream::operator<<(std::ostream &(*func)(std::ostream &)) {
    if (condition_) {
        func(targetStream_);
    }
    return *this;
}
