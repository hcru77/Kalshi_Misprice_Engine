#include "../ringbuffer.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>

// Cross-platform CPU pause for spinning
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <immintrin.h>
#endif

inline void spin_pause() {
    _mm_pause();
}

// representation incoming market tick
struct MarketTick {
	uint64_t sequence_id;
	uint8_t price_cents;
	uint64_t quantity;
};

void test_basic_ops() {

	// storing ints and capacity = 4
	RingBuffer<int, 4> rb;
	assert(rb.push(10));
	assert(rb.push(20));
	assert(rb.push(30));
	assert(rb.push(40));

	// Buffer is full (Capacity = 4)
	assert(!rb.push(50));

	int val;
	assert(rb.pop(val) && val == 10);
	assert(rb.pop(val) && val == 20);

	assert(rb.push(50));
	assert(rb.push(60));

	assert(rb.pop(val) && val == 30);
	assert(rb.pop(val) && val == 40);
	assert(rb.pop(val) && val == 50);
	assert(rb.pop(val) && val == 60);

	assert(!rb.pop(val));
	std::cout << "[PASS] Basic RingBuffer operations verified." << std::endl;
}

void benchmark_spsc_throughput() {
    constexpr size_t CAPACITY = 65536; // 2^16
    constexpr size_t TOTAL_MESSAGES = 10'000'000;

    auto rb = std::make_unique<RingBuffer<MarketTick, CAPACITY>>();

    std::cout << "Benchmarking SPSC queue across 2 threads (" << TOTAL_MESSAGES << " messages)..." << std::endl;
    auto start_time = std::chrono::steady_clock::now();

    std::thread producer([&]() {
        for (uint64_t i = 0; i < TOTAL_MESSAGES; ++i) {
            MarketTick tick{
                .sequence_id = i,
                .price_cents = static_cast<uint8_t>(1 + (i % 99)),
                .quantity = 100
            };
            // Spin until space is available
            while (!rb->push(tick)) {
                spin_pause();
            }
        }
    });

    std::thread consumer([&]() {
        MarketTick tick;
        uint64_t received = 0;
        while (received < TOTAL_MESSAGES) {
            if (rb->pop(tick)) {
                assert(tick.sequence_id == received);   // check for race conditions (if none, lock-free atomics worked
                ++received;
            }
            else {
                spin_pause();
            }
        }
    });

    producer.join();
    consumer.join();

    auto end_time = std::chrono::steady_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    double msgs_per_sec = (static_cast<double>(TOTAL_MESSAGES) / total_ms) * 1000.0;

    std::cout << "Elapsed: " << total_ms << " ms\n"
        << "Throughput: " << static_cast<uint64_t>(msgs_per_sec) << " msgs/sec\n"
        << "Average latency per hop: " << (static_cast<double>(total_ms * 1'000'000) / TOTAL_MESSAGES) << " ns"
        << std::endl;
}

int main() {
    test_basic_ops();
    benchmark_spsc_throughput();
    return 0;
}