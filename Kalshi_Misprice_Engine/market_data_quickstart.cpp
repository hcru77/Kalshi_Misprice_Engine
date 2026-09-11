#include "simdjson.h"
#include <curl/curl.h>  // libcurl for http requests

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

constexpr const char* API_BASE = "https://external-api.kalshi.com/trade-api/v2";

size_t write_callback(
    char* data,
    size_t size,
    size_t count,
    void* user_data
) {
    auto* response = static_cast<std::string*>(user_data);

    const size_t bytes_received = size * count;
    response->append(data, bytes_received);

    return bytes_received;
}


std::string http_get(const std::string& url) {
    CURL* curl = curl_easy_init();

    if (curl == nullptr) {
        throw std::runtime_error("Could not initialize libcurl");
    }

    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    const CURLcode result = curl_easy_perform(curl);

    if (result != CURLE_OK) {
        std::string error = curl_easy_strerror(result);
        curl_easy_cleanup(curl);

        throw std::runtime_error("HTTP request failed: " + error);
    }

    curl_easy_cleanup(curl);
    return response;
}


simdjson::dom::element parse_json(
    simdjson::dom::parser& parser,
    const std::string& response
) {
    auto document = parser.parse(response);

    if (document.error()) {
        throw std::runtime_error(
            simdjson::error_message(document.error())
        );
    }

    return document.value();
}

void print_series_information (
    simdjson::dom::element response
) {
    auto series = response["series"];

    std::cout << "Series title: "
              << std::string_view(series["title"]) << '\n';

    std::cout << "Frequency: "
              << std::string_view(series["frequency"]) << '\n';

    std::cout << "Category: "
              << std::string_view(series["category"]) << "\n\n";
}

std::string print_markets(
    simdjson::dom::element response
) {
    auto markets = response["markets"].get_array();

    if (markets.error()) {
        throw std::runtime_error("Response did not contain markets");
    }

    if (markets.value().begin() == markets.value().end()) {
        throw std::runtime_error(
            "No open markets were found"
        );
    }

    std::string first_market_ticker;

    std::cout << "Active markets:\n";

    for (auto market : markets.value()) {
        const std::string ticker =
            std::string(std::string_view(market["ticker"]));

        if (first_market_ticker.empty()) {
            first_market_ticker = ticker;
        }

        std::cout << "- " << ticker << ": "
                  << std::string_view(market["title"]) << '\n';

        std::cout << "  Event: "
                  << std::string_view(market["event_ticker"]) << '\n';

        std::cout << "  YES bid: $"
                  << std::string_view(market["yes_bid_dollars"]) << '\n';

        std::cout << "  Volume: "
                  << std::string_view(market["volume_fp"]) << "\n\n";
    }

    return first_market_ticker;
}

void print_event_information(
    simdjson::dom::element response
) {
    auto event = response["event"];

    std::cout << "Event details:\n";

    std::cout << "Title: "
              << std::string_view(event["title"]) << '\n';

    std::cout << "Category: "
              << std::string_view(event["category"]) << "\n\n";
}

void print_orderbook(
    simdjson::dom::element response,
    const std::string& market_ticker
) {
    auto orderbook = response["orderbook_fp"];

    std::cout << "Orderbook for "
              << market_ticker << ":\n\n";

    std::cout << "YES bids:\n";

    size_t printed = 0;

    for (auto level : orderbook["yes_dollars"].get_array()) {
        if (printed == 5) {
            break;
        }

        std::cout << "  Price: $"
                  << std::string_view(level.at(0))
                  << ", Quantity: "
                  << std::string_view(level.at(1))
                  << '\n';

        ++printed;
    }

    std::cout << "\nNO bids:\n";

    printed = 0;

    for (auto level : orderbook["no_dollars"].get_array()) {
        if (printed == 5) {
            break;
        }

        std::cout << "  Price: $"
                  << std::string_view(level.at(0))
                  << ", Quantity: "
                  << std::string_view(level.at(1))
                  << '\n';

        ++printed;
    }
}

int main() {
    constexpr const char* series_ticker = "KXHIGHNY";

    try {
        curl_global_init(CURL_GLOBAL_DEFAULT);

        simdjson::dom::parser parser;

        const std::string series_url =
            std::string(API_BASE) +
            "/series/" +
            series_ticker;

        const std::string series_json =
            http_get(series_url);

        auto series_response =
            parse_json(parser, series_json);

        print_series_information(series_response);

        const std::string markets_url =
            std::string(API_BASE) +
            "/markets?series_ticker=" +
            series_ticker +
            "&status=open";

        const std::string markets_json =
            http_get(markets_url);

        auto markets_response =
            parse_json(parser, markets_json);

        const std::string market_ticker =
            print_markets(markets_response);

        auto markets =
            markets_response["markets"].get_array();

        auto first_market =
            *markets.begin();

        const std::string event_ticker =
            std::string(
                std::string_view(first_market["event_ticker"])
            );

        const std::string event_url =
            std::string(API_BASE) +
            "/events/" +
            event_ticker;

        const std::string event_json =
            http_get(event_url);

        auto event_response =
            parse_json(parser, event_json);

        print_event_information(event_response);

        const std::string orderbook_url =
            std::string(API_BASE) +
            "/markets/" +
            market_ticker +
            "/orderbook";

        const std::string orderbook_json =
            http_get(orderbook_url);

        auto orderbook_response =
            parse_json(parser, orderbook_json);

        print_orderbook(
            orderbook_response,
            market_ticker
        );

        curl_global_cleanup();
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error) {
        std::cerr << "Error: "
                  << error.what()
                  << '\n';

        curl_global_cleanup();
        return EXIT_FAILURE;
    }
}