# TPG Test Application - Binary File Format

## Overview

This document specifies the binary file format used by the TPG test application.

## 1. Binary File Format (.bin/.val)

**Binary file with header and int16 data.**

### 1.1 File Structure

```
┌─────────────────┐
│ File Header     │ (12 bytes)
├─────────────────┤
│ Data Block 0    │ (N samples × 2 bytes each)
├─────────────────┤
│ Data Block 1    │ (N samples × 2 bytes each)
├─────────────────┤
│ Data Block 2    │ (N samples × 2 bytes each)
├─────────────────┤
│ ...             │
└─────────────────┘
```

This binary file format can be used for:
- **Input data files (.bin)**: Time step data for processor testing
- **Validation files (.bin)**: Expected results for validation steps
- **Pipeline tests**: Multiple time step payloads for different channels

### 1.2 Header Specification

| Offset | Size (bytes) | Type   | Name         | Description                                    |
|-------|--------------|--------|--------------|------------------------------------------------|
| 0     | 4            | uint32 | magic_number | Magic number: 0x54504754 ("TPGT")              |
| 4     | 4            | uint32 | version      | Format version: 1.0.4 (matches tpglibs)        |
| 8     | 4            | uint32 | reserved     | Reserved for future use                        |

### 1.3 Data Format

- **Data Type:** int16 (2 bytes per sample)
- **Byte Order:** Little-endian
- **Layout:** One data block per read operation
- **Header:** 16 bytes with magic number and version

### 1.4 Usage with BinarySignalReader

`samples_per_block` is a logical abstraction for the size of data to be fed into a processing entity. If it is a single processor, then it is 16 integer type data for the 16 channels. If it is a TPGenerator data block, then it would be 4 pipelines * 16 channels.

```cpp
BinarySignalReader<int16_t> reader("data.bin");

// Read one data block at a time
while (!reader.eof()) {
    auto data_block = reader.next(samples_per_block);
    if (data_block.empty()) break;
    
    // Process this data block
    auto result = processor->process(data_block);
}
```

### 1.5 Header Validation

```cpp
struct BinaryFileHeader {
    uint32_t magic_number;
    uint32_t version;
    uint32_t reserved;
};

// Validate header when opening file
BinaryFileHeader header;
file.read(reinterpret_cast<char*>(&header), sizeof(BinaryFileHeader));

if (header.magic_number != 0x54504754) {
    throw std::runtime_error("Invalid binary file format");
}
if (header.version != 0x010004) {  // 1.0.4 in hex
    throw std::runtime_error("Unsupported file version");
}
```

## 2. Configuration File Format (.json)

**Simple JSON configuration file.**

### 2.1 Example Configuration

```json
{
  "processor_config": {
    "accum_limit": 10,
    "metric_collect_toggle_state": false
  },
  "test_config": {
    "processor_name": "AVXFrugalPedestalSubtractProcessor",
    "max_steps": 250000,
    "samples_per_time_step": 16,
    "validation_steps": [1, 200000]
  }
}
```

### 2.2 Configuration Fields

- **processor_config:** Processor-specific parameters (varies by processor type)
- **test_config.processor_name:** String specifying the processor name to use for the test. This must correspond to a known processor class name (e.g., `"AVXFrugalPedestalSubtractProcessor"`)
- **test_config.max_steps:** Integer specifying the maximum number of processing steps/time steps to run processing (even if no validation steps are being performed). If not provided, the maximum step will be inferred from the largest value in `validation_steps`.
- **test_config.samples_per_time_step:** Number of samples per time step (should in most cases be 16 for a single processor)
- **test_config.validation_steps:** Array of processing step numbers to perform validation (exact integer comparison)


## 3. Test File Examples

### 3.1 AVXFrugalPedestalSubtractProcessor Sanity Test

**Files:**
- `tpg_processor_avx_fps_sanity_test.bin` - Input file with constant 0 values
- `tpg_processor_avx_fps_sanity_val.bin` - Validation file with expected outputs
- `tpg_processor_avx_fps_sanity_config.json` - Configuration file

**Test Logic:**
- **Input:** Constant 0 values for all channels across all time steps
- **Step 1:** Baseline validation - output should be `-16384` (0 - initial_pedestal)
- **Step 200000:** Convergence validation - output should be `0` (0 - converged_pedestal)

**Configuration:**
```json
{
  "processor_config": {
    "accum_limit": 10,
    "metric_collect_toggle_state": false
  },
  "test_config": {
    "samples_per_time_step": 16,
    "validation_steps": [1, 200000]
  }
}
```

### 4. General Test File Naming Convention

**Pattern:** `tpg_processor_[processor_name]_[test_name]_[test/val/config]`

**Examples:**
- `tpg_processor_avx_fps_sanity_test.bin` - AVX Frugal Pedestal Subtract sanity test input
- `tpg_processor_avx_fps_sanity_val.val` - AVX Frugal Pedestal Subtract sanity test validation
- `tpg_processor_avx_fps_sanity_config.json` - AVX Frugal Pedestal Subtract sanity test config

