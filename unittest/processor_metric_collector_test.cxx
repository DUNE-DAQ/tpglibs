/**
 * @file processor_metric_collector_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE boost_test_macro_overview
#define FMT_HEADER_ONLY
#include <boost/test/unit_test.hpp>

#include "tpglibs/ProcessorMetricCollector.hpp"

namespace tpglibs {
  
  BOOST_AUTO_TEST_CASE(test_processor_metric_collector_sanity) {
    // Sanity test: create, run, signal, get metrics, and stop
    std::vector<std::string> configs{};
    std::vector<int16_t> planes{};
    ProcessorMetricCollector collector(configs, planes, 4);

    // Launch run loop in background
    collector.run();

    // Trigger some signal collects
    collector.signal_collect();

    collector.signal_collect();

    // Retrieve metrics (stub returns empty)
    auto metrics = collector.get_retrieved_processor_metrics();

    // Stop collector and join thread
    collector.stop();
  }

} // tpglibs
