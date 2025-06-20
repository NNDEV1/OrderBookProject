#pragma once

#include <map>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <numeric>

#include "Usings.h"
#include "Order.h"
#include "OrderModify.h"
#include "Trade.h"
#include "OrderBookLevelInfos.h"
#include "Side.h"

using BidsMap = std::map<Price, OrderPointers, std::greater<Price>>;
using AsksMap = std::map<Price, OrderPointers, std::less<Price>>;

class OrderBook {
    private:
        struct OrderEntry {
            OrderPointer order_{ nullptr };
            OrderPointers::iterator location_;
        };

        BidsMap bids_;
        AsksMap asks_;
        std::unordered_map<OrderId, OrderEntry> orders_;

        bool canMatch(Side side, Price price) const;
        Trades MatchOrders();

    public:
        Trades AddOrder(OrderPointer order);
        void CancelOrder(OrderId orderId);
        Trades MatchOrder(OrderModify order);
        std::size_t Size() const;
        OrderBookLevelInfos getOrderInfos() const;
        void printOrderBook() const;
};
