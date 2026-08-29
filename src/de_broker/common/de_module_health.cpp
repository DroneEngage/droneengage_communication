#include <fstream>
#include <sstream>
#include <string>

#include "de_module_health.hpp"
#include "../../messages.hpp"
#include "../../helpers/helpers.hpp"

using namespace de::comm;


void CModuleHealthMonitor::configure (const double max_rss_mb, const double max_growth_mb_per_hour, const std::size_t history_samples)
{
    m_max_rss_mb             = max_rss_mb;
    m_max_growth_mb_per_hour = max_growth_mb_per_hour;
    m_history_samples        = history_samples;
}


/**
 * @brief Parses VmRSS/VmPeak/VmSwap/Threads (kB where applicable) out of
 * /proc/self/status. Returns false if the file could not be opened (e.g.
 * non-Linux host or sandboxed environment without /proc).
 */
bool CModuleHealthMonitor::readProcSelfStatus (double& rss_mb, double& vmpeak_mb, double& vmswap_mb, int& threads) const
{
    std::ifstream status_file("/proc/self/status");
    if (!status_file.is_open()) return false;

    rss_mb = vmpeak_mb = vmswap_mb = 0;
    threads = 0;

    std::string line;
    while (std::getline(status_file, line))
    {
        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "VmRSS:")
        {
            uint64_t value_kb = 0;
            iss >> value_kb;
            rss_mb = value_kb / 1024.0;
        }
        else if (key == "VmPeak:")
        {
            uint64_t value_kb = 0;
            iss >> value_kb;
            vmpeak_mb = value_kb / 1024.0;
        }
        else if (key == "VmSwap:")
        {
            uint64_t value_kb = 0;
            iss >> value_kb;
            vmswap_mb = value_kb / 1024.0;
        }
        else if (key == "Threads:")
        {
            iss >> threads;
        }
    }

    return true;
}


/**
 * @brief Least-squares linear regression of RSS (MB) over elapsed time (hours)
 * across the rolling history. Returns 0 if fewer than 2 samples are available.
 */
double CModuleHealthMonitor::calculateSlope () const
{
    const std::size_t n = m_history.size();
    if (n < 2) return 0.0;

    const uint64_t t0 = m_history.front().timestamp_usec;
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0;
    for (const auto& s : m_history)
    {
        const double x = (s.timestamp_usec - t0) / 3600000000.0; // usec -> hours
        const double y = s.rss_mb;
        sum_x  += x;
        sum_y  += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }

    const double denom = (static_cast<double>(n) * sum_xx) - (sum_x * sum_x);
    if (denom == 0.0) return 0.0; // all samples effectively at the same instant

    return ((static_cast<double>(n) * sum_xy) - (sum_x * sum_y)) / denom;
}


MODULE_HEALTH_SAMPLE CModuleHealthMonitor::sample()
{
    MODULE_HEALTH_SAMPLE result;

    const uint64_t now = get_time_usec();
    if (m_start_time_usec == 0) m_start_time_usec = now;

    double rss_mb = 0, vmpeak_mb = 0, vmswap_mb = 0;
    int threads = 0;
    if (!readProcSelfStatus(rss_mb, vmpeak_mb, vmswap_mb, threads))
    {
        return result; // valid=false, caller should skip sending
    }

    m_history.push_back({now, rss_mb});
    while (m_history.size() > m_history_samples) m_history.pop_front();

    const double slope_mb_h = calculateSlope();

    result.valid      = true;
    result.rss_mb     = rss_mb;
    result.vmpeak_mb  = vmpeak_mb;
    result.vmswap_mb  = vmswap_mb;
    result.threads    = threads;
    result.slope_mb_h = slope_mb_h;
    result.uptime_sec = (now - m_start_time_usec) / 1000000ULL;

    // Ignore noise below +/-1 MB/h when deriving the trend direction.
    if (slope_mb_h > 1.0)       result.trend = MODULE_HEALTH_TREND_UP;
    else if (slope_mb_h < -1.0) result.trend = MODULE_HEALTH_TREND_DOWN;
    else                        result.trend = MODULE_HEALTH_TREND_STABLE;

    // Ceiling breach is CRITICAL regardless of history size; sustained growth
    // rate is WARNING, but only once the rolling window is full (avoids
    // false positives from startup allocation).
    if (rss_mb > m_max_rss_mb)
        result.status = MODULE_HEALTH_STATUS_CRITICAL;
    else if (m_history.size() >= m_history_samples && slope_mb_h > m_max_growth_mb_per_hour)
        result.status = MODULE_HEALTH_STATUS_WARNING;
    else
        result.status = MODULE_HEALTH_STATUS_OK;

    return result;
}
