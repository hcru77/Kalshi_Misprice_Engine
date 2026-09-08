#include "../orderbook.h"
#include <iostream>
#include <chrono>
#include <cassert>

void run_tests() {
    OrderBook book;

    // Test 1: Bids and Derived Asks
    // YES bid @ 45 (qty 200), NO bid @ 57 (qty 150)
    book.set_level(Side::YES, 45, 200);
    book.set_level(Side::NO, 57, 150);

    auto best_yes_bid = book.get_best_bid(Side::YES);
    auto best_no_bid = book.get_best_bid(Side::NO);
    assert(best_yes_bid.has_value() && best_yes_bid->price_cents == 45);
    assert(best_no_bid.has_value() && best_no_bid->price_cents == 57);

    // Derived asks:
    auto best_yes_ask = book.get_best_ask(Side::YES);
    auto best_no_ask = book.get_best_ask(Side::NO);
    assert(best_yes_ask.has_value() && best_yes_ask->price_cents == 43);
    assert(best_no_ask.has_value() && best_no_ask->price_cents == 55);

    // Sum of asks = 98, arbitrage
    std::cout << "[PASS] Test 1: Bids and derived asks match expected parity." << std::endl;


    // Test 2: level clearing and recomputation
    book.set_level(Side::YES, 48, 50); // New highest YES bid
    assert(book.get_best_bid(Side::YES)->price_cents == 48);

    book.set_level(Side::YES, 48, 0);
    assert(book.get_best_bid(Side::YES)->price_cents == 45); // Should fall back to 45
    std::cout << "[PASS] Test 2: Level clearing and scan-down match expected." << std::endl;
}

void run_benchmark() {
    OrderBook book;

    constexpr size_t ITERATIONS = 10'000'000;
    std::cout << "Benchmarking " << ITERATIONS << " updates..." << std::endl;

    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        uint8_t price = 1 + (i % 99);
        book.set_level(Side::YES, price, (i & 1) ? 100 : 0);
    }
    auto end = std::chrono::steady_clock::now();

    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::cout << "Latency: " << (double)total_ns / ITERATIONS << " ns per update." << std::endl;
}

int main() {
    run_tests();
    run_benchmark();
    return 0;
}