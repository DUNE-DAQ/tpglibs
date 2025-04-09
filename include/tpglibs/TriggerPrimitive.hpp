/**
 * @file TriggerPrimitive.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

 #ifndef TPGLIBS_INCLUDE_TPGLIBS_TRIGGERPRIMITIVE_HPP_
 #define TPGLIBS_INCLUDE_TPGLIBS_TRIGGERPRIMITIVE_HPP_
 
 #include "tpglibs/Types.hpp"
 
 #include <bitset>
 #include <cstddef>
 #include <cstdint>
 #include <iostream>
 #include <limits>
 #include <type_traits>
 
 namespace tpglibs {
 
 /**
  * @brief A single energy deposition on a TPC or PDS channel
  */
 struct TriggerPrimitive
 {
   static constexpr uint8_t s_trigger_primitive_version = 2;
 
   // Metadata.
   uint64_t version : 8;
   uint64_t flag : 8;
   uint64_t detid : 8;
 
   // Physics data.
   uint64_t channel : 24;
 
   uint64_t samples_over_threshold : 16;
   uint64_t time_start : 64;
   uint64_t samples_to_peak : 16;
 
   uint64_t adc_integral : 32;
   uint64_t adc_peak : 16;
 
   TriggerPrimitive()
     : version(s_trigger_primitive_version)
     , flag(0)
     , detid(INVALID_DETID)
     , channel(INVALID_TP_CHANNEL)
     , samples_over_threshold(INVALID_SAMPLES_OVER_THRESHOLD)
     , time_start(INVALID_TIMESTAMP)
     , samples_to_peak(INVALID_SAMPLES_TO_PEAK)
     , adc_integral(0)
     , adc_peak(0)
   {}
 };
 
 } // namespace tpglibs
 
 #endif // TPGLIBS_INCLUDE_TPGLIBS_TRIGGERPRIMITIVE_HPP_
