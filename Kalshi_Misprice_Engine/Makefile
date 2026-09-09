CXX = g++
CXXFLAGS = -std=c++20 -O3 -march=native -Wall -Wextra -pthread

all: test_ob test_arb test_rb test_kp

test_ob: tests/test_orderbook.cpp orderbook.h
	$(CXX) $(CXXFLAGS) tests/test_orderbook.cpp -o test_ob

test_arb: tests/test_arbitragedetect.cpp orderbook.h arbitragedetector.h
	$(CXX) $(CXXFLAGS) tests/test_arbitragedetect.cpp -o test_arb

test_rb: tests/test_ringbuffer.cpp ringbuffer.h
	$(CXX) $(CXXFLAGS) tests/test_ringbuffer.cpp -o test_rb

test_kp: tests/test_kalshiparser.cpp kalshiparser.h simdjson.cpp simdjson.h
	$(CXX) $(CXXFLAGS) tests/test_kalshiparser.cpp simdjson.cpp -o test_kp

clean:
	rm -f test_ob test_arb test_rb test_kp

.PHONY: all clean