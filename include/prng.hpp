//          Copyright surrealwaffle 2024.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef PREDICTING_RANDOM_PRNG_HPP
#define PREDICTING_RANDOM_PRNG_HPP

#include <cstdint>

#include <limits>

#include "cyclic_fixed_queue.hpp"

/**
 * \brief An implementation of a PRNG used in glibc as described here:
 *        https://www.mathstat.dal.ca/~selinger/random/
 *
 * This type satisfies the concept `std::uniform_random_bit_generator`.
 */
class reference_generator
{
public:
  using result_type = std::uint32_t;
  using table_type  = cyclic_fixed_queue<result_type, 31>;
  
  static constexpr result_type min() noexcept { return std::numeric_limits<result_type>::min(); }
  static constexpr result_type max() noexcept { return std::numeric_limits<result_type>::max() >> 1; }
  
  // -------------------------------------------------------------------------------
  // CONSTRUCTORS
  
  /**
   * \brief Initializes the PRNG using \a seed.
   */
  constexpr reference_generator(result_type seed) noexcept : queue_(table_from_seed(seed))
  {
    // queue_ contains initial state at this point
    // advance the PRNG to make the output less predictable
    for (int i = 34; i < 344; ++i)
      advance();
  }
  
  /**
   * \brief Initializes the internal state directly from \a table.
   */
  constexpr reference_generator(const table_type& table) noexcept : queue_(table) {}
  
  // -------------------------------------------------------------------------------
  // OBSERVERS
  
  friend constexpr bool operator==(const reference_generator&, const reference_generator&) = default;
  
  /**
   * \brief Returns the next internal state value.
   */
  [[nodiscard]] constexpr result_type peek_state() const noexcept { return queue_(-3) + queue_(-31); }
  
  /**
   * \brief Returns the next output value.
   */
  [[nodiscard]] constexpr result_type peek() const noexcept { return peek_state() >> 1; }
  
  /**
   * \brief Returns a reference to the internal state.
   */
  [[nodiscard]] constexpr const table_type& table() const noexcept { return queue_; }
  
  // -------------------------------------------------------------------------------
  // MODIFIERS
  
  /**
   * \brief Generates a pseudo-random value, advancing the state by one position.
   */
  constexpr result_type advance() noexcept { return queue_.pop_and_push(peek_state()) >> 1; }
  
  /**
   * \brief Generates a pseudo-random value, advancing the state by one position.
   */
  constexpr result_type operator()() noexcept { return advance(); }

private:
  table_type queue_;
  
  /**
   * \brief Returns internal state for a generator using \a seed.
   */
  [[nodiscard]] static constexpr table_type table_from_seed(result_type seed) noexcept;
};

constexpr auto reference_generator::table_from_seed(result_type seed) noexcept
  -> table_type
{
  table_type result;
  result.push(seed);
  for (int i = 1; i < 31; ++i) {
    // cast here is necessary to maintain the sign of the operation
    auto value = (16807LL * static_cast<std::int32_t>(result.back())) % 2147483647;
    if (value < 0)
      value += 2147483647;
    result.push(static_cast<result_type>(value));
  }
  
  for (int i = 31; i < 34; ++i)
    result.pop_and_push(result_type(result.front()));
  
  return result;
}

#endif // PREDICTING_RANDOM_PRNG_HPP