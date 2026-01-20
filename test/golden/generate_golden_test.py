#!/usr/bin/env python3
"""
Generate golden test data files for TPGenerator test application.

Generates:
- Input frames file (.bin) with 1-2 simple frames
- Validation file (.val) with expected TPs (or empty if no TPs expected)
- Config file (.json) for the test
"""

import struct
import json
import os
import sys

# Binary file format constants
MAGIC_NUMBER = 0x54504754  # "TPGT"
VERSION = 0x010004  # 1.0.4 in hex
HEADER_SIZE = 12  # magic (4) + version (4) + reserved (4)

# Frame constants
NUM_CHANNELS = 64
NUM_TIME_SAMPLES = 256
FRAME_HEADER_SIZE = 8  # timestamp (8)
FRAME_DATA_SIZE = NUM_CHANNELS * NUM_TIME_SAMPLES * 2  # int16_t = 2 bytes

def write_file_header(f):
    """Write binary file header to file."""
    f.write(struct.pack('<I', MAGIC_NUMBER))  # magic (little-endian)
    f.write(struct.pack('<I', VERSION))       # version
    f.write(struct.pack('<I', 0))             # reserved

def write_frame(f, frame_index, adc_data):
    """Write a frame to the frames file.
    
    Args:
        f: File handle
        frame_index: Frame index (for timestamp)
        adc_data: List of int16 values, length = NUM_CHANNELS * NUM_TIME_SAMPLES
                  Layout: row-major [time_sample][channel]
    """
    # Frame header
    timestamp = frame_index * 1000
    f.write(struct.pack('<Q', timestamp))      # timestamp (8 bytes, little-endian)
    
    # Frame data: write as int16_t array
    for value in adc_data:
        f.write(struct.pack('<h', value))  # int16_t, little-endian

def write_tps(f, frame_index, tps):
    """Write TPs to validation file.
    
    Args:
        f: File handle
        frame_index: Frame index
        tps: List of TP dictionaries with fields: time_start, channel, adc_peak, samples_over_threshold
    """
    num_tps = len(tps)
    
    # Write frame index (uint32_t)
    f.write(struct.pack('<I', frame_index))
    
    # Write TP count (uint32_t)
    f.write(struct.pack('<I', num_tps))
    
    # Write TPs
    # Note: This is a simplified version. The actual TriggerPrimitive struct
    # may have more fields. For a true golden test, we should serialize the
    # actual struct from C++ code.
    # For now, we write a minimal representation that matches the expected format.
    # The TP struct size needs to match sizeof(TriggerPrimitive) from C++.
    
    # TriggerPrimitive struct fields (approximate, may vary):
    # - time_start: int64_t (8 bytes)
    # - channel: uint32_t (4 bytes)  
    # - adc_peak: int16_t (2 bytes)
    # - samples_over_threshold: uint16_t (2 bytes)
    # - ... other fields (padding/alignment may apply)
    
    # For now, we'll write a simplified version. The actual struct size
    # should be obtained from C++ code. Let's assume a minimal size of 24 bytes
    # (this is a placeholder - actual size should match sizeof(TriggerPrimitive))
    TP_STRUCT_SIZE = 24  # Placeholder - should match actual struct size
    
    for tp in tps:
        # Write TP fields (simplified - actual struct may have different layout)
        # We write: time_start (8), channel (4), adc_peak (2), samples_over_threshold (2), padding (8)
        f.write(struct.pack('<q', tp['time_start']))  # int64_t
        f.write(struct.pack('<I', tp['channel']))     # uint32_t
        f.write(struct.pack('<h', tp['adc_peak']))    # int16_t
        f.write(struct.pack('<H', tp['samples_over_threshold']))  # uint16_t
        # Padding to reach TP_STRUCT_SIZE (8 bytes)
        f.write(struct.pack('<Q', 0))  # 8 bytes padding

def create_spike_pattern(threshold):
    """Create a simple spike pattern in ADC data.
    
    Creates a spike in channel 0, time samples 100-105 (above threshold).
    
    Returns:
        List of int16 values (NUM_CHANNELS * NUM_TIME_SAMPLES)
    """
    adc_data = [0] * (NUM_CHANNELS * NUM_TIME_SAMPLES)
    
    # Create a spike in channel 0, time samples 100-105
    # Layout: adc_data[time_sample * NUM_CHANNELS + channel]
    spike_value = threshold + 100  # Well above threshold
    for t in range(100, 106):  # 100-105 inclusive
        idx = t * NUM_CHANNELS + 0
        adc_data[idx] = spike_value
    
    return adc_data

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <output_directory>")
        print(f"Example: {sys.argv[0]} .")
        sys.exit(1)
    
    output_dir = sys.argv[1]
    
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    frames_file = os.path.join(output_dir, "tpg_generator_golden_frames.bin")
    validation_file = os.path.join(output_dir, "tpg_generator_golden_val.val")
    config_file = os.path.join(output_dir, "tpg_generator_golden_config.json")
    
    threshold = 200  # Threshold for plane 0
    
    print("Generating golden test data...")
    print(f"  Frames file: {frames_file}")
    print(f"  Validation file: {validation_file}")
    print(f"  Config file: {config_file}")
    
    # Generate frames file
    with open(frames_file, 'wb') as f:
        write_file_header(f)
        
        # Frame 0: Simple spike pattern (should produce TPs)
        frame0_data = create_spike_pattern(threshold)
        write_frame(f, 0, frame0_data)
        
        # Frame 1: All zeros (no TPs expected)
        frame1_data = [0] * (NUM_CHANNELS * NUM_TIME_SAMPLES)
        write_frame(f, 1, frame1_data)
    
    print("  Generated frames file: 2 frames")
    
    # Generate validation file
    with open(validation_file, 'wb') as f:
        write_file_header(f)
        
        # Frame 0: Expected TPs from the spike pattern
        # Note: These are placeholder values. For a true golden test,
        # run the test app first to capture actual TP output, then use those.
        frame0_tps = [
            {
                'time_start': 100,  # Starting time sample
                'channel': 0,        # Channel 0
                'adc_peak': threshold + 100,  # Peak ADC value
                'samples_over_threshold': 6   # Samples 100-105 (6 samples)
            }
        ]
        write_tps(f, 0, frame0_tps)
        
        # Frame 1: No TPs (empty)
        write_tps(f, 1, [])
    
    print("  Generated validation file: 2 frames (1 with TPs, 1 empty)")
    print("  WARNING: TP values are placeholders - verify by running test app")
    
    # Generate config file
    config = {
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
            "sample_tick_difference": 1.0,
            "sot_minima": [1, 1, 1],
            "validation_frames": [0, 1]
        },
        "channel_plane_mappings": []
    }
    
    # Generate 64 channel-plane mappings
    for ch in range(64):
        if ch < 16:
            plane = 0
        elif ch < 32:
            plane = 1
        elif ch < 48:
            plane = 2
        else:
            plane = 0
        config["channel_plane_mappings"].append([ch, plane])
    
    with open(config_file, 'w') as f:
        json.dump(config, f, indent=2)
    
    print("  Generated config file")
    print("Golden test data generation complete!")
    print("\nNext steps:")
    print("  1. Run the test app to verify it works")
    print("  2. If TP values differ, update validation file with actual output")
    print("  3. Use the updated validation file as the true golden reference")

if __name__ == '__main__':
    main()

