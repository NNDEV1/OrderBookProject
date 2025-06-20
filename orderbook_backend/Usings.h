#pragma once
#include <vector>
#include <list>
#include <memory>

using Price = std::int32_t;
using Quantity = std::int32_t;
using OrderId = std::uint64_t;

// Forward declarations
class Order;
class Trade;

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;
using Trades = std::vector<Trade>;
