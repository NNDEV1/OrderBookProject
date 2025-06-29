#pragma once
#include <vector>
#include "TradeInfo.h"

class Trade {
    public:
        Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade){
            bidTrade_ = bidTrade;
            askTrade_ = askTrade;
        }

        const TradeInfo& getBidTrade() const {
            return bidTrade_;
        }

        const TradeInfo& getAskTrade() const {
            return askTrade_;
        }

    private:
        TradeInfo bidTrade_;
        TradeInfo askTrade_;
};