#include "logger.h"

namespace logger {
    Logger* g_logger = nullptr;

    void initializeLogger(const std::string& transaction_file,
                         const std::string& error_file) {
        if (g_logger == nullptr) {
            g_logger = new Logger(transaction_file, error_file);
        }
    }

    void shutdownLogger() {
        if (g_logger != nullptr) {
            delete g_logger;
            g_logger = nullptr;
        }
    }
}