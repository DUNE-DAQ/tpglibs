/**
 * @file test_config_parser_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE test_config_parser_test
#define FMT_HEADER_ONLY

#include "tpglibs/testapp/common/TestConfigParser.hpp"
#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>
#include <string>

using tpglibs::testapp::TestConfigParser;
using tpglibs::testapp::ProcessorTestConfig;

BOOST_AUTO_TEST_SUITE(TestConfigParserTest)

BOOST_AUTO_TEST_CASE(TestValidConfigWithAllFields)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10},
      {"metric_collect_toggle_state", false}
    }},
    {"test_config", {
      {"processor_name", "AVXFrugalPedestalSubtractProcessor"},
      {"samples_per_time_step", 16},
      {"max_steps", 250000},
      {"validation_steps", {1, 200000}}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(success);
  BOOST_CHECK(error.empty());
  BOOST_CHECK_EQUAL(result.processor_name, "AVXFrugalPedestalSubtractProcessor");
  BOOST_CHECK_EQUAL(result.samples_per_time_step, 16);
  BOOST_CHECK_EQUAL(result.max_steps, 250000);
  BOOST_CHECK_EQUAL(result.validation_steps.size(), 2);
  BOOST_CHECK_EQUAL(result.validation_steps[0], 1);
  BOOST_CHECK_EQUAL(result.validation_steps[1], 200000);
  BOOST_CHECK(result.processor_config.contains("accum_limit"));
}

BOOST_AUTO_TEST_CASE(TestValidConfigWithoutMaxSteps)
{
  // max_steps should be derived from validation_steps
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"validation_steps", {1, 200000}}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(success);
  BOOST_CHECK(error.empty());
  BOOST_CHECK_EQUAL(result.max_steps, 200001);  // max(1, 200000) + 1
}

BOOST_AUTO_TEST_CASE(TestValidConfigWithoutValidationSteps)
{
  // validation_steps is optional
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"max_steps", 1000}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(success);
  BOOST_CHECK(error.empty());
  BOOST_CHECK(result.validation_steps.empty());
  BOOST_CHECK_EQUAL(result.max_steps, 1000);
}

BOOST_AUTO_TEST_CASE(TestMissingTestConfig)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("test_config") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestMissingProcessorConfig)
{
  nlohmann::json config = {
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"max_steps", 1000}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("processor_config") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestMissingProcessorName)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"samples_per_time_step", 16},
      {"max_steps", 1000}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("processor_name") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestMissingSamplesPerTimeStep)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"max_steps", 1000}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("samples_per_time_step") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidProcessorNameType)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", 123},  // Should be string
      {"samples_per_time_step", 16},
      {"max_steps", 1000}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("processor_name") != std::string::npos);
  BOOST_CHECK(error.find("string") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidSamplesPerTimeStepType)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", "16"},  // Should be integer
      {"max_steps", 1000}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("samples_per_time_step") != std::string::npos);
  BOOST_CHECK(error.find("integer") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestNegativeSamplesPerTimeStep)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", -16},  // Should be positive
      {"max_steps", 1000}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("positive") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidValidationStepsType)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"validation_steps", "not an array"}  // Should be array
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("validation_steps") != std::string::npos);
  BOOST_CHECK(error.find("array") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidValidationStepsElementType)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"validation_steps", {1, "200000", 3}}  // Should contain integers
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("validation_steps") != std::string::npos);
  BOOST_CHECK(error.find("integers") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidMaxStepsType)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"max_steps", "1000"}  // Should be integer
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("max_steps") != std::string::npos);
  BOOST_CHECK(error.find("integer") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestNegativeMaxSteps)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"max_steps", -1000}  // Should be positive
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("positive") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestMissingMaxStepsAndValidationSteps)
{
  // Both max_steps and validation_steps are missing/empty
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK((error.find("max_steps") != std::string::npos || 
               error.find("validation_steps") != std::string::npos));
}

BOOST_AUTO_TEST_CASE(TestEmptyValidationSteps)
{
  // Empty validation_steps array should still require max_steps
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"validation_steps", nlohmann::json::array()}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK((error.find("max_steps") != std::string::npos || 
               error.find("validation_steps") != std::string::npos));
}

BOOST_AUTO_TEST_CASE(TestInvalidProcessorConfigType)
{
  nlohmann::json config = {
    {"processor_config", "not an object"},  // Should be object
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"max_steps", 1000}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("processor_config") != std::string::npos);
  BOOST_CHECK(error.find("object") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidTestConfigType)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", "not an object"}  // Should be object
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("test_config") != std::string::npos);
  BOOST_CHECK(error.find("object") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestMaxStepsDerivedFromSingleValidationStep)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"validation_steps", {42}}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(success);
  BOOST_CHECK(error.empty());
  BOOST_CHECK_EQUAL(result.max_steps, 43);  // 42 + 1
}

BOOST_AUTO_TEST_CASE(TestMaxStepsDerivedFromMultipleValidationSteps)
{
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"validation_steps", {10, 5, 100, 50}}  // Max is 100
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(success);
  BOOST_CHECK(error.empty());
  BOOST_CHECK_EQUAL(result.max_steps, 101);  // max(10, 5, 100, 50) + 1
  BOOST_CHECK_EQUAL(result.validation_steps.size(), 4);
}

BOOST_AUTO_TEST_CASE(TestNegativeValidationStep)
{
  // validation_steps should reject negative values
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"validation_steps", {-5}}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("validation_steps") != std::string::npos);
  BOOST_CHECK(error.find("non-negative") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestMixedPositiveNegativeValidationSteps)
{
  // validation_steps should reject if any value is negative
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"validation_steps", {1, -5, 10}}  // Contains negative value
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("validation_steps") != std::string::npos);
  BOOST_CHECK(error.find("non-negative") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestZeroValidationStep)
{
  // Zero is a valid (non-negative) validation step
  nlohmann::json config = {
    {"processor_config", {
      {"accum_limit", 10}
    }},
    {"test_config", {
      {"processor_name", "AVXThresholdProcessor"},
      {"samples_per_time_step", 16},
      {"validation_steps", {0, 10}}
    }}
  };
  
  ProcessorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_processor_config(config, result, error);
  
  BOOST_CHECK(success);
  BOOST_CHECK(error.empty());
  BOOST_CHECK_EQUAL(result.max_steps, 11);  // max(0, 10) + 1
  BOOST_CHECK_EQUAL(result.validation_steps.size(), 2);
  BOOST_CHECK_EQUAL(result.validation_steps[0], 0);
  BOOST_CHECK_EQUAL(result.validation_steps[1], 10);
}

BOOST_AUTO_TEST_SUITE_END()

