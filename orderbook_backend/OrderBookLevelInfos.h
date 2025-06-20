#pragma once
#include "LevelInfo.h"

class OrderBookLevelInfos {
    public:
        OrderBookLevelInfos(const LevelInfos& bids, const LevelInfos& asks){
            bids_ = bids;
            asks_ = asks;
        }

        const LevelInfos& getBids() const {
            return bids_;
        }

        const LevelInfos& getAsks() const {
            return asks_;
        }

    private:
        LevelInfos bids_;
        LevelInfos asks_;
};