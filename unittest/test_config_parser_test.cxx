/**
 * @file test_config_parser_test.cxx
 *
 * @copyright This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE test_config_parser_test

#include "tpglibs/testapp/common/TestConfigParser.hpp"
#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>
#include <string>

using tpglibs::testapp::TestConfigParser;
using tpglibs::testapp::ProcessorTestConfig;
using tpglibs::testapp::TPGeneratorTestConfig;

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

// ============================================================================
// TPGenerator Config Parsing Tests
// ============================================================================

BOOST_AUTO_TEST_SUITE(TPGeneratorConfigParserTest)

BOOST_AUTO_TEST_CASE(TestValidTPGeneratorConfigWithAllFields)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {
          {"plane0", 200},
          {"plane1", 300},
          {"plane2", 445}
        }}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0}), 
      nlohmann::json::array({2, 0}), nlohmann::json::array({3, 0}),
      nlohmann::json::array({16, 1}), nlohmann::json::array({17, 1}), 
      nlohmann::json::array({18, 1}), nlohmann::json::array({19, 1}),
      nlohmann::json::array({32, 2}), nlohmann::json::array({33, 2}), 
      nlohmann::json::array({34, 2}), nlohmann::json::array({35, 2})
    })},
    {"test_config", {
      {"sample_tick_difference", 1.5},
      {"sot_minima", {2, 3, 4}},
      {"validation_frames", {0, 10, 100}},
      {"max_frames", 200}
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(success);
  BOOST_CHECK(error.empty());
  BOOST_CHECK_EQUAL(result.processor_configs.size(), 1);
  BOOST_CHECK_EQUAL(result.processor_configs[0].first, "AVXThresholdProcessor");
  BOOST_CHECK(result.processor_configs[0].second.contains("plane0"));
  BOOST_CHECK_EQUAL(result.channel_plane_mappings.size(), 12);
  BOOST_CHECK_EQUAL(result.channel_plane_mappings[0].first, 0);
  BOOST_CHECK_EQUAL(result.channel_plane_mappings[0].second, 0);
  BOOST_CHECK_CLOSE(result.sample_tick_difference, 1.5f, 0.001f);
  BOOST_CHECK_EQUAL(result.sot_minima.size(), 3);
  BOOST_CHECK_EQUAL(result.sot_minima[0], 2);
  BOOST_CHECK_EQUAL(result.sot_minima[1], 3);
  BOOST_CHECK_EQUAL(result.sot_minima[2], 4);
  BOOST_CHECK_EQUAL(result.validation_frames.size(), 3);
  BOOST_CHECK_EQUAL(result.validation_frames[0], 0);
  BOOST_CHECK_EQUAL(result.validation_frames[1], 10);
  BOOST_CHECK_EQUAL(result.validation_frames[2], 100);
  BOOST_CHECK_EQUAL(result.max_frames, 200);
}

BOOST_AUTO_TEST_CASE(TestValidTPGeneratorConfigWithDefaults)
{
  // Test with minimal required fields, using defaults
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(success);
  BOOST_CHECK(error.empty());
  BOOST_CHECK_CLOSE(result.sample_tick_difference, 1.0f, 0.001f);
  BOOST_CHECK_EQUAL(result.sot_minima.size(), 3);
  BOOST_CHECK_EQUAL(result.sot_minima[0], 1);
  BOOST_CHECK_EQUAL(result.sot_minima[1], 1);
  BOOST_CHECK_EQUAL(result.sot_minima[2], 1);
  BOOST_CHECK(result.validation_frames.empty());
  BOOST_CHECK_EQUAL(result.max_frames, -1);  // Not set
}

BOOST_AUTO_TEST_CASE(TestValidTPGeneratorConfigMaxFramesDerived)
{
  // max_frames should be derived from validation_frames
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", {
      {"validation_frames", {0, 10, 100}}
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(success);
  BOOST_CHECK(error.empty());
  BOOST_CHECK_EQUAL(result.max_frames, 101);  // max(0, 10, 100) + 1
}

BOOST_AUTO_TEST_CASE(TestMissingProcessorConfigs)
{
  nlohmann::json config = {
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("processor_configs") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestEmptyProcessorConfigs)
{
  nlohmann::json config = {
    {"processor_configs", nlohmann::json::array()},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("non-empty") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidProcessorConfigsType)
{
  nlohmann::json config = {
    {"processor_configs", "not an array"},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("processor_configs") != std::string::npos);
  BOOST_CHECK(error.find("array") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidProcessorConfigElement)
{
  nlohmann::json config = {
    {"processor_configs", {
      "not an object"
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("object") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestMissingProcessorName)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("processor_name") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestMissingProcessorConfig)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("config") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestMissingChannelPlaneMappings)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("channel_plane_mappings") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestEmptyChannelPlaneMappings)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array()}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("non-empty") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidChannelPlaneMappingFormat)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", {
      {0, 0, 0}  // Should be 2 elements, not 3
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("2 elements") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidChannelPlaneMappingTypes)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", {
      {"0", 0}  // Channel should be unsigned integer
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("channel_plane_mapping") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidPlaneValue)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", {
      {0, 5}  // Plane should be 0, 1, or 2
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("plane") != std::string::npos);
  BOOST_CHECK((error.find("0, 1, or 2") != std::string::npos || 
               error.find("0") != std::string::npos));
}

BOOST_AUTO_TEST_CASE(TestNegativePlaneValue)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", {
      {0, -1}  // Plane should be 0, 1, or 2
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("plane") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidSampleTickDifferenceType)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", {
      {"sample_tick_difference", "1.0"}  // Should be number
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("sample_tick_difference") != std::string::npos);
  BOOST_CHECK(error.find("number") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestNegativeSampleTickDifference)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", {
      {"sample_tick_difference", -1.0}  // Should be positive
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("positive") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidSotMinimaType)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", {
      {"sot_minima", "not an array"}
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("sot_minima") != std::string::npos);
  BOOST_CHECK(error.find("array") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestEmptySotMinima)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", {
      {"sot_minima", nlohmann::json::array()}
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("sot_minima") != std::string::npos);
  BOOST_CHECK(error.find("empty") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidSotMinimaElementType)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", {
      {"sot_minima", {1, "2", 3}}  // Should contain unsigned integers
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("sot_minima") != std::string::npos);
  BOOST_CHECK(error.find("integers") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidValidationFramesType)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", {
      {"validation_frames", "not an array"}
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("validation_frames") != std::string::npos);
  BOOST_CHECK(error.find("array") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidValidationFramesElementType)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", {
      {"validation_frames", {0, "10", 20}}  // Should contain integers
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("validation_frames") != std::string::npos);
  BOOST_CHECK(error.find("integers") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestNegativeValidationFrame)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", {
      {"validation_frames", {-1, 10}}  // Should be non-negative
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("validation_frames") != std::string::npos);
  BOOST_CHECK(error.find("non-negative") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestInvalidMaxFramesType)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", {
      {"max_frames", "100"}  // Should be integer
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("max_frames") != std::string::npos);
  BOOST_CHECK(error.find("integer") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestNegativeMaxFrames)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", {
      {"max_frames", -100}  // Should be positive
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("positive") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestMultipleProcessorConfigs)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      },
      {
        {"processor_name", "AVXFrugalPedestalSubtractProcessor"},
        {"config", {{"accum_limit", 10}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(success);
  BOOST_CHECK(error.empty());
  BOOST_CHECK_EQUAL(result.processor_configs.size(), 2);
  BOOST_CHECK_EQUAL(result.processor_configs[0].first, "AVXThresholdProcessor");
  BOOST_CHECK_EQUAL(result.processor_configs[1].first, "AVXFrugalPedestalSubtractProcessor");
}

BOOST_AUTO_TEST_CASE(TestZeroValidationFrame)
{
  // Zero is a valid (non-negative) validation frame
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", {
      {"validation_frames", {0, 10}}
    }}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(success);
  BOOST_CHECK(error.empty());
  BOOST_CHECK_EQUAL(result.validation_frames.size(), 2);
  BOOST_CHECK_EQUAL(result.validation_frames[0], 0);
  BOOST_CHECK_EQUAL(result.validation_frames[1], 10);
  BOOST_CHECK_EQUAL(result.max_frames, 11);  // max(0, 10) + 1
}

BOOST_AUTO_TEST_CASE(TestInvalidTestConfigType)
{
  nlohmann::json config = {
    {"processor_configs", {
      {
        {"processor_name", "AVXThresholdProcessor"},
        {"config", {{"plane0", 200}}}
      }
    }},
    {"channel_plane_mappings", nlohmann::json::array({
      nlohmann::json::array({0, 0}), nlohmann::json::array({1, 0})
    })},
    {"test_config", "not an object"}
  };
  
  TPGeneratorTestConfig result;
  std::string error;
  
  bool success = TestConfigParser::parse_tpgenerator_config(config, result, error);
  
  BOOST_CHECK(!success);
  BOOST_CHECK(!error.empty());
  BOOST_CHECK(error.find("test_config") != std::string::npos);
  BOOST_CHECK(error.find("object") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

