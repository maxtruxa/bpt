#include "./log.hpp"

#include <neo/assert.hpp>
#include <neo/event.hpp>

#include <spdlog/spdlog.h>

#include <ostream>

#if _WIN32
#include <consoleapi2.h>
static void set_utf8_output() {
    // 65'001 is the codepage id for UTF-8 output
    ::SetConsoleOutputCP(65'001);
}
#else
static void set_utf8_output() {
    // Nothing on other platforms
}
#endif

void dds::log::init_logger() noexcept {
    // spdlog::set_pattern("[%H:%M:%S] [%^%-5l%$] %v");
    spdlog::set_pattern("[%^%-5l%$] %v");
}

void dds::log::ev_log::print() const noexcept { log_print(level, message); }

void dds::log::log_print(dds::log::level l, std::string_view msg) noexcept {
    static auto logger_inst = [] {
        auto logger = spdlog::default_logger_raw();
        logger->set_level(spdlog::level::trace);
        set_utf8_output();
        return logger;
    }();

    const auto lvl = [&] {
        switch (l) {
        case level::trace:
            return spdlog::level::trace;
        case level::debug:
            return spdlog::level::debug;
        case level::info:
            return spdlog::level::info;
        case level::warn:
            return spdlog::level::warn;
        case level::error:
            return spdlog::level::err;
        case level::critical:
            return spdlog::level::critical;
        case level::silent:
            return spdlog::level::off;
        }
        neo_assert_always(invariant, false, "Invalid log level", msg, int(l));
    }();

    logger_inst->log(lvl, msg);
}

void dds::log::log_emit(dds::log::ev_log ev) noexcept {
    if (!neo::get_event_subscriber<ev_log>()) {
        thread_local bool did_warn = false;
        if (!did_warn) {
            log_print(level::warn,
                      "The calling thread issued a log message, but there is no subscriber "
                      "listening for it. The log message will be dropped. This is a bug!");
            did_warn = true;
        }
    }
    neo::emit(ev);
}
