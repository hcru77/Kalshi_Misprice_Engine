#include "../orderbook.h"
#include "../arbitragedetector.h"
#include <iostream>
#include <cassert>

void test_arbitrage_detector() {
    OrderBook book;

    book.set_level(Side::YES, 48, 500);
    book.set_level(Side::NO, 48, 500);

    auto signal1 = ArbitrageDetector::evaluate(book);
    assert(!signal1.has_value());
    std::cout << "[PASS] Scenario 1: Normal market correctly rejected." << std::endl;

    // Case 2: Gross arb that fails fee check
    book.reset();
    book.set_level(Side::YES, 43, 100);
    book.set_level(Side::NO, 56, 100);

    auto signal2 = ArbitrageDetector::evaluate(book);
    assert(!signal2.has_value());
    std::cout << "[PASS] Scenario 2: Unprofitable gross arb filtered out by fee model." << std::endl;

    // Case 3: Genuine profitable window
    book.reset();
    book.set_level(Side::NO, 60, 300);
    book.set_level(Side::YES, 50, 100);

    auto signal3 = ArbitrageDetector::evaluate(book);
    assert(signal3.has_value());
    assert(signal3->ask_yes_cents == 40);
    assert(signal3->ask_no_cents == 50);
    assert(signal3->max_contracts == 100); // limited by the 100 YES bids
    assert(signal3->net_profit_cents > 0);

    std::cout << "[PASS] Scenario 3: Real arbitrage detected successfully!" << std::endl;
    signal3->print();
}

int main() {
    test_arbitrage_detector();
    return 0;
}