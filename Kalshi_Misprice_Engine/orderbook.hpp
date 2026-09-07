#pragma once
#include <cstdint>
#include <array>
#include <algorithm>
#include <optional>
#include <iostream>


enum class Side : uint8_t {
	YES = 0,
	NO = 1
};

struct PriceLevel {
	uint8_t price_cents;
	uint64_t quantity;	// number of resting contracts
};

class alignas(64) OrderBook {
public:
	static constexpr uint8_t MIN_PRICE = 1;
	static constexpr uint8_t MAX_PRICE = 99;
	static constexpr size_t NUM_LEVELS = 100;

	OrderBook() {
		reset();
	}

	void reset() noexcept {
		m_yes_bids.fill(0);
		m_no_bids.fill(0);
		m_best_yes_bid = 0;
		m_best_no_bid = 0;
	}

	inline void set_level(Side side, uint8_t price, uint64_t qty) noexcept {
		if (price < MIN_PRICE || price > MAX_PRICE) [[unlikely]] return;

		auto& bids = (side == Side::YES) ? m_yes_bids : m_no_bids;
		auto& best_bid = (side == Side::YES) ? m_best_yes_bid : m_best_no_bid;

		bids[price] = qty;
		if (qty > 0) {
			if (price > best_bid) {
				best_bid = price;
			}
		}
		else if (price == best_bid) {
			recompute_best_bid(side);
		}
	}

	inline void apply_delta(Side side, uint8_t price, int64_t delta_qty) noexcept {
		if (price < MIN_PRICE || price > MAX_PRICE) [[unlikely]] return;

		auto& bids = (side == Side::YES) ? m_yes_bids : m_no_bids;
		int64_t current = static_cast<int64_t>(bids[price]);
		int64_t updated = current + delta_qty;

		set_level(side, price, updated <= 0 ? 0 : static_cast<uint64_t>(updated));
	}

	[[nodiscard]] inline std::optional<PriceLevel> get_best_bid(Side side) const noexcept {
		uint8_t best = (side == Side::YES) ? m_best_yes_bid : m_best_no_bid;
		if (best == 0) return std::nullopt;
		const auto& bids = (side == Side::YES) ? m_yes_bids : m_no_bids;
		return PriceLevel{best, bids[best]};
	}

	// Best ask for YES is going to be 100 - best bid for NO
	[[nodiscard]] inline std::optional<PriceLevel> get_best_ask(Side side) const noexcept {
		Side opposite = (side == Side::YES) ? Side::NO : Side::YES;
		uint8_t opp_best = (opposite == Side::YES) ? m_best_yes_bid : m_best_no_bid;
		if (opp_best == 0) return std::nullopt;

		const auto& opp_bids = (opposite == Side::YES) ? m_yes_bids : m_no_bids;
		uint8_t ask_price = 100 - opp_best;
		return PriceLevel{ ask_price, opp_bids[opp_best] };
	}

	[[nodiscard]] inline uint64_t get_quantity_at(Side side, uint8_t price) const noexcept {
		if (price < MIN_PRICE || price > MAX_PRICE) return 0;
		return (side == Side::YES) ? m_yes_bids[price] : m_no_bids[price];
	}

	[[nodiscard]] inline bool has_liquidity() const noexcept {
		return (m_best_yes_bid > 0) && (m_best_no_bid > 0);
	}
	
private:
	inline void recompute_best_bid(Side side) noexcept {
		auto& bids = (side == Side::YES) ? m_yes_bids : m_no_bids;
		auto& best_bid = (side == Side::YES) ? m_best_yes_bid : m_best_no_bid;

		for (int p = MAX_PRICE; p >= MIN_PRICE; --p) {
			if (bids[p] > 0) {
				best_bid = static_cast<uint8_t>(p);
				return;
			}
		}
		best_bid = 0;	// empty book
	}

	std::array<uint64_t, NUM_LEVELS> m_yes_bids{};
	std::array<uint64_t, NUM_LEVELS> m_no_bids{};
	uint8_t m_best_yes_bid{ 0 };
	uint8_t m_best_no_bid{ 0 };
};