//          Copyright surrealwaffle 2024.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <memory>

#include "prng.hpp"

constexpr int reference_buffer_offset = 344;
std::unique_ptr<std::uint32_t[]> reference_implementation(unsigned long seed, long long count) noexcept;

int main(int argc, char* argv[])
{
  if (argc < 3) {
    std::printf("Usage: %s <seed> <count>\n", argv[0]);
    return EXIT_FAILURE;
  }
  
  const auto seed = static_cast<unsigned long>(std::atoll(argv[1]));
  const auto count = std::atoll(argv[2]);
  if (count < 0)
    return EXIT_SUCCESS;
  
  reference_generator gen(static_cast<reference_generator::result_type>(seed));
  const auto reference_buffer = reference_implementation(seed, count);
  
  for (long long i = 0; i < count; ++i) {
    if (i < 64)
        std::printf("[%02lld] = %010lld | %lld\n", i, gen.peek_state(), gen.peek_state() % 2);
    
    const auto expected_value = reference_buffer[i + reference_buffer_offset] >> 1;
    const auto generated_value = static_cast<long long>(gen());
    
    if (expected_value != generated_value) {
      std::printf("Mismatch from [%lld]: got %lld, expected %lld\n", 
        i,
        generated_value, expected_value);
      return EXIT_FAILURE;
    }
  }
  
  assert(gen == gen);
  assert(!(gen != gen));
  
  std::puts("All tested values matched the reference implementation");
  return EXIT_SUCCESS;
}

std::unique_ptr<std::uint32_t[]> reference_implementation(unsigned long seed, long long count) noexcept
{
  assert(count >= 0);
  count += reference_buffer_offset;
  std::unique_ptr<std::uint32_t[]> result{new std::uint32_t[count]};
  
  result[0] = static_cast<std::uint32_t>(seed);
  for (int i = 1; i < 31; ++i) {
    auto value = (16807LL * result[i - 1]) % 2147483647;
    if (value < 0)
        value += 2147483647;
    result[i] = static_cast<std::uint32_t>(value);
  }
  
  for (int i = 31; i < 34; ++i)
      result[i] = result[i - 31];
  
  for (int i = 34; i < count; ++i)
      result[i] = result[i - 3] + result[i - 31];
  
  return result;
}