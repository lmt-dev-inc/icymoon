#include "stream_logger.h"

#include <im3e/utils/core/throw_utils.h>

#include <fmt/format.h>

#include <algorithm>
#include <fstream>
#include <iostream>

using namespace im3e;
using namespace std::string_view_literals;

namespace {

constexpr auto RootName = "Root"sv;

auto addErrorToTrackers(StreamLoggerContext& rContext, std::string_view message)
{
    std::lock_guard<std::mutex> lg(rContext.trackersMutex);
    std::ranges::for_each(rContext.pTrackers, [&](auto& pTracker) { pTracker->addError(message); });
}

}  // namespace

StreamLogger::StreamLogger(std::string_view name, std::shared_ptr<std::ostream> pStream)
  : m_name(name)
  , m_pContext(std::make_shared<StreamLoggerContext>())
{
    m_pContext->pStream = throwIfArgNull(std::move(pStream), "Cannot create logger without stream");
}

void StreamLogger::setLevelFilter(LogLevel level)
{
    m_pContext->levelFilter.store(level);
}

void StreamLogger::error(std::string_view message) const
{
    _log('E', LogLevel::Error, message);

    addErrorToTrackers(*m_pContext, message);
}
void StreamLogger::warning(std::string_view message) const
{
    _log('W', LogLevel::Warning, message);
}
void StreamLogger::info(std::string_view message) const
{
    _log('I', LogLevel::Info, message);
}
void StreamLogger::debug(std::string_view message) const
{
    _log('D', LogLevel::Debug, message);
}
void StreamLogger::verbose(std::string_view message) const
{
    _log('V', LogLevel::Verbose, message);
}

auto StreamLogger::createChild(std::string_view name) const -> std::unique_ptr<ILogger>
{
    return std::unique_ptr<ILogger>(new StreamLogger(name, m_pContext));
}

auto StreamLogger::createGlobalTracker() -> UniquePtrWithDeleter<ILoggerTracker>
{
    std::lock_guard<std::mutex> lg(m_pContext->trackersMutex);
    m_pContext->pTrackers.emplace_back(std::make_unique<LoggerTracker>());

    return UniquePtrWithDeleter<ILoggerTracker>(m_pContext->pTrackers.back().get(), [pContext = m_pContext](auto* pT) {
        std::lock_guard<std::mutex> lg(pContext->trackersMutex);
        const auto [itStart, itEnd] = std::ranges::remove_if(pContext->pTrackers,
                                                             [&](auto& pTracker) { return pTracker.get() == pT; });
        pContext->pTrackers.erase(itStart, itEnd);
    });
}

StreamLogger::StreamLogger(std::string_view name, std::shared_ptr<StreamLoggerContext> pContext)
  : m_name(name)
  , m_pContext(std::move(pContext))
{
}

void StreamLogger::_log(char type, LogLevel level, std::string_view message) const
{
    if (level <= m_pContext->levelFilter.load())
    {
        std::lock_guard<std::mutex> lg(m_pContext->streamMutex);
        *(m_pContext->pStream) << fmt::format("[{}][{}] {}", type, m_name, message) << std::endl;
    }
}

std::unique_ptr<ILogger> im3e::createTerminalLogger()
{
    return std::make_unique<StreamLogger>(RootName, std::shared_ptr<std::ostream>(&std::cout, [](auto*) {}));
}

std::unique_ptr<ILogger> im3e::createFileLogger(const std::filesystem::path& rFilePath)
{
    return std::make_unique<StreamLogger>(RootName, std::make_shared<std::ofstream>(rFilePath, std::ios::trunc));
}