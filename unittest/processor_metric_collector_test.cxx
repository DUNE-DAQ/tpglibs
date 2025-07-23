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
#include "trgdataformats/Types.hpp"
#include "tpglibs/AVXProcessor.hpp"
#include "tpglibs/AVXFrugalPedestalSubtractProcessor.hpp"

#include "tpglibs/ProcessorMetricCollector.hpp"

namespace tpglibs {
  
  BOOST_AUTO_TEST_CASE(test_processor_metric_collector_sanity) {
    // Sanity test: create, run, signal, get metrics, and stop
    std::vector<std::pair<std::string, nlohmann::json>> configs = {
      {
        "AVXFrugalPedestalSubtractProcessor",
        {
          {"accum_limit", 42}
        }
      },
      {
        "AVXFrugalPedestalSubtractProcessor",
        {
          {"accum_limit", 42}
        }
      },
    };

    // Lazy with the channel-plane assignments.
    std::vector<std::pair<dunedaq::trgdataformats::channel_t, int16_t>> channel_plane_numbers = 
    {{ 0, 0}, { 1, 0}, { 2, 0}, { 3, 0}, { 4, 0}, { 5, 0}, { 6, 0}, { 7, 0}, { 8, 0}, { 9, 0}, {10, 0}, {11, 0}, {12, 0}, {13, 0}, {14, 0}, {15, 0},
    {16, 1}, {17, 1}, {18, 1}, {19, 1}, {20, 1}, {21, 1}, {22, 1}, {23, 1}, {24, 1}, {25, 1}, {26, 1}, {27, 1}, {28, 1}, {29, 1}, {30, 1}, {31, 1},
    {32, 2}, {33, 2}, {34, 2}, {35, 2}, {36, 2}, {37, 2}, {38, 2}, {39, 2}, {40, 2}, {41, 2}, {42, 2}, {43, 2}, {44, 2}, {45, 2}, {46, 2}, {47, 2},
    {48, 0}, {49, 0}, {50, 0}, {51, 0}, {52, 0}, {53, 1}, {54, 1}, {55, 1}, {56, 1}, {57, 1}, {58, 2}, {59, 2}, {60, 2}, {61, 2}, {62, 2}, {63, 2}};
    ProcessorMetricCollector<__m256i> collector;

    collector.configure(configs, channel_plane_numbers, 4);

    std::string proc_name = "AVXFrugalPedestalSubtractProcessor";

    std::shared_ptr<AVXProcessor> pc = std::make_shared<AVXFrugalPedestalSubtractProcessor>();

    collector.attach_processor(*pc.get(), proc_name, 1);

    auto processors = collector._get_attached_processors();

    BOOST_TEST(processors[0] != nullptr);
    BOOST_TEST(processors[1] == nullptr);

    auto info = collector._get_processor_metric_table();

    BOOST_TEST(info[0].m_pipeline_id == 1);
    BOOST_TEST(info[0].m_names_of_metrics.size() == 2);

    // Launch run loop in background
    collector.run();

    // Trigger some signal collects
    collector.signal_collect();

    collector.signal_collect();

    collector.signal_collect();

    // Retrieve metrics (stub returns empty)
    auto metrics = collector.get_retrieved_processor_metrics();

    // Stop collector and join thread
    collector.stop();
  }

} // tpglibs
