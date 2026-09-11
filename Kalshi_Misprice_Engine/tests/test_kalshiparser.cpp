#include "../kalshiparser.h"
#include <iostream>
#include <chrono>
#include <cassert>

void test_correctness() {
    KalshiParser parser;

    // Simulate an incoming websocket message from Kalshi, wrapped in a padded_string
    auto json_msg = simdjson::padded_string(
        std::string(R"({"type":"orderbook_delta","market_ticker":"INFLATION-26","price":45,"delta":200,"side":"yes"})")
    );

    auto result = parser.parse_msg(json_msg);

    assert(result.has_value());
    assert(result->side == Side::YES);
    assert(result->price_cents == 45);
    assert(result->delta_quantity == 200);

    std::cout << "[PASS] Kalshi JSON parsed successfully into C++ struct.\n";
}

void benchmark_parsing() {
    KalshiParser parser;
    auto json_msg = simdjson::padded_string(
        std::string(R"({"type":"orderbook_delta","market_ticker":"INFLATION-26","price":45,"delta":200,"side":"yes"})")
    );

    constexpr size_t ITERATIONS = 1'000'000;
    std::cout << "Benchmarking simdjson (" << ITERATIONS << " parses)...\n";

    auto start = std::chrono::steady_clock::now();

    for (size_t i = 0; i < ITERATIONS; ++i) {
        volatile auto temp = parser.parse_msg(json_msg);
    }

    auto end = std::chrono::steady_clock::now();
    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << "Latency: " << (static_cast<double>(total_ns) / ITERATIONS) << " ns per parse.\n";
}

int main() {
    test_correctness();
    benchmark_parsing();
    return 0;
}