//          Copyright surrealwaffle 2024.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <cstdlib>

#include "prng.hpp"
#include "solver.hpp"

constexpr auto solve_prng(reference_generator& gen) noexcept
  -> std::pair<long long, reference_generator>
{
  long long steps = 0;
  solver s;
  
  std::optional<reference_generator> result;
  auto try_solve = [&] () -> bool {
    ++steps;
    return (result = s.feed(gen())).has_value();
  };
  for (result = std::nullopt; !try_solve();) {
    /* DO NOTHING */
  }
  
  return std::make_pair(steps, *result);
}

int main(int argc, char* argv[])
{
  if (argc != 2) {
    std::printf("usage: %s <seed>\n", argv[0]);
    return EXIT_FAILURE;
  }
  
  const unsigned long long seed = static_cast<unsigned long long>(std::atoll(argv[1]));
  std::printf("testing seed: %llu\n", seed);
  
  reference_generator gen{static_cast<reference_generator::result_type>(seed)};
  auto [steps, solved_gen] = solve_prng(gen);
  std::printf("solved in %lld samples\n", static_cast<long long>(steps));
  
  {
    const auto& src_table = gen.table();
    const auto& sol_table = solved_gen.table();
    
    std::printf("%3s %8s %8s\n", "pos", "source", "solved");
    for (int i = 0; i < 31; ++i) {
      std::printf("%3d %08lX %08lX\n",
        -(30 - i),
        static_cast<unsigned long>(src_table(-30 + i)),
        static_cast<unsigned long>(sol_table(-30 + i))
      );
    }
  }
  
  if (gen != solved_gen) {
    std::printf("generator was not correctly solved for this seed");
    return EXIT_FAILURE;
  }
  
  std::printf("generator from seed %llu was solved\n", seed);
  return EXIT_SUCCESS;
}