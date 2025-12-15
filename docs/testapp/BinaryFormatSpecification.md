# Binary File Format Specification

## Overview

This document specifies the binary file formats used by:
- `test_tpg_processor_app.cxx` - TPG Processor test application
- `test_tpg_generator_app.cxx` - TPGenerator test application

## 1. Common File Header

All binary files start with a 12-byte header:

| Offset | Size (bytes) | Type   | Name         | Description                          |
|--------|--------------|--------|--------------|--------------------------------------|
| 0      | 4            | uint32 | magic_number | Magic number: `0x54504754` ("TPGT")  |
| 4      | 4            | uint32 | version      | Format version: `0x010004` (1.0.4)   |
| 8      | 4            | uint32 | reserved     | Reserved for future use              |

**C++ Structure:**
```cpp
struct BinaryFileHeader {
  uint32_t magic_number;
  uint32_t version;
  uint32_t reserved;
};
```

## 2. Processor Test Binary Format

### 2.1 Input File Format (.bin)

**Purpose**: Store time step data for processor testing.

**File Structure:**
```
┌─────────────────┐
│ File Header     │ (12 bytes) - See Section 1
├─────────────────┤
│ Data Block 0    │ (samples_per_time_step × 2 bytes)
├─────────────────┤
│ Data Block 1    │ (samples_per_time_step × 2 bytes)
├─────────────────┤
│ Data Block 2    │ (samples_per_time_step × 2 bytes)
├─────────────────┤
│ ...             │
└─────────────────┘
```

**Data Format:**
- **Data Type**: `int16_t` (2 bytes per sample)
- **Byte Order**: Little-endian
- **Block Size**: `samples_per_time_step × sizeof(int16_t)` bytes per block
- **Layout**: Sequential blocks, each containing `samples_per_time_step` consecutive `int16_t` values

### 2.2 Validation File Format (.bin)

**Purpose**: Store expected processor outputs for validation steps.

**File Structure:**
```
┌─────────────────┐
│ File Header     │ (12 bytes) - See Section 1
├─────────────────┤
│ Data Block 0    │ (samples_per_time_step × 2 bytes)
├─────────────────┤
│ Data Block 1    │ (samples_per_time_step × 2 bytes)
├─────────────────┤
│ Data Block 2    │ (samples_per_time_step × 2 bytes)
├─────────────────┤
│ ...             │
└─────────────────┘
```

**Data Format:**
- **Data Type**: `int16_t` (2 bytes per sample)
- **Byte Order**: Little-endian
- **Block Size**: `samples_per_time_step × sizeof(int16_t)` bytes per block
- **Layout**: Sequential blocks, where block N corresponds to step N
- **Access**: Validation data for step `target_step` is located at offset:
  ```
  offset = 12 + (target_step × samples_per_time_step × 2)
  ```

### 2.3 Configuration File Format (.json)

**Purpose**: Configure processor and test parameters.

**Structure:**
```json
{
  "processor_config": {
    /* Processor-specific parameters */
  },
  "test_config": {
    "processor_name": "string",
    "samples_per_time_step": integer,
    "max_steps": integer,
    "validation_steps": [integer, ...]
  }
}
```

**Fields:**
- **processor_config** (object, required): Processor-specific configuration parameters
- **test_config.processor_name** (string, required): Processor class name
- **test_config.samples_per_time_step** (integer, required): Number of samples per time step (typically 16)
- **test_config.max_steps** (integer, optional): Maximum number of processing steps
- **test_config.validation_steps** (array of integers, optional): Step indices to perform validation

## 3. TPGenerator Test Binary Format

### 3.1 Frame Input File Format (.bin)

**Purpose**: Store frame data to be processed by TPGenerator.

**File Structure:**
```
┌─────────────────┐
│ File Header     │ (12 bytes) - See Section 1
├─────────────────┤
│ Frame 0         │ (32,784 bytes for 64×256 frames)
├─────────────────┤
│ Frame 1         │ (32,784 bytes)
├─────────────────┤
│ Frame 2         │ (32,784 bytes)
├─────────────────┤
│ ...             │
└─────────────────┘
```

**Frame Structure:**
```
┌─────────────────────┐
│ Frame Header        │ (16 bytes)
│ - timestamp (8)     │ uint64_t: Frame timestamp
│ - another_key (8)   │ uint64_t: Additional header field
├─────────────────────┤
│ Frame Data          │ (32,768 bytes for 64×256)
│ - data[]            │ int16_t array: ADC samples
└─────────────────────┘
```

**Frame Size Calculation:**
- Header: 16 bytes (8 bytes timestamp + 8 bytes another_key)
- Data: `num_channels × num_time_samples × sizeof(int16_t)` bytes
- Total: `16 + (num_channels × num_time_samples × 2)` bytes

**Fixed Configuration:**
- 64 channels (4 pipelines × 16 channels)
- 256 time samples per frame
- Frame size: `16 + (64 × 256 × 2) = 32,784` bytes

**Frame Data Layout:**
- **Data Type**: `int16_t` (2 bytes per sample, little-endian)
- **ADC Value Range**: Values must be in 14-bit range [0, 16383] to match TPGenerator expectations
  - Values exceeding this range will be clamped to [0, 16383] during frame creation
  - Storage is 16-bit `int16_t`, but valid values are constrained to 14-bit range
- **Organization**: Row-major order - all channels for time sample 0, then all channels for time sample 1, etc.
- **Channel Ordering**: Channels 0-15 (pipeline 0), 16-31 (pipeline 1), 32-47 (pipeline 2), 48-63 (pipeline 3)

**Example** (4 channels, 3 time samples):
```
Time 0: [ch0, ch1, ch2, ch3]
Time 1: [ch0, ch1, ch2, ch3]
Time 2: [ch0, ch1, ch2, ch3]
```
Stored as: `[ch0_t0, ch1_t0, ch2_t0, ch3_t0, ch0_t1, ch1_t1, ch2_t1, ch3_t1, ch0_t2, ch1_t2, ch2_t2, ch3_t2]`

### 3.2 TP Validation File Format (.val)

**Purpose**: Store expected `TriggerPrimitive` objects for validation.

**File Structure:**
```
┌─────────────────┐
│ File Header     │ (12 bytes) - See Section 1
├─────────────────┤
│ Frame Index 0   │ (4 bytes: uint32_t)
│ TP Count 0      │ (4 bytes: uint32_t)
│ TP Data 0       │ (num_tps × sizeof(TriggerPrimitive))
├─────────────────┤
│ Frame Index 1   │ (4 bytes: uint32_t)
│ TP Count 1      │ (4 bytes: uint32_t)
│ TP Data 1       │ (num_tps × sizeof(TriggerPrimitive))
├─────────────────┤
│ ...             │
└─────────────────┘
```

**TP Record Structure:**

For each validation frame:

| Offset | Size (bytes) | Type   | Name        | Description                          |
|--------|--------------|--------|-------------|--------------------------------------|
| 0      | 4            | uint32 | frame_index | Frame index this TP set belongs to    |
| 4      | 4            | uint32 | num_tps     | Number of TPs in this record         |
| 8      | N            | TP[]   | tps         | Array of TriggerPrimitive objects    |

**TP Serialization Format:**

TPs are stored using raw binary serialization of the `dunedaq::trgdataformats::TriggerPrimitive` struct:

1. **Serialization Method**: Direct memory copy (`memcpy`-style) of the struct
2. **Byte Order**: Platform-dependent (little-endian on x86_64)
3. **Alignment/Padding**: Matches the struct's natural alignment (compiler-dependent)
4. **Field Order**: Matches the struct definition in `trgdataformats/TriggerPrimitive.hpp`
5. **Size**: Use `sizeof(dunedaq::trgdataformats::TriggerPrimitive)` at compile time

**Key Fields in TriggerPrimitive** (for reference):
- `time_start` (int64_t): Timestamp when TP starts
- `channel` (channel_t, typically uint32_t): Channel number
- `adc_peak` (int16_t): Peak ADC value
- `samples_over_threshold` (uint16_t): Number of samples over threshold

**Note**: The exact size and layout of `TriggerPrimitive` depends on the DUNE DAQ version. The test application uses `sizeof(dunedaq::trgdataformats::TriggerPrimitive)` at compile time.

**TP Sorting Requirements:**

In the test application, TPs are sorted for deterministic file format consistency:
1. **Primary key**: `time_start` (ascending)
2. **Secondary key**: `channel` (ascending)
3. **Tertiary key**: `samples_over_threshold` (ascending)

### 3.3 Configuration File Format (.json)

**Purpose**: Configure TPGenerator and test parameters.

**Structure:**
```json
{
  "processor_configs": [
    {
      "processor_name": "string",
      "config": {
        /* Processor-specific parameters */
      }
    }
  ],
  "test_config": {
    "num_channels": integer,
    "num_time_samples": integer,
    "num_pipelines": integer,
    "sample_tick_difference": float,
    "sot_minima": [integer, integer, integer],
    "max_frames": integer,
    "validation_frames": [integer, ...]
  },
  "channel_plane_mappings": [
    [integer, integer],
    ...
  ]
}
```

**Fields:**

**processor_configs** (array, required):
- Array of processor configurations
- Each element contains:
  - `processor_name` (string): Processor class name
  - `config` (object): Processor-specific configuration parameters

**test_config** (object, required):
- `num_channels` (integer): Total number of channels (typically 64)
- `num_time_samples` (integer): Number of time samples per frame (typically 256)
- `num_pipelines` (integer): Number of pipelines (typically 4)
- `sample_tick_difference` (float): Number of ticks between time samples
- `sot_minima` (array of 3 integers): Minimum samples over threshold per plane [plane0, plane1, plane2]
- `max_frames` (integer, optional): Maximum number of frames to process
- `validation_frames` (array of integers): Frame indices to perform validation on

**channel_plane_mappings** (array, required):
- Array of `[channel_id, plane_number]` pairs
- Must contain exactly `num_channels` entries
- Channel IDs must be unique and in range [0, num_channels-1]
