#pragma once

#include <memory>
#include <stdexcept>

#include "OrderType.h"
#include "Usings.h"
#include "Side.h"
#include "Constants.h"

class Order {
    public:
        Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity){
            orderType_ = orderType;
            orderId_ = orderId;
            side_ = side;
            price_ = price;
            initialQuantity_ = quantity;
            remainingQuantity_ = quantity;
        }

        Order(OrderId orderId, Side side, Quantity quantity){
            Order(OrderType::Market, orderId, side, Constants::InvalidPrice, quantity);
        }

        OrderId getOrderId() const {
            return orderId_;
        }

        OrderType getOrderType() const {
            return orderType_;
        }

        Side getSide() const {
            return side_;
        }

        Price getPrice() const {
            return price_;
        }

        Quantity getInitialQuantity() const {
            return initialQuantity_;
        }

        Quantity getRemainingQuantity() const {
            return remainingQuantity_;
        }

        Quantity getFilledQuantity() const {
            return getInitialQuantity() - getRemainingQuantity();
        }

        bool isFilled() const {
            return getRemainingQuantity() == 0;
        }

        void fill(Quantity quantity) {
            if (quantity > getRemainingQuantity()) {
                throw std::runtime_error("Cannot fill more than remaining quantity");
            }

            remainingQuantity_ -= quantity;
        }

        void ToGoodTillCancel(Price price){
            if (getOrderType() != OrderType::Market){
                throw std::runtime_error("Cannot convert non-market order");
            }

            price_ = price;
            orderType_ = OrderType::GoodTillCancel;
        }

    private:
        OrderType orderType_;
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity initialQuantity_;
        Quantity remainingQuantity_;
};