#include "ringbuffer.h"
#include "orderbook.h"
#include "arbitragedetector.h"
#include "kalshiparser.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <string>

// Cross-platform CPU pause for spinning
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <immintrin.h>
#endif

inline void spin_pause() {
	_mm_pause();
}

int main() {
	std::cout << "Starting Kalshi Engine Pipeline ...\n";

	constexpr size_t CAPACITY = 65536;
	auto rb = std::make_unique<RingBuffer<MarketUpdate, CAPACITY>>();

	// Simulating the kalshi websocket feed
	std::vector<simdjson::padded_string> live_feed;
	live_feed.emplace_back(std::string(R"({"type":"orderbook_delta","market_ticker":"INFLATION-26","price":45,"delta":200,"side":"yes"})"));
	live_feed.emplace_back(std::string(R"({"type":"orderbook_delta","market_ticker":"INFLATION-26","price":50,"delta":100,"side":"no"})"));
	// This last tick creates a crossed book (ask YES = 50, ask NO = 44, 50 + 44 = 94 < 100), triggering an arbitrage opportunity
	live_feed.emplace_back(std::string(R"({"type":"orderbook_delta","market_ticker":"INFLATION-26","price":56,"delta":300,"side":"yes"})"));

	constexpr size_t TOTAL_MESSAGES = 10'000'000;

	// (Thread 2) : strategy core --> Orderbook and Alpha logic (no parsing)
	std::thread strategy_core([&]() {
		OrderBook ob;
		MarketUpdate update;
		uint64_t processed = 0;

		// Keep track of the last known profitable state to avoid spamming
		bool was_profitable = false;

		while (processed < TOTAL_MESSAGES) {
			if (rb->pop(update)) {
				ob.apply_delta(update.side, update.price_cents, update.delta_quantity);

				auto opt_arb = ArbitrageDetector::evaluate(ob);

				if (opt_arb.has_value()) {
					if (!was_profitable) {
						opt_arb->print();
						was_profitable = true;
					}
				}
				else {
					was_profitable = false; 
				}
				++processed;
			}
			else {
				spin_pause();
			}
		}
	});


	// (Thread 1) : network core --> Dedicated entirely to network I/O and parsing
	std::thread network_core([&]() {
		KalshiParser parser;

		auto start = std::chrono::steady_clock::now();

		for (size_t i = 0; i < TOTAL_MESSAGES; ++i) {
			const auto& raw_json = live_feed[i % live_feed.size()];

			auto parsed_opt = parser.parse_msg(raw_json);
			if (parsed_opt.has_value()) {
				while (!rb->push(parsed_opt.value())) {
					spin_pause();
				}
			}
		}

		auto end = std::chrono::steady_clock::now();
		auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		
		std::cout << "[DONE] Processed " << TOTAL_MESSAGES << " full pipeline updates.\n";
		std::cout << "End-to-End Elapsed Time: " << total_ms << " ms\n";
	});
	
	network_core.join();
	strategy_core.join();
	return 0;
}