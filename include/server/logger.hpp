#pragma once

#include <initializer_list>
#include <string_view>
#include <utility>

void log_info(std::string_view msg,
              std::initializer_list<std::pair<std::string_view, std::string_view>> fields = {});
void log_error(std::string_view msg,
               std::initializer_list<std::pair<std::string_view, std::string_view>> fields = {});
