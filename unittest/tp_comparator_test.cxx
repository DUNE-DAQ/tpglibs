/**
 * @file tp_comparator_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE tp_comparator_test
#define FMT_HEADER_ONLY

#include "tpglibs/testapp/tp/TPComparator.hpp"
#include "test_helpers.hpp"

#include <boost/test/unit_test.hpp>
#include <vector>
#include <cstdint>
#include <limits>

using tpglibs::unittest::create_test_tp;

BOOST_AUTO_TEST_SUITE(TPComparatorTest)

BOOST_AUTO_TEST_CASE(TestExactMatch)
{
  // Create identical TP vectors
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  expected.push_back(create_test_tp(1000, 5, 200, 3));
  expected.push_back(create_test_tp(2000, 7, 250, 4));
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  actual.push_back(create_test_tp(1000, 5, 200, 3));
  actual.push_back(create_test_tp(2000, 7, 250, 4));
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(result.matches);
  BOOST_CHECK_EQUAL(result.first_mismatch_index, std::numeric_limits<size_t>::max());
  BOOST_CHECK(result.mismatch_reason.empty());
}

BOOST_AUTO_TEST_CASE(TestExactMatchUnsorted)
{
  // Create identical TP vectors but in different order
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  expected.push_back(create_test_tp(2000, 7, 250, 4));  // Later time_start
  expected.push_back(create_test_tp(1000, 5, 200, 3));  // Earlier time_start
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  actual.push_back(create_test_tp(1000, 5, 200, 3));    // Different order
  actual.push_back(create_test_tp(2000, 7, 250, 4));
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(result.matches);
  BOOST_CHECK_EQUAL(result.first_mismatch_index, std::numeric_limits<size_t>::max());
  BOOST_CHECK(result.mismatch_reason.empty());
}

BOOST_AUTO_TEST_CASE(TestSizeMismatch)
{
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  expected.push_back(create_test_tp(1000, 5, 200, 3));
  expected.push_back(create_test_tp(2000, 7, 250, 4));
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  actual.push_back(create_test_tp(1000, 5, 200, 3));
  // Missing second TP
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(!result.matches);
  BOOST_CHECK_EQUAL(result.first_mismatch_index, 0);
  BOOST_CHECK(result.mismatch_reason.find("Size mismatch") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("expected 2") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("got 1") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestEmptyVectors)
{
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(result.matches);
  BOOST_CHECK_EQUAL(result.first_mismatch_index, std::numeric_limits<size_t>::max());
  BOOST_CHECK(result.mismatch_reason.empty());
}

BOOST_AUTO_TEST_CASE(TestTimeStartMismatch)
{
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  expected.push_back(create_test_tp(1000, 5, 200, 3));
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  auto tp = create_test_tp(2000, 5, 200, 3);  // Different time_start
  actual.push_back(tp);
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(!result.matches);
  BOOST_CHECK_EQUAL(result.first_mismatch_index, 0);
  BOOST_CHECK(result.mismatch_reason.find("time_start mismatch") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("expected 1000") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("got 2000") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestChannelMismatch)
{
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  expected.push_back(create_test_tp(1000, 5, 200, 3));
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  auto tp = create_test_tp(1000, 10, 200, 3);  // Different channel
  actual.push_back(tp);
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(!result.matches);
  BOOST_CHECK_EQUAL(result.first_mismatch_index, 0);
  BOOST_CHECK(result.mismatch_reason.find("channel mismatch") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("expected 5") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("got 10") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestAdcPeakMismatch)
{
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  expected.push_back(create_test_tp(1000, 5, 200, 3));
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  auto tp = create_test_tp(1000, 5, 300, 3);  // Different adc_peak
  actual.push_back(tp);
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(!result.matches);
  BOOST_CHECK_EQUAL(result.first_mismatch_index, 0);
  BOOST_CHECK(result.mismatch_reason.find("adc_peak mismatch") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("expected 200") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("got 300") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestSamplesOverThresholdMismatch)
{
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  expected.push_back(create_test_tp(1000, 5, 200, 3));
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  auto tp = create_test_tp(1000, 5, 200, 5);  // Different samples_over_threshold
  actual.push_back(tp);
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(!result.matches);
  BOOST_CHECK_EQUAL(result.first_mismatch_index, 0);
  BOOST_CHECK(result.mismatch_reason.find("samples_over_threshold mismatch") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("expected 3") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("got 5") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestAdcIntegralMismatch)
{
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  auto tp_expected = create_test_tp(1000, 5, 200, 3);
  tp_expected.adc_integral = 1000;
  expected.push_back(tp_expected);
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  auto tp_actual = create_test_tp(1000, 5, 200, 3);
  tp_actual.adc_integral = 2000;  // Different adc_integral
  actual.push_back(tp_actual);
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(!result.matches);
  BOOST_CHECK_EQUAL(result.first_mismatch_index, 0);
  BOOST_CHECK(result.mismatch_reason.find("adc_integral mismatch") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("expected 1000") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("got 2000") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestSamplesToPeakMismatch)
{
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  auto tp_expected = create_test_tp(1000, 5, 200, 3);
  tp_expected.samples_to_peak = 10;
  expected.push_back(tp_expected);
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  auto tp_actual = create_test_tp(1000, 5, 200, 3);
  tp_actual.samples_to_peak = 20;  // Different samples_to_peak
  actual.push_back(tp_actual);
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(!result.matches);
  BOOST_CHECK_EQUAL(result.first_mismatch_index, 0);
  BOOST_CHECK(result.mismatch_reason.find("samples_to_peak mismatch") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("expected 10") != std::string::npos);
  BOOST_CHECK(result.mismatch_reason.find("got 20") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestMultipleTPsWithSorting)
{
  // Create TPs that need sorting (different order in expected vs actual)
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  expected.push_back(create_test_tp(1000, 5, 200, 3));
  expected.push_back(create_test_tp(2000, 7, 250, 4));
  expected.push_back(create_test_tp(1500, 6, 225, 3));
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  actual.push_back(create_test_tp(1500, 6, 225, 3));  // Different order
  actual.push_back(create_test_tp(1000, 5, 200, 3));
  actual.push_back(create_test_tp(2000, 7, 250, 4));
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(result.matches);
  BOOST_CHECK_EQUAL(result.first_mismatch_index, std::numeric_limits<size_t>::max());
  BOOST_CHECK(result.mismatch_reason.empty());
}

BOOST_AUTO_TEST_CASE(TestMultipleTPsSecondMismatch)
{
  // First TP matches, second TP has mismatch
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  expected.push_back(create_test_tp(1000, 5, 200, 3));
  expected.push_back(create_test_tp(2000, 7, 250, 4));
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  actual.push_back(create_test_tp(1000, 5, 200, 3));
  actual.push_back(create_test_tp(2000, 7, 300, 4));  // Different adc_peak
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(!result.matches);
  BOOST_CHECK_EQUAL(result.first_mismatch_index, 1);
  BOOST_CHECK(result.mismatch_reason.find("adc_peak mismatch") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestSortingByTimeStart)
{
  // Test sorting by time_start (primary key)
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  expected.push_back(create_test_tp(1000, 5, 200, 3));
  expected.push_back(create_test_tp(2000, 5, 200, 3));  // Same channel, later time
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  actual.push_back(create_test_tp(2000, 5, 200, 3));  // Reversed order
  actual.push_back(create_test_tp(1000, 5, 200, 3));
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(result.matches);
}

BOOST_AUTO_TEST_CASE(TestSortingByChannel)
{
  // Test sorting by channel (secondary key when time_start matches)
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  auto tp1 = create_test_tp(1000, 5, 200, 3);
  expected.push_back(tp1);
  auto tp2 = create_test_tp(1000, 7, 200, 3);  // Same time_start, different channel
  expected.push_back(tp2);
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  actual.push_back(tp2);  // Reversed order
  actual.push_back(tp1);
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(result.matches);
}

BOOST_AUTO_TEST_CASE(TestSortingBySamplesOverThreshold)
{
  // Test sorting by samples_over_threshold (tertiary key)
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> expected;
  auto tp1 = create_test_tp(1000, 5, 200, 3);
  tp1.samples_over_threshold = 3;
  expected.push_back(tp1);
  auto tp2 = create_test_tp(1000, 5, 200, 3);  // Same time_start and channel
  tp2.samples_over_threshold = 7;  // Different samples_over_threshold
  expected.push_back(tp2);
  
  std::vector<dunedaq::trgdataformats::TriggerPrimitive> actual;
  actual.push_back(tp2);  // Reversed order
  actual.push_back(tp1);
  
  auto result = tpglibs::testapp::TPComparator::compare(expected, actual);
  
  BOOST_CHECK(result.matches);
}

BOOST_AUTO_TEST_CASE(TestFormatTP)
{
  auto tp = create_test_tp(1000, 5, 200, 3);
  tp.adc_integral = 5000;
  tp.samples_to_peak = 10;
  
  std::string formatted = tpglibs::testapp::TPComparator::format_tp(tp);
  
  BOOST_CHECK(formatted.find("time_start=1000") != std::string::npos);
  BOOST_CHECK(formatted.find("channel=5") != std::string::npos);
  BOOST_CHECK(formatted.find("adc_peak=200") != std::string::npos);
  BOOST_CHECK(formatted.find("samples_over_threshold=3") != std::string::npos);
  BOOST_CHECK(formatted.find("adc_integral=5000") != std::string::npos);
  BOOST_CHECK(formatted.find("samples_to_peak=10") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

