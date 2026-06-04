#include "server/logger.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <sstream>

namespace {
std::mutex log_mutex;

std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_buf;
#if defined(_WIN32)
    gmtime_s(&tm_buf, &time);
#else
    gmtime_r(&time, &tm_buf);
#endif

    std::ostringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S") << '.' << std::setfill('0') << std::setw(3)
       << ms.count() << 'Z';
    return ss.str();
}

void log_message(std::string_view level, std::string_view msg,
                 std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    nlohmann::json j;
    j["level"] = level;
    j["ts"] = current_timestamp();
    j["msg"] = msg;
    for (const auto& [k, v] : fields) {
        j[std::string(k)] = v;
    }
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cerr << j.dump() << std::endl;
}
}  // namespace

void log_info(std::string_view msg,
              std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    log_message("info", msg, fields);
}

void log_error(std::string_view msg,
               std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    log_message("error", msg, fields);
}
