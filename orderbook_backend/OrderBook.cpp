#include "OrderBook.h"
#include <iomanip>

bool OrderBook::canMatch(Side side, Price price) const {
    if (side == Side::Buy){
        if (asks_.empty())
            return false;
        
        const auto& [bestAsk, _] = *asks_.begin();
        return price >= bestAsk;
    } else {
        if (bids_.empty()){
            return false;
        }

        const auto& [bestBid, _] = *bids_.begin();
        return price <= bestBid;
    }
}

Trades OrderBook::MatchOrders() {
    Trades trades;
    trades.reserve(orders_.size());

    while (true){
        if (bids_.empty() || asks_.empty()){
            break;
        }

        auto& [bidPrice, bids] = *bids_.begin();
        auto& [askPrice, asks] = *asks_.begin();

        if (bidPrice < askPrice){
            break;
        }

        while (!bids.empty() && !asks.empty()) {
            auto bid = bids.front();
            auto ask = asks.front();

            Quantity quantity = std::min(bid->getRemainingQuantity(), ask->getRemainingQuantity());
            bid->fill(quantity);
            ask->fill(quantity);

            if (bid->isFilled()){
                bids.pop_front();
                orders_.erase(bid->getOrderId());
            }

            if (ask->isFilled()){
                asks.pop_front();
                orders_.erase(ask->getOrderId());
            }

            trades.push_back(Trade{ 
                TradeInfo{ bid->getOrderId(), bid->getPrice(), quantity }, 
                TradeInfo{ ask->getOrderId(), ask->getPrice(), quantity }});
        }

        if (bids.empty()){
            bids_.erase(bidPrice);
        }

        if (asks.empty()){
            asks_.erase(askPrice);
        }
    }

    if (!bids_.empty()){
        auto& [_, bids] = *bids_.begin();
        auto& order = bids.front();
        if (order->getOrderType() == OrderType::FillAndKill){
            CancelOrder(order->getOrderId());
        }
    }

    if (!asks_.empty()){
        auto& [_, asks] = *asks_.begin();
        auto& order = asks.front();
        if (order->getOrderType() == OrderType::FillAndKill){
            CancelOrder(order->getOrderId());
        }
    }

    return trades;
}

Trades OrderBook::AddOrder(OrderPointer order){
    if (orders_.find(order->getOrderId()) != orders_.end()){
        return { };
    }

    if (order->getOrderType() == OrderType::FillAndKill && !canMatch(order->getSide(), order->getPrice())){
        return { };
    }

    OrderPointers::iterator iterator;

    if (order->getOrderType() == OrderType::Market){
        if (order->getSide() == Side::Buy){
            if (asks_.empty()) {
                throw std::runtime_error("Market Buy Order cannot be placed: No Ask orders available");
            }
            auto [worst_ask, _] = *asks_.begin();
            order->ToGoodTillCancel(worst_ask);
        } else {
            if (bids_.empty()) {
                throw std::runtime_error("Market Sell Order cannot be placed: No Bid orders available");
            }
            auto [worst_bid, _] = *bids_.begin();
            order->ToGoodTillCancel(worst_bid);
        }
    }     

    

    if (order->getSide() == Side::Buy){
        auto& orders = bids_[order->getPrice()];
        orders.push_back(order);
        iterator = std::next(orders.begin(), orders.size() - 1);
    }
    else {   
        auto& orders = asks_[order->getPrice()];
        orders.push_back(order);
        iterator = std::next(orders.begin(), orders.size() - 1);
    }

    orders_.insert({ order->getOrderId(), OrderEntry{ order, iterator } });

    return MatchOrders();
}

void OrderBook::CancelOrder(OrderId orderId){
    if (orders_.find(orderId) == orders_.end()){
        return;
    }

    const auto [order, iterator] = orders_.at(orderId);
    orders_.erase(orderId);

    if (order->getSide() == Side::Sell){
        auto price = order->getPrice();
        auto& orders = asks_.at(price);
        orders.erase(iterator);
        if (orders.empty()){
            asks_.erase(price);
        }
    } else {
        auto price = order->getPrice();
        auto& orders = bids_.at(price);
        orders.erase(iterator);
        if (orders.empty()){
            bids_.erase(price);
        }
    }
}

Trades OrderBook::MatchOrder(OrderModify order){
    if (orders_.find(order.getOrderId()) == orders_.end()){
        return { };
    }

    const auto& [existingOrder, _] = orders_.at(order.getOrderId());
    CancelOrder(order.getOrderId());
    return AddOrder(order.toOrderPointer(existingOrder->getOrderType()));
}

std::size_t OrderBook::Size() const { return orders_.size(); }

OrderBookLevelInfos OrderBook::getOrderInfos() const {
    LevelInfos bidInfos, askInfos;
    bidInfos.reserve(orders_.size());
    askInfos.reserve(orders_.size());

    auto CreateLevelInfos = [](Price price, const OrderPointers& orders) {
        return LevelInfo{ price, std::accumulate(orders.begin(), orders.end(), (Quantity)0, 
        [](Quantity runningSum, const OrderPointer& order)
        { return runningSum + order->getRemainingQuantity(); }) };
    };

    for (const auto& [price, orders] : bids_){
        bidInfos.push_back(CreateLevelInfos(price, orders));
    }

    for (const auto& [price, orders] : asks_){
        askInfos.push_back(CreateLevelInfos(price, orders));
    }

    return OrderBookLevelInfos{ bidInfos, askInfos };
}

void OrderBook::printOrderBook() const {
    std::cout << "\n=== ORDER BOOK ===" << std::endl;
    
    // Using stringstream instead of std::format for C++17 compatibility
    std::ostringstream header;
    header << std::left << std::setw(10) << "BID QTY" << " " 
           << std::setw(10) << "BID PRICE" << " | " 
           << std::setw(10) << "ASK PRICE" << " " 
           << std::setw(10) << "ASK QTY";
    std::cout << header.str() << std::endl;
    std::cout << std::string(45, '-') << std::endl;

    // Get all bid and ask levels
    std::vector<std::pair<Price, Quantity>> bidLevels;
    std::vector<std::pair<Price, Quantity>> askLevels;

    // Collect bid levels (already sorted by price descending)
    for (const auto& [price, orders] : bids_) {
        Quantity totalQty = 0;
        for (const auto& order : orders) {
            totalQty += order->getRemainingQuantity();
        }
        bidLevels.push_back({price, totalQty});
    }

    // Collect ask levels (already sorted by price ascending)
    for (const auto& [price, orders] : asks_) {
        Quantity totalQty = 0;
        for (const auto& order : orders) {
            totalQty += order->getRemainingQuantity();
        }
        askLevels.push_back({price, totalQty});
    }

    // Print levels side by side
    size_t maxLevels = std::max(bidLevels.size(), askLevels.size());
    
    for (size_t i = 0; i < maxLevels; ++i) {
        std::string bidQty = "";
        std::string bidPrice = "";
        std::string askPrice = "";
        std::string askQty = "";

        if (i < bidLevels.size()) {
            bidQty = std::to_string(bidLevels[i].second);
            bidPrice = std::to_string(bidLevels[i].first);
        }

        if (i < askLevels.size()) {
            askPrice = std::to_string(askLevels[i].first);
            askQty = std::to_string(askLevels[i].second);
        }

        std::ostringstream row;
        row << std::left << std::setw(10) << bidQty << " " 
            << std::setw(10) << bidPrice << " | " 
            << std::setw(10) << askPrice << " " 
            << std::setw(10) << askQty;
        std::cout << row.str() << std::endl;
    }

    std::cout << std::string(45, '-') << std::endl;
    std::cout << "Total Orders: " << orders_.size() << std::endl;
    std::cout << "=================" << std::endl;
} 