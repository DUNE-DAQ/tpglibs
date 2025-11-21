/**
 * @file binary_file_validator_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE binary_file_validator_test
#define FMT_HEADER_ONLY

#include "tpglibs/testapp/common/BinaryFileValidator.hpp"

#include <boost/test/unit_test.hpp>
#include <sstream>
#include <vector>
#include <cstring>

using tpglibs::testapp::BinaryFileHeader;
using tpglibs::testapp::BinaryFileValidator;

namespace {

std::string make_header_buffer(uint32_t magic = BinaryFileValidator::MAGIC_NUMBER,
                               uint32_t version = BinaryFileValidator::VERSION,
                               uint32_t reserved = 0x00000000) {
  BinaryFileHeader header{magic, version, reserved};
  std::string buffer(BinaryFileValidator::HEADER_SIZE, '\0');
  std::memcpy(buffer.data(), &header, sizeof(BinaryFileHeader));
  return buffer;
}

} // namespace

BOOST_AUTO_TEST_SUITE(BinaryFileValidatorTest)

BOOST_AUTO_TEST_CASE(TestValidateStreamSuccess)
{
  auto buffer = make_header_buffer();
  std::istringstream stream(buffer);
  BinaryFileHeader header{};
  std::string error;
  bool ok = BinaryFileValidator::validate_stream(stream, header, error);

  BOOST_CHECK(ok);
  BOOST_CHECK(error.empty());
  BOOST_CHECK_EQUAL(header.magic_number, BinaryFileValidator::MAGIC_NUMBER);
  BOOST_CHECK_EQUAL(header.version, BinaryFileValidator::VERSION);
}

BOOST_AUTO_TEST_CASE(TestValidateStreamInvalidMagic)
{
  auto buffer = make_header_buffer(0xDDDDAAAA);
  std::istringstream stream(buffer);
  BinaryFileHeader header{};
  std::string error;
  bool ok = BinaryFileValidator::validate_stream(stream, header, error);

  BOOST_CHECK(!ok);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("magic") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestValidateStreamInvalidVersion)
{
  auto buffer = make_header_buffer(BinaryFileValidator::MAGIC_NUMBER, 0x99999999);
  std::istringstream stream(buffer);
  BinaryFileHeader header{};
  std::string error;
  bool ok = BinaryFileValidator::validate_stream(stream, header, error);

  BOOST_CHECK(!ok);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("version") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestValidateStreamShortHeader)
{
  std::string buffer(BinaryFileValidator::HEADER_SIZE - 4, '\0');
  std::istringstream stream(buffer);
  BinaryFileHeader header{};
  std::string error;
  bool ok = BinaryFileValidator::validate_stream(stream, header, error);

  BOOST_CHECK(!ok);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("header") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestValidateBufferSuccess)
{
  auto buffer = make_header_buffer();
  BinaryFileHeader header{};
  std::string error;
  bool ok = BinaryFileValidator::validate_buffer(
      reinterpret_cast<const uint8_t*>(buffer.data()),
      buffer.size(),
      header,
      error);

  BOOST_CHECK(ok);
  BOOST_CHECK(error.empty());
  BOOST_CHECK_EQUAL(header.magic_number, BinaryFileValidator::MAGIC_NUMBER);
  BOOST_CHECK_EQUAL(header.version, BinaryFileValidator::VERSION);
}

BOOST_AUTO_TEST_CASE(TestValidateBufferInsufficientBytes)
{
  std::vector<uint8_t> buffer(BinaryFileValidator::HEADER_SIZE - 1, 0);
  BinaryFileHeader header{};
  std::string error;
  bool ok = BinaryFileValidator::validate_buffer(buffer.data(), buffer.size(), header, error);

  BOOST_CHECK(!ok);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("Insufficient") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

