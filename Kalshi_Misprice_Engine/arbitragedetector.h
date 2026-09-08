#pragma once

#include "orderbook.h"
#include <cmath>
#include <algorithm>
#include <optional>
#include <iostream>


/*

Looks at orderbook at every single tick to determine if there is an arbitrage opportunity

*/

struct ArbitrageOpportunity {
	uint8_t ask_yes_cents;
	uint8_t ask_no_cents;
	uint64_t max_contracts;
	uint64_t gross_profit_cents;
	uint64_t total_fee_cents;
	int64_t net_profit_cents;

    void print() const {
        std::cout << "\n================ ARBITRAGE SIGNAL ================\n"
            << " Buy YES @ " << static_cast<int>(ask_yes_cents) << "c | "
            << " Buy NO  @ " << static_cast<int>(ask_no_cents) << "c\n"
            << " Total Ask Cost: " << static_cast<int>(ask_yes_cents + ask_no_cents) << "c\n"
            << " Volume: " << max_contracts << " contracts\n"
            << " Gross Profit: " << gross_profit_cents << "c ($" << gross_profit_cents / 100.0 << ")\n"
            << " Taker Fees:   " << total_fee_cents << "c ($" << total_fee_cents / 100.0 << ")\n"
            << " Net Profit:   " << net_profit_cents << "c ($" << net_profit_cents / 100.0 << ")\n"
            << "===================================================\n" << std::endl;
    }
};

class ArbitrageDetector {
public:

    //fee math
    [[nodiscard]] static inline uint64_t calculate_leg_fee(uint8_t price_cents, uint64_t contracts) noexcept {
        if (contracts == 0 || price_cents == 0 || price_cents >= 100) return 0;

        double p = price_cents / 100.0;
        double fee_dollars = contracts * 0.07 * p * (1.0 - p);

        return static_cast<uint64_t>(std::ceil(fee_dollars * 100.0));
    }

    [[nodiscard]] static inline std::optional<ArbitrageOpportunity> evaluate(const OrderBook& book) noexcept {
        auto best_yes_ask = book.get_best_ask(Side::YES);
        auto best_no_ask = book.get_best_ask(Side::NO);

        if (!best_yes_ask.has_value() || !best_no_ask.has_value()) {
            return std::nullopt;
        }

        uint8_t yes_price = best_yes_ask->price_cents;
        uint8_t no_price = best_no_ask->price_cents;

        if (yes_price + no_price >= 100) {
            return std::nullopt;
        }

        uint64_t volume = std::min(best_yes_ask->quantity, best_no_ask->quantity);
        if (volume == 0) {
            return std::nullopt;
        }

        uint64_t gross_margin_per_pair = 100 - (yes_price + no_price);
        uint64_t gross_profit = volume * gross_margin_per_pair;

        uint64_t yes_fee = calculate_leg_fee(yes_price, volume);
        uint64_t no_fee = calculate_leg_fee(no_price, volume);
        uint64_t total_fee = yes_fee + no_fee;

        int64_t net_profit = static_cast<int64_t>(gross_profit) - static_cast<int64_t>(total_fee);
        if (net_profit > 0) {
            return ArbitrageOpportunity{
                .ask_yes_cents = yes_price,
                .ask_no_cents = no_price,
                .max_contracts = volume,
                .gross_profit_cents = gross_profit,
                .total_fee_cents = total_fee,
                .net_profit_cents = net_profit
            };
        }
        return std::nullopt;
     }
};