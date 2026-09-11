#pragma once

#include "orderbook.h"
#include "simdjson.h"
#include <string_view>
#include <optional>
#include <iostream>

/*

Takes padded JSON string, parses the specific fields we need, and returns MarketUpdate struct

*/

struct MarketUpdate {
	Side side;
	uint8_t price_cents;
	int64_t delta_quantity;
};

class KalshiParser {
public:
	std::optional<MarketUpdate> parse_msg(const simdjson::padded_string& json_msg) {
		try {
			// feed string to parser
			simdjson::ondemand::document doc = m_parser.iterate(json_msg);

			std::string_view type = doc["type"];
			if (type != "orderbook_delta") {
				return std::nullopt;
			}

			MarketUpdate update;

			std::string_view side_str = doc["side"];
			update.side = (side_str == "yes") ? Side::YES : Side::NO;

			update.price_cents = static_cast<uint8_t>(uint64_t(doc["price"]));
			update.delta_quantity = static_cast<int64_t>(doc["delta"]);

			return update;
		}
		catch (const simdjson::simdjson_error& e) {
			std::cerr << "[JSON ERROR] " << e.what() << "\n";
			return std::nullopt;
		}
	}
private:
	simdjson::ondemand::parser m_parser;
};

