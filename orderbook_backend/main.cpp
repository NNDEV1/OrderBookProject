#include <iostream>
#include <random>
#include <chrono>
#include "OrderBook.h"

using Clock = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;
using ns = std::chrono::nanoseconds;

void benchmarkOrderBook(std::size_t numOrders) {
    OrderBook orderbook;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> priceDist(90, 110);
    std::uniform_int_distribution<int> quantityDist(1, 100);
    std::bernoulli_distribution sideDist(0.5);

    auto start = Clock::now();

    for (std::size_t i = 1; i <= numOrders; ++i) {
        Side side = sideDist(rng) ? Side::Buy : Side::Sell;
        int price = priceDist(rng);
        int quantity = quantityDist(rng);
        OrderId id = i;
        
        auto order = std::make_shared<Order>(OrderType::GoodTillCancel, id, side, price, quantity);
        orderbook.AddOrder(order);
    }

    auto end = Clock::now();
    auto duration = std::chrono::duration_cast<ms>(end - start);
    double timeSec = duration.count() / 1000.0;

    std::cout << "Inserted " << numOrders << " orders in " << timeSec << " seconds." << std::endl;
    std::cout << "Throughput: " << (numOrders / timeSec) << " orders/sec." << std::endl;
    double avgLatencyNs = (duration.count() * 1e6) / numOrders;  // in nanoseconds
    std::cout << "Average Latency per order: " << avgLatencyNs << " ns" << std::endl;
}

int main() {

    benchmarkOrderBook(1'000'000);

    // OrderBook orderbook;

    // const OrderId orderId = 1;
    // const OrderId orderId2 = 2;
    // const OrderId orderId3 = 3;
    // const OrderId orderId4 = 4;
    // const OrderId orderId5 = 5;

    // std::cout << "Adding buy order at price 100, quantity 10" << std::endl;
    // orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId, Side::Buy, 100, 10));
    // std::cout << "Order book size: " << orderbook.Size() << std::endl;
    // orderbook.printOrderBook();

    // std::cout << "\nAdding buy order at price 99, quantity 5" << std::endl;
    // orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId3, Side::Buy, 99, 5));
    // orderbook.printOrderBook();

    // std::cout << "\nAdding sell order at price 101, quantity 8" << std::endl;
    // orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId4, Side::Sell, 101, 8));
    // orderbook.printOrderBook();

    // std::cout << "\nAdding sell order at price 102, quantity 12" << std::endl;
    // orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId5, Side::Sell, 102, 12));
    // orderbook.printOrderBook();

    // std::cout << "\nAdding sell order at price 102, quantity 12" << std::endl;
    // orderbook.AddOrder(std::make_shared<Order>(OrderType::Market, orderId2, Side::Buy, NULL, 12));
    // orderbook.printOrderBook();

    // std::cout << "\nAdding sell order at price 100, quantity 10 (should match with buy order)" << std::endl;
    // orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId2, Side::Sell, 100, 10));
    // std::cout << "Order book size after trade: " << orderbook.Size() << std::endl;
    // orderbook.printOrderBook();

    return 0;
} 