#pragma once

#include <atomic>
#include <array>
#include <cstddef>
#include <optional>
#include <new>

// capacity must be a power of 2 to replace expensive modulo operations with bitwise and &
template <typename T, size_t Capacity>
class RingBuffer {
	static_assert((Capacity > 1) && ((Capacity& (Capacity - 1)) == 0), "Capacity must be a power of 2!");
public:
	RingBuffer() : m_tail(0), m_head(0) {}

	~RingBuffer() {
		T discarded;
		while (pop(discarded)) {}
	}

	// delete to prevent slicing or thread hazards
	RingBuffer(const RingBuffer&) = delete;
	RingBuffer& operator=(const RingBuffer&) = delete;

	//called by producer thread
	template<typename... Args>
	[[nodiscard]] bool emplace(Args&&... args) noexcept {
		const size_t current_tail = m_tail.load(std::memory_order_relaxed);
		const size_t current_head = m_head.load(std::memory_order_acquire);

		// check if queue is full
		if (current_tail - current_head >= Capacity) {
			return false;
		}

		// construct object in place in circular storage
		const size_t index = current_tail & BUFFER_MASK;
		new (&m_storage[index].storage) T(std::forward<Args>(args)...);

		// publish update to consumer 
		m_tail.store(current_tail + 1, std::memory_order_release);
		return true;
	}

	// called by producer thread
	[[nodiscard]] bool push(const T& item) noexcept {
		return emplace(item);
	}

	[[nodiscard]] bool push(T&& item) noexcept {
		return emplace(std::move(item));
	}

	//called by consumer thread
	[[nodiscard]] bool pop(T& out_item) noexcept {
		const size_t current_head = m_head.load(std::memory_order_relaxed);
		const size_t current_tail = m_tail.load(std::memory_order_acquire);

		if (current_head == current_tail) {
			return false;
		}

		// convert absolute counter into a valid array index
		const size_t index = current_head & BUFFER_MASK;


		// point to the raw bytes and treat them as object of type T
		auto* ptr = reinterpret_cast<T*>(&m_storage[index].storage);
		out_item = std::move(*ptr); // crossed from ring buffer to thread's local variable

		ptr->~T();

		m_head.store(current_head + 1, std::memory_order_release);
		return true;
	}

	[[nodiscard]] bool empty() noexcept {
		return m_head.load(std::memory_order_relaxed) == m_tail.load(std::memory_order_relaxed);
	}

	[[nodiscard]] size_t size() noexcept {
		size_t head = m_head.load(std::memory_order_relaxed);
		size_t tail = m_tail.load(std::memory_order_relaxed);

		return (tail >= head) ? (tail - head) : 0;
	}



private:
	static constexpr size_t BUFFER_MASK = Capacity - 1;

	// Type-safe uninitialized storage slot for T
	struct Node {
		alignas(alignof(T)) std::byte storage[sizeof(T)];
	};

	// buffer array
	std::array<Node, Capacity> m_storage;

	// put read and write head on seperate 64-byte cache lines to avoid false sharing 
	alignas(64) std::atomic<size_t> m_tail{ 0 }; // Written by Producer, read by Consumer
	alignas(64) std::atomic<size_t> m_head{ 0 }; // Written by Consumer, read by Producer
};