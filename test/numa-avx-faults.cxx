#define FMT_HEADER_ONLY

#include <immintrin.h>
#include <numa.h>
#include <utility>
#include <fmt/core.h>
#include <cstdint>

void
_mm256_print_epi16(__m256i& A) {
    uint16_t printout[16];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(printout), A);
    fmt::print("{:3}\n", fmt::join(printout, ", "));
    return;
}

struct alignas(8) thing {
  char data[7200];
};

int main(void)
{
  numa_set_preferred(0);
  numa_set_bind_policy(1);
  numa_set_strict(1);
  size_t num_things = 4096;
  size_t thing_size = 7200;
  thing* records = static_cast<thing*>(numa_alloc_onnode(sizeof(thing)*num_things, 0));

  fmt::print("Filling.\n");
  for (int i = 0; i < thing_size; i+=2) {
    for (int j = 0; j < num_things; j++) {
      uint16_t* in = reinterpret_cast<uint16_t*>(records[j].data+i);
      (*in) = j*10 + i / 2;
    }
  }

//  for (int i = thing_size - 32; i < thing_size; i+=2) {
//    int16_t* out = reinterpret_cast<int16_t*>(records[4096].data + i);
//    fmt::print("Out: {}\n", (*out));
//  }

  fmt::print("Reading.\n");
  char* cursor = reinterpret_cast<char*>(records[0].data);
  cursor += num_things * thing_size - 31;
  __m256i regi = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(cursor));
  _mm256_print_epi16(regi);
  return 0;
}
