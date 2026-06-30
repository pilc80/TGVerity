#pragma once

#include <optional>
#include <string>

namespace tgverity {

std::string encodeFrame(const std::string& payload);
std::optional<std::string> decodeFrame(const std::string& frame);

} // namespace tgverity
