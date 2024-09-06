//          Copyright surrealwaffle 2024.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef PREDICTING_RANDOM_SOLVER_HPP
#define PREDICTING_RANDOM_SOLVER_HPP

// ---------------------------------------------------------------------------------
// SOLVER EXPLANATION
//
// If rand() defers to glibc, then the PRNG employed is either an LCG or a 
// modified LFSR, where the LSB is shifted out when emitting output. srand() 
// provided by glibc selects this PRNG by default (referred to as TYPE_3), and this
// solver targets this random number generator.
//
// Peter Selinger has a good explanation of how this PRNG works.
// https://www.mathstat.dal.ca/~selinger/random/
//
// The internal state of the LFSR updates according to the sequence
//  s_{i} := s_{i-3} + s_{i-31} (mod 2^32), for i >= 31.
// For 0 <= i < 31, s_{i} is populated according to the seed. (Note: this is a 
// conceptual simplification).
// Then certainly, modulo 2,
//  s_{i} = s_{i-3} + s_{i-31} (mod 2), for i >= 31.
//
// Mathematically, the output o_i is
//  o_{i} := (s_{i+T} - (s_{i+T} mod 2)) / 2,
// where T > 0 is employed to make the output less predictable. To make things 
// easier to keep track of, we will instead write this as:
//  o_{i} := (x_{i} - (x_{i} mod 2)) / 2,
// where x_{i} := s_{i+T}.
//
// It is quickly verified that
//  o_{i} = (o_{i-3} + o_{i-31} + (x_{i-3} mod 2) * (x_{i-31} mod 2) mod 2^31).
//
// In the event that last term is 1, it indicates three things:
//  x_{i-3}  = 1 (mod 2),
//  x_{i-31} = 1 (mod 2),
//  x_{i}    = 0 (mod 2).
//
// Furthermore, it is trivial to write (x_{i} mod 2) as a linear combination over 
// GF(2) of the initial system parities (s_{i} mod 2 for 0 <= i < 31). We can 
// therefore use such events to build a system of independent linear equations in 
// terms of the first 31 parity bits. Once those unknowns are solved for, they can 
// be used to determine the current parity bits. The current parity bits are enough 
// to reconstruct the internal state of the modified LFSR PRNG, because the PRNG 
// emits all but the first bit of the internal state.
//
// IMPLEMENTATION NOTES
// In order to maintain the list of equations, a specialized matrix type is used 
// which allows for querying about whether or not a candidate equation adds any 
// useful information about the system (see semicanonical_b32x32). The operations 
// involved allow for natural, incremental Gaussian elimination, which is further 
// taken advantage of to perform direct inversion (as opposed to matrix inversion
// and then a matrix-vector multiply) on an augmented matrix in which the 32nd 
// column is a coefficient corresponding to a constant `1`. That is, each row in 
// the matrix can be seen as representing an equation
//  c_0 * (s_0 mod 2) + ... + c_31 * (s_31 mod 2) + c_32 * 1 = 0 (mod 2).
//
// Ideally, I would also supply some bit vector type to represent the rows and thus
// differentiate rows from just `std::uint32_t`, but I'm not confident that this
// change would be worth it for this specific application.
//
// The implementation also fails spectacularly for seed 0, but glibc prevents this 
// seed from showing up anyways (and its output could be quickly detected anyways).
// ---------------------------------------------------------------------------------

#include <cassert>
#include <cstdint>

#include <optional>
#include <limits>

#include "cyclic_fixed_queue.hpp"
#include "prng.hpp"

/**
 * \brief A specialized type representing 32x32 matrices over GF(2) which maintains
 *        the matrix in row semi-canonical form - that is, pivots are always along
 *        the diagonal and zero rows are permitted to space the pivot rows.
 */
class semicanonical_b32x32
{
public:
  using row_type = std::uint32_t;
  static constexpr int size = std::numeric_limits<row_type>::digits;
  
  /**
   * \brief Constructs to the zero matrix (all elements `0`).
   */
  constexpr semicanonical_b32x32() noexcept : rows_{/*ZERO*/} {}
  
  /**
   * \brief Returns the column-wise sum of rows selected by \a select, modulo 2.
   */
  constexpr row_type row_sum(std::uint32_t select) const noexcept;
  
  /**
   * \brief Returns the row at \a index.
   */
  constexpr row_type operator[](int index) const noexcept { return rows_[index]; }
  
  /**
   * \brief Attempts to push \a row into the matrix.
   *
   * The row is pushed into the matrix if and only if it is not a linear combination
   * of rows present.
   *
   * \return \c trie if and only if \a row was pushed into the matrix.
   */
  constexpr bool push_row(row_type row) noexcept;
  
private:
  alignas(64 * sizeof(row_type)) std::array<row_type, size> rows_;
};

class solver
{
public:
  using generator_type = reference_generator; ///< The targeted generator type.
  using value_type = typename generator_type::result_type; 
  
  // -------------------------------------------------------------------------------
  // CONSTRUCTORS
  
  /** 
   * \brief Constructs to a solver that is ready to be fed output.
   */
  constexpr solver() noexcept;
  
  // -------------------------------------------------------------------------------
  // OBSERVERS
  
  // -------------------------------------------------------------------------------
  // MODIFIERS
  
  /**
   * \brief Feeds an output \a value from the PRNG 
   */
  [[nodiscard]] constexpr std::optional<generator_type> feed(value_type value) noexcept;
  
private:
  cyclic_fixed_queue<value_type, 31> history;    ///< Keeps track of recent values.
  cyclic_fixed_queue<std::uint32_t, 31> parity;  ///< Parities of recent states in
                                                 ///< terms of initial system
                                                 ///< parities.
  
  struct {
    int rank = 0; ///< The rank of #matrix.
    semicanonical_b32x32 matrix {};
    
    /**
     * \brief Records the equation 
     *        `Sum[p_i * c_i, {i in [0 .. 31]}] = parity (mod 2)`, where \c p_i
     *        is the initial system parity at step `i` and \c c_i is the `i`th bit
     *        of \a coefficients from the LSB, acting as a weight.
     *
     * \return \c true if the system of linear equations can be solved.
     */
    constexpr bool push(std::uint32_t coefficients, bool parity) noexcept
    {
      coefficients |= static_cast<std::uint32_t>(parity) << 31;
      return (rank += matrix.push_row(coefficients)) == 31;
    }
  } equations;
  
  /**
   * \brief Reconstructs the target generator.
   *
   * The system must be solvable, i.e. `equations.rank == 31`.
   *
   * \return A generator producing equivalent output to the one which fed the solver
   *         values.
   */
  [[nodiscard]] constexpr generator_type solve() noexcept;
  
  /**
   * \brief Reconstructs the current internal state parities of the target generator.
   *
   * The system must be solvable, i.e. `equations.rank == 31`.
   *
   * \return The internal state parities, ordered from oldest in the LSB to most
   *         recent in the MSB.
   */
  [[nodiscard]] constexpr std::uint32_t solve_parities() noexcept;
};

constexpr solver::solver() noexcept
{
  for (int i = 0; i < 31; ++i)
    parity.push(static_cast<std::uint32_t>(1uL << i));
  
  for (int i = 31; i < 34; ++i)
    parity.pop_and_push(std::uint32_t(parity.front()));
  
  for (int i = 34; i < 344; ++i)
    parity.pop_and_push(parity(-3) ^ parity(-31)); // addition, mod 2
}

constexpr auto solver::feed(value_type value) noexcept -> std::optional<generator_type>
{
  if (history.ssize() < 31) {
    history.push(value);
    parity.pop_and_push(parity(-3) ^ parity(-31));
  } else {
    const auto o31 = history(-31); // o_{i-31}
    const auto o3  = history(-3);  // o_{i-3}
    
    const auto q31 = parity(-31);  // p_{i-31} in terms of initial parities
    const auto q3  = parity(-3);   // p_{i-3}  in terms of initial parities
    const auto q0  = q31 ^ q3;
    
    history.pop_and_push(value);
    parity.pop_and_push(q0);
    
    const auto expected = (o31 + o3) % (1uL << 31);
    if (value != expected) {
      assert(value == (expected + 1uL));
      
      if (equations.push(q31, true) || equations.push(q3, true))
        return solve();
    }
  }
  
  return std::nullopt;
}

constexpr auto solver::solve() noexcept -> generator_type
{
  assert(equations.rank == 31);
  
  // equations.matrix.reduce();
  
  // parity_bits ordered from oldest (LSB) to most recent (MSB)
  auto table = history;
  for (auto parity_bits = solve_parities(); auto& state : table) {
    state = (state << 1) | (parity_bits & 1u);
    parity_bits >>= 1;
  }
  
  return generator_type{table};
}

constexpr std::uint32_t solver::solve_parities() noexcept
{
  assert(equations.rank == 31);
  
  std::uint32_t initial_state = 0;
  for (int i = 0; i < 32; ++i) {
    const auto row = equations.matrix[i];
    assert(std::popcount(row) <= 2);
    initial_state |= (row >> 31) << i; // last bit indicates parity
  }
  
  value_type result = 0;
  for (int i = 0; auto coefficients : parity)
    // this may look confusing, but it really is just multiplying the coefficients
    // by the initial state and summing them, modulo 2, then shifting the result 
    // into the i'th bit.
    result |= static_cast<value_type>(std::popcount(coefficients & initial_state) % 2) << i++;
  
  return result;
}

constexpr auto semicanonical_b32x32::row_sum(std::uint32_t select) const noexcept
  -> row_type
{
  // gcc: -O3 produces good throughput
  // clang: -O2 produces good throughput (O3 is regression)
  
  row_type result = 0;
  for (int i = 0; i < 32; ++i)
    result ^= !!(select & (static_cast<std::uint32_t>(1) << i)) * rows_[i];
  return result;
}

constexpr bool semicanonical_b32x32::push_row(row_type row) noexcept
{
  row ^= row_sum(row); // eliminate current pivots
  
  if (row == 0)
    return false; // nothing to add
  
  const int pivot = std::countr_zero(row);
  assert(0 <= pivot && pivot < 32);
  assert(rows_[pivot] == 0);
  
  // gaussian elimination to remove row from all the rows
  for (int i = 0; i < 32; ++i)
    rows_[i] ^= !!(rows_[i] & (static_cast<row_type>(1) << pivot)) * row;
  rows_[pivot] = row;
  
  return true;
}

#endif // PREDICTING_RANDOM_SOLVER_HPP