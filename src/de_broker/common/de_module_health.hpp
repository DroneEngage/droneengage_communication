#ifndef DE_MODULE_HEALTH_H_
#define DE_MODULE_HEALTH_H_

#include <cstdint>
#include <cstddef>
#include <deque>

namespace de
{
    namespace comm
    {
        /**
         * @brief Result of one CModuleHealthMonitor::sample() call - see
         * TYPE_AndruavMessage_MODULE_HEALTH_STATUS in messages.hpp for field meaning.
         */
        typedef struct MODULE_HEALTH_SAMPLE_TAG
        {
            double   rss_mb     = 0;
            double   vmpeak_mb  = 0;
            double   vmswap_mb  = 0;
            int      threads    = 0;
            double   slope_mb_h = 0;
            int      trend      = 0; // MODULE_HEALTH_TREND_*
            int      status     = 0; // MODULE_HEALTH_STATUS_*
            uint64_t uptime_sec = 0;
            bool     valid      = false; // false if /proc/self/status could not be read
        } MODULE_HEALTH_SAMPLE;

        /**
         * @brief Samples this process's own memory footprint from /proc/self/status,
         * keeps a short rolling history, and derives a growth trend + health status
         * against configurable thresholds.
         *
         * This is the "Layer 2" in-process counterpart to the external
         * droneengage_performance_monitor (servers/droneengage_performance_monitor):
         * de_comm can self-report its own memory health to the GCS via the
         * communication server (see CAndruavFacade::sendMemoryStatus()) without
         * needing an external watchdog.
         *
         * Linux-only (reads /proc/self/status); sample() returns MODULE_HEALTH_SAMPLE
         * with valid=false if unavailable, so callers can skip sending in that case.
         */
        class CModuleHealthMonitor
        {
            public:
                static CModuleHealthMonitor& getInstance()
                {
                    static CModuleHealthMonitor instance;

                    return instance;
                }

                CModuleHealthMonitor(CModuleHealthMonitor const&) = delete;
                void operator=(CModuleHealthMonitor const&)       = delete;

            private:
                CModuleHealthMonitor() {};

            public:

                /**
                 * @brief Configure ceiling/growth thresholds used to derive [hs] status.
                 * Defaults (500MB ceiling / 20MB per hour growth) are generic; override
                 * per-module if it is known to legitimately use more (e.g. de_camera).
                 *
                 * @param max_rss_mb              RSS above this is MODULE_HEALTH_STATUS_CRITICAL.
                 * @param max_growth_mb_per_hour   Slope above this (once history is full) is MODULE_HEALTH_STATUS_WARNING.
                 * @param history_samples          Rolling window size used for the slope calculation.
                 */
                void configure (const double max_rss_mb, const double max_growth_mb_per_hour, const std::size_t history_samples = 40);

                /**
                 * @brief Reads current process memory, updates the rolling history, and
                 * returns the derived sample. Call periodically (e.g. every 30s) from the
                 * module's own main loop.
                 */
                MODULE_HEALTH_SAMPLE sample();

            private:

                struct RawSample { uint64_t timestamp_usec; double rss_mb; };

                bool readProcSelfStatus (double& rss_mb, double& vmpeak_mb, double& vmswap_mb, int& threads) const;
                double calculateSlope () const;

            private:

                std::deque<RawSample> m_history;
                std::size_t m_history_samples          = 40;
                double      m_max_rss_mb                = 500.0;
                double      m_max_growth_mb_per_hour    = 20.0;
                uint64_t    m_start_time_usec            = 0;
        };
    }
}
#endif
