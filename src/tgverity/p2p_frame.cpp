#include "tgverity/p2p_frame.h"

#include <iomanip>
#include <sstream>

namespace tgverity {

std::string encodeFrame(const std::string& payload) {
    std::ostringstream out;
    out << "TGV1 " << std::setw(8) << std::setfill('0') << std::hex << payload.size() << "\n" << payload;
    return out.str();
}

std::optional<std::string> decodeFrame(const std::string& frame) {
    if (frame.rfind("TGV1 ", 0) != 0) return std::nullopt;
    auto newline = frame.find('\n');
    if (newline == std::string::npos) return std::nullopt;
    auto sizeText = frame.substr(5, newline - 5);
    if (sizeText.size() != 8) return std::nullopt;

    std::size_t size = 0;
    std::istringstream in(sizeText);
    in >> std::hex >> size;
    if (!in || frame.size() - newline - 1 != size) return std::nullopt;
    return frame.substr(newline + 1);
}

} // namespace tgverity
