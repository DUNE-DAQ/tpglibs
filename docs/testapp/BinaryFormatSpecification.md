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

---

## 5. TPGenerator Test Application Binary Formats

### 5.1 Overview

The TPGenerator test application uses two distinct binary formats:
- **Frame Input Files**: Binary files containing frame data compatible with `DUMMY_FRAME_STRUCT` or similar frame types
- **TP Validation Files**: Binary files containing serialized `TriggerPrimitive` objects for validation

### 5.2 Frame Input File Format (.bin)

**Purpose**: Store frame data to be processed by TPGenerator.

#### 5.2.1 File Structure

```
┌─────────────────┐
│ File Header     │ (12 bytes) - Same as Section 1.2
├─────────────────┤
│ Frame 0         │ (Frame size: variable, see below)
├─────────────────┤
│ Frame 1         │ (Frame size: variable)
├─────────────────┤
│ Frame 2         │ (Frame size: variable)
├─────────────────┤
│ ...             │
└─────────────────┘
```

#### 5.2.2 Frame Structure

Each frame follows the structure compatible with `DUMMY_FRAME_STRUCT`:

```
┌─────────────────────┐
│ Frame Header        │
│ - timestamp (8 bytes)│
│ - another_key (8 bytes)│
├─────────────────────┤
│ Frame Data          │
│ - data[] (N bytes)  │
└─────────────────────┘
```

**Frame Size Calculation:**
- Header: 16 bytes (timestamp + another_key)
- Data: `num_channels * num_time_samples * sizeof(int16_t)` bytes
- Total: `16 + (num_channels * num_time_samples * 2)` bytes

**Typical Configuration:**
- For 64 channels (4 pipelines × 16 channels) with 256 time samples:
  - Frame size = 16 + (64 × 256 × 2) = 32,784 bytes

#### 5.2.3 Frame Data Layout

Frame data is organized as a 2D array: `data[time_sample][channel]`

- **Row-major order**: All channels for time sample 0, then all channels for time sample 1, etc.
- **Channel ordering**: Channels 0-15 (pipeline 0), 16-31 (pipeline 1), 32-47 (pipeline 2), 48-63 (pipeline 3)
- **Data type**: int16_t (2 bytes per sample, little-endian)

**Example for 4 channels, 3 time samples:**
```
Time 0: [ch0, ch1, ch2, ch3]
Time 1: [ch0, ch1, ch2, ch3]
Time 2: [ch0, ch1, ch2, ch3]
```

Stored as: `[ch0_t0, ch1_t0, ch2_t0, ch3_t0, ch0_t1, ch1_t1, ch2_t1, ch3_t1, ch0_t2, ch1_t2, ch2_t2, ch3_t2]`

### 5.3 TP Validation File Format (.val)

**Purpose**: Store expected `TriggerPrimitive` objects for validation.

#### 5.3.1 File Structure

```
┌─────────────────┐
│ File Header     │ (12 bytes) - Same as Section 1.2
├─────────────────┤
│ Frame Index 0   │ (4 bytes: uint32_t frame_index)
│ TP Count 0      │ (4 bytes: uint32_t num_tps)
│ TP Data 0       │ (num_tps * sizeof(TriggerPrimitive))
├─────────────────┤
│ Frame Index 1   │ (4 bytes: uint32_t frame_index)
│ TP Count 1      │ (4 bytes: uint32_t num_tps)
│ TP Data 1       │ (num_tps * sizeof(TriggerPrimitive))
├─────────────────┤
│ ...             │
└─────────────────┘
```

#### 5.3.2 TP Record Structure

For each validation frame:

| Offset | Size (bytes) | Type   | Name        | Description                          |
|--------|--------------|--------|-------------|--------------------------------------|
| 0      | 4            | uint32 | frame_index | Frame index this TP set belongs to    |
| 4      | 4            | uint32 | num_tps     | Number of TPs in this record         |
| 8      | N            | TP[]   | tps         | Array of TriggerPrimitive objects     |

**TP Serialization Format:**

TPs are stored using raw binary serialization of the `dunedaq::trgdataformats::TriggerPrimitive` struct. This means:

1. **Serialization Method**: Direct memory copy (`memcpy`-style) of the struct
2. **Byte Order**: Platform-dependent (little-endian on x86_64)
3. **Alignment/Padding**: Matches the struct's natural alignment (compiler-dependent)
4. **Field Order**: Matches the struct definition in `trgdataformats/TriggerPrimitive.hpp`
5. **Size**: Use `sizeof(dunedaq::trgdataformats::TriggerPrimitive)` at compile time

**Key Fields in TriggerPrimitive** (for reference, actual struct may have more):
- `time_start` (int64_t): Timestamp when TP starts
- `channel` (channel_t, typically uint32_t): Channel number
- `adc_peak` (int16_t): Peak ADC value
- `samples_over_threshold` (uint16_t): Number of samples over threshold

**TP Sorting Requirements:**

TPs must be sorted for deterministic comparison and file format consistency:
1. **Primary key**: `time_start` (ascending)
2. **Secondary key**: `channel` (ascending)
3. **Tertiary key**: `samples_over_threshold` (ascending)

Both `TPWriter` and `TPReader` must use the same sorting criteria. The `TPComparator` utility should also use this same ordering.

**Version and Magic Number:**

TP validation files use the same file header as other binary files (Section 1.2):
- **Magic Number**: `0x54504754` ("TPGT" in ASCII)
- **Version**: `0x010004` (1.0.4 in hex, matches tpglibs version)
- **Reserved**: `0x00000000` (4 bytes)

**Note**: The exact size and layout of `TriggerPrimitive` depends on the DUNE DAQ version. The test application must use `sizeof(dunedaq::trgdataformats::TriggerPrimitive)` at compile time, not hardcoded values. This ensures compatibility across DUNE DAQ versions.

#### 5.3.3 TP Comparison Logic

When validating TPs:
1. Read expected TPs for the validation frame index
2. Sort both expected and actual TPs using the same criteria (time_start, channel, samples_over_threshold)
3. Compare element-by-element:
   - `time_start` must match exactly
   - `channel` must match exactly
   - `adc_peak` must match exactly
   - `samples_over_threshold` must match exactly
   - Other fields as defined by TriggerPrimitive struct

### 5.4 TPGenerator Configuration File Format (.json)

**Purpose**: Configure TPGenerator and test parameters.

#### 5.4.1 Example Configuration

```json
{
  "processor_configs": [
    {
      "processor_name": "AVXThresholdProcessor",
      "config": {
        "plane0": 200,
        "plane1": 300,
        "plane2": 445
      }
    }
  ],
  "test_config": {
    "num_channels": 64,
    "num_time_samples": 256,
    "num_pipelines": 4,
    "sample_tick_difference": 1.0,
    "sot_minima": [1, 1, 1],
    "max_frames": 1000,
    "validation_frames": [0, 10, 100]
  },
  "channel_plane_mappings": [
    [0, 0], [1, 0], [2, 0], [3, 0], [4, 0], [5, 0], [6, 0], [7, 0],
    [8, 0], [9, 0], [10, 0], [11, 0], [12, 0], [13, 0], [14, 0], [15, 0],
    [16, 1], [17, 1], [18, 1], [19, 1], [20, 1], [21, 1], [22, 1], [23, 1],
    [24, 1], [25, 1], [26, 1], [27, 1], [28, 1], [29, 1], [30, 1], [31, 1],
    [32, 2], [33, 2], [34, 2], [35, 2], [36, 2], [37, 2], [38, 2], [39, 2],
    [40, 2], [41, 2], [42, 2], [43, 2], [44, 2], [45, 2], [46, 2], [47, 2],
    [48, 0], [49, 0], [50, 0], [51, 0], [52, 0],
    [53, 1], [54, 1], [55, 1], [56, 1], [57, 1],
    [58, 2], [59, 2], [60, 2], [61, 2], [62, 2], [63, 2]
  ]
}
```

#### 5.4.2 Configuration Fields

**processor_configs** (array, required):
- Array of processor configurations
- Each element contains:
  - `processor_name` (string): Name of the processor class (e.g., "AVXThresholdProcessor")
  - `config` (object): Processor-specific configuration parameters

**test_config** (object, required):
- `num_channels` (integer): Total number of channels (typically 64 for 4 pipelines)
- `num_time_samples` (integer): Number of time samples per frame (typically 256)
- `num_pipelines` (integer): Number of pipelines (typically 4, each handles 16 channels)
- `sample_tick_difference` (float): Number of ticks between time samples
- `sot_minima` (array of integers): Minimum samples over threshold per plane [plane0, plane1, plane2]
- `max_frames` (integer, optional): Maximum number of frames to process. If not provided, inferred from `validation_frames`
- `validation_frames` (array of integers): Frame indices to perform validation on

**channel_plane_mappings** (array, required):
- Array of `[channel_id, plane_number]` pairs
- Must contain exactly `num_channels` entries
- Channel IDs must be unique and in range [0, num_channels-1]

### 5.5 TPGenerator Test File Naming Convention

**Pattern:** `tpg_generator_[test_name]_[frames/val/config]`

**Examples:**
- `tpg_generator_sanity_frames.bin` - TPGenerator sanity test input frames
- `tpg_generator_sanity_val.val` - TPGenerator sanity test validation TPs
- `tpg_generator_sanity_config.json` - TPGenerator sanity test configuration

### 5.6 Usage Example

```cpp
// Read frame from input file
FrameReader reader("input_frames.bin");
auto frame = reader.next_frame();  // Returns frame object

// Process through TPGenerator
TPGenerator tpg;
tpg.configure(processor_configs, channel_plane_mappings, sample_tick_difference);
std::vector<TriggerPrimitive> tps = tpg(&frame);

// Validate against expected TPs
TPReader validation_reader("validation.val");
auto expected_tps = validation_reader.get_tps_for_frame(frame_index);
bool matches = compare_tps(tps, expected_tps);
```
