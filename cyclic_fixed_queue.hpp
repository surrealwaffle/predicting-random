//          Copyright surrealwaffle 2024.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cassert>
#include <cstddef>

#include <algorithm>
#include <array>
#include <concepts>
#include <iterator>
#include <memory>
#include <utility>

template<typename BidirIt>
class wrap_around_iterator
{
public:
  using difference_type = typename std::iterator_traits<BidirIt>::difference_type;
  using value_type = typename std::iterator_traits<BidirIt>::value_type;
  using pointer = typename std::iterator_traits<BidirIt>::pointer;
  using reference = typename std::iterator_traits<BidirIt>::reference;
  using iterator_category = std::bidirectional_iterator_tag;
  
  friend constexpr bool operator==(const wrap_around_iterator& lhs, const wrap_around_iterator& rhs) noexcept
  {
    return lhs.it == rhs.it;
  }
  
  wrap_around_iterator() = default;
  
  explicit constexpr wrap_around_iterator(BidirIt it, BidirIt last, BidirIt reset) noexcept
    : it(it), last(last), reset(reset) {}
  
  constexpr reference operator*() noexcept { return *it; }
  
  constexpr wrap_around_iterator& operator++() noexcept {
    if (++it == last)
      it = reset;
    return *this;
  }
  
  constexpr wrap_around_iterator operator++(int) noexcept {
    auto copy = *this;
    ++(*this);
    return copy;
  }
  
  constexpr wrap_around_iterator& operator--() noexcept {
    if (it == reset)
      it = std::prev(last);
    return *this;
  }
  
  constexpr wrap_around_iterator operator--(int) noexcept {
    auto copy = *this;
    --(*this);
    return copy;
  }

private:
  BidirIt it, last, reset;
};

/**
 * \brief Provides a cylic, first-in-first-out queue over storage of fixed capacity.
 *
 * Unlike other containers like `std::vector`, all storage elements are constructed
 * and destroyed with the queue. If \a T is not trivially destructible, elements 
 * are popped by assigning them the default-constructed value.
 */
template<std::semiregular T, std::size_t Capacity>
class cyclic_fixed_queue
{
public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  
  using iterator = wrap_around_iterator<pointer>;
  using const_iterator = wrap_around_iterator<const_pointer>;
  
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // -------------------------------------------------------------------------------
  // CONSTRUCTORS
  
  /**
   * \brief Constructs to the empty queue.
   */
  cyclic_fixed_queue() = default;
  
  /**
   * \brief Constructs the queue populated with `[first, last)`.
   */
  template<typename ForwardIt>
  explicit constexpr cyclic_fixed_queue(ForwardIt first, ForwardIt last)
  {
    assert(std::cmp_less_equal(std::distance(first, last), Capacity));
    for (; first != last; ++first)
      push(*first);
  }
  
  // -------------------------------------------------------------------------------
  // OBSERVERS
  
  friend constexpr bool operator==(const cyclic_fixed_queue& lhs, const cyclic_fixed_queue& rhs)
  {
    if (lhs.size() != rhs.size())
      return false;
    
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
  }
  
  /**
   * \brief Returns a reference to the first element in the queue.
   */
  [[nodiscard]] constexpr reference front() noexcept {
    assert(!empty());
    return storage_[front_];
  }
  
  /**
   * \brief Returns a reference to the first element in the queue.
   */
  [[nodiscard]] constexpr const_reference front() const noexcept {
    assert(!empty());
    return storage_[front_];
  }
  
  /**
   * \brief Returns a reference to the last element in the queue.
   */
  [[nodiscard]] constexpr reference back() noexcept {
    assert(!empty());
    return storage_[(front_ + ssize() - 1) % std::ssize(storage_)];
  }
  
  /**
   * \brief Returns a reference to the last element in the queue.
   */
  [[nodiscard]] constexpr const_reference back() const noexcept {
    assert(!empty());
    return storage_[(front_ + ssize() - 1) % std::ssize(storage_)];
  }
  
  /**
   * \brief Returns a reference to the index at a relative \a offset.
   *
   * If \a offset is negative, indexing is relative to the end of the queue, where 
   * an \a offset of `-1` refers to the `back()`. Otherwise, \a offset is taken 
   * relative to the front of the queue.
   */
  [[nodiscard]] constexpr reference operator()(difference_type offset) noexcept
  {
    if (offset < 0) {
      assert(ssize() + offset >= 0);
      return storage_[(front_ + ssize() + offset) % std::ssize(storage_)];
    } else {
      assert(ssize() >= offset);
      return storage_[(front_ + offset) % std::ssize(storage_)];
    }
  }
  
  /**
   * \brief Returns a reference to the index at a relative \a offset.
   *
   * If \a offset is negative, indexing is relative to the end of the queue, where 
   * an \a offset of `-1` refers to the `back()`. Otherwise, \a offset is taken 
   * relative to the front of the queue.
   */
  [[nodiscard]] constexpr const_reference operator()(difference_type offset) const noexcept
  {
    if (offset < 0)
      return storage_[(front_ + ssize() + offset) % std::ssize(storage_)];
    else
      return storage_[(front_ + offset) % std::ssize(storage_)];
  }
  
  /**
   * \brief Returns the number of elements in the queue.
   */
  [[nodiscard]] constexpr size_type ssize() const noexcept { return size_; }
  
  /**
   * \brief Returns the number of elements in the queue.
   */
  [[nodiscard]] constexpr size_type size() const noexcept { return static_cast<size_type>(size_); }
  
  /**
   * \brief Returns \c true if and only if the queue is empty.
   */
  [[nodiscard]] constexpr bool empty() const noexcept { return size() <= 0; }
  
  // -------------------------------------------------------------------------------
  // ITERATORS
  
  /**
   * \brief Returns an iterator to the front of the queue.
   */
  [[nodiscard]] constexpr iterator begin() noexcept
  {
    const auto reset = storage_.data();
    const auto last = reset + std::ssize(storage_);
    return iterator{std::addressof(front()), last, reset};
  }
  
  /**
   * \brief Returns an iterator to the front of the queue.
   */
  [[nodiscard]] constexpr const_iterator begin() const noexcept
  {
    const auto reset = storage_.data();
    const auto last = reset + std::ssize(storage_);
    return const_iterator{std::addressof(front()), last, reset};
  }
  
  /**
   * \brief Returns an iterator to the front of the queue.
   */
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept
  {
    const auto reset = storage_.data();
    const auto last = reset + std::ssize(storage_);
    return const_iterator{std::addressof(front()), last, reset};
  }
  
  /**
   * \brief Returns an iterator to the end of the queue.
   */
  [[nodiscard]] constexpr iterator end() noexcept
  {
    const auto reset = storage_.data();
    const auto last = reset + std::ssize(storage_);
    return iterator{std::next(std::addressof(back())), last, reset};
  }
  
  /**
   * \brief Returns an iterator to the end of the queue.
   */
  [[nodiscard]] constexpr const_iterator end() const noexcept
  {
    const auto reset = storage_.data();
    const auto last = reset + std::ssize(storage_);
    return const_iterator{std::next(std::addressof(back())), last, reset};
  }
  
  /**
   * \brief Returns an iterator to the end of the queue.
   */
  [[nodiscard]] constexpr const_iterator cend() const noexcept
  {
    const auto reset = storage_.data();
    const auto last = reset + std::ssize(storage_);
    return const_iterator{std::next(std::addressof(back())), last, reset};
  }
  
  /**
   * \brief Returns an iterator to the front of the reversed queue.
   */
  [[nodiscard]] constexpr reverse_iterator rbegin() noexcept
  {
    return reverse_iterator{end()};
  }
  
  /**
   * \brief Returns an iterator to the front of the reversed queue.
   */
  [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
  {
    return const_reverse_iterator{end()};
  }
  
  /**
   * \brief Returns an iterator to the front of the reversed queue.
   */
  [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
  {
    return const_reverse_iterator{end()};
  }
  
  /**
   * \brief Returns an iterator to the end of the reversed queue.
   */
  [[nodiscard]] constexpr reverse_iterator rend() noexcept
  {
    return reverse_iterator{begin()};
  }
  
  /**
   * \brief Returns an iterator to the end of the reversed queue.
   */
  [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
  {
    return const_reverse_iterator{begin()};
  }
  
  /**
   * \brief Returns an iterator to the end of the reversed queue.
   */
  [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
  {
    return const_reverse_iterator{begin()};
  }
  
  // -------------------------------------------------------------------------------
  // MODIFIERS
  
  /**
   * \brief Pushes the element \a value to the end of the queue.
   *
   * This operation invalidates iterators on this queue.
   */
  constexpr reference push(const value_type& value) 
  {
    assert(std::cmp_less(size(), Capacity));
    return storage_[(front_ + size_++) % std::ssize(storage_)] = value;
  }
  
  /**
   * \brief Pushes the element \a value to the end of the queue.
   *
   * This operation invalidates iterators on this queue.
   */
  constexpr reference push(value_type&& value) 
  {
    assert(std::cmp_less(size(), Capacity));
    return storage_[(front_ + size_++) % std::ssize(storage_)] = std::move(value);
  }
  
  /**
   * \brief Pops the first element off the queue.
   *
   * If \a T is not trivially destructible, the popped element is 'destroyed' by 
   * assigning it the default-constructed value.
   *
   * This operation invalidates iterators on this queue.
   */
  constexpr void pop() noexcept
  {
    assert(!empty());
    
    if constexpr (!std::is_trivially_destructible_v<T>) {
      // clear the front by assigning it the default-constructed value
      front() = T();
    }
    
    if (++front_ == std::ssize(storage_))
      front_ = 0;
    --size_;
  }
  
  /**
   * \brief Pops the first element off the queue and adds \a value to the end of 
   *        the queue.
   *
   * This operation invalidates iterators on this queue.
   */
  constexpr reference pop_and_push(const value_type& value)
  {
    pop();
    return push(value);
  }
  
  /**
   * \brief Pops the first element off the queue and adds \a value to the end of 
   *        the queue.
   *
   * This operation invalidates iterators on this queue.
   */
  constexpr reference pop_and_push(value_type&& value)
  {
    pop();
    return push(std::move(value));
  }

private:
  std::array<T, Capacity + 1 /*SENTINEL*/> storage_;
  difference_type front_ = 0;
  difference_type size_ = 0;
};