#pragma once

#include <im3e/utils/core/types.h>
#include <im3e/utils/loggers.h>

#include <atomic>
#include <mutex>
#include <vector>

namespace im3e {

class LoggerTracker : public ILoggerTracker
{
public:
    void addError(std::string_view message) { m_errors.emplace_back(message); }

    void clearErrors() override { m_errors.clear(); }

    auto getErrors() const -> std::vector<std::string> override { return m_errors; }

private:
    std::vector<std::string> m_errors;
};

struct StreamLoggerContext
{
    std::atomic<LogLevel> levelFilter{LogLevel::Debug};

    std::mutex streamMutex;
    std::shared_ptr<std::ostream> pStream;

    std::mutex trackersMutex;
    std::vector<std::unique_ptr<LoggerTracker>> pTrackers;
};

class StreamLogger : public ILogger
{
public:
    StreamLogger(std::string_view category, std::shared_ptr<std::ostream> pStream);

    void setLevelFilter(LogLevel level) override;

    void error(std::string_view message) const override;
    void warning(std::string_view message) const override;
    void info(std::string_view message) const override;
    void debug(std::string_view message) const override;
    void verbose(std::string_view message) const override;

    auto createChild(std::string_view name) const -> std::unique_ptr<ILogger> override;
    auto createGlobalTracker() -> UniquePtrWithDeleter<ILoggerTracker> override;

private:
    StreamLogger(std::string_view name, std::shared_ptr<StreamLoggerContext> pContext);

    void _log(char type, LogLevel level, std::string_view message) const;

    const std::string m_name;
    const std::shared_ptr<StreamLoggerContext> m_pContext;
};

}  // namespace im3e