//          Copyright surrealwaffle 2024.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

// USAGE: (program) <SEED>
// Attempts to reconstruct a generator following glibc random() given non-zero SEED.
// 
// When reconstructed, the state table of both the reference generator and the 
// reconstructed generator are output for manual verification.

#include <cstdio>
#include <cstdlib>

#include <functional>

#include "prng.hpp"
#include "solver.hpp"

namespace
{
  struct reconstruction_result
  {
    long long           steps; ///< The number of values fed to the solver.
    reference_generator gen;   ///< The reconstructed generator.
  };
  
  /**
   * \brief Reconstructs a #reference_generator from its output.
   *
   * \param [in] gen The function that provides the output of the generator to 
   *                 reconstruct.
   */
  reconstruction_result reconstruct_prng(
    std::function<reference_generator::result_type()> gen) noexcept;
}

int main(int argc, char* argv[])
{
  if (argc != 2) {
    std::printf("usage: %s <seed>\n", argv[0]);
    return EXIT_FAILURE;
  }
  
  const unsigned long long seed = static_cast<unsigned long long>(std::atoll(argv[1]));
  if (seed == 0)
  {
    std::printf("%s\n", "Please provide a non-zero seed");
    return EXIT_FAILURE;
  }
  
  std::printf("testing seed: %llu\n", seed);
  reference_generator gen{static_cast<reference_generator::result_type>(seed)};
  auto [steps, solved_gen] = reconstruct_prng([&gen] { return gen(); });
  std::printf(
    "%s generator from seed %llu\n",
    gen != solved_gen ? "failed to reconstruct" : "reconstructed",
    seed);
  std::printf("from %lld samples\n", static_cast<long long>(steps));
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
  
  if (gen != solved_gen)
    return EXIT_FAILURE;
  
  return EXIT_SUCCESS;
}

namespace
{
  reconstruction_result reconstruct_prng(
    std::function<reference_generator::result_type()> gen) noexcept
  {
    long long steps = 0;
    std::optional<reference_generator> result;
    for (solver s; !result; result = s.feed(gen()))
    {
      ++steps;
    }
    
    return reconstruction_result{.steps = steps, .gen = *result};
  }
}