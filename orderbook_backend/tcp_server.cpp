#include <iostream>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <json/json.h>
#include "OrderBook.h"

class OrderBookServer {
private:
    OrderBook orderbook_;
    int server_fd_;
    int port_;

    std::string processRequest(const std::string& request) {
        Json::Value root;
        Json::Reader reader;
        Json::Value response;
        
        if (!reader.parse(request, root)) {
            response["error"] = "Invalid JSON";
            return jsonToString(response);
        }

        std::string action = root.get("action", "").asString();
        
        try {
            if (action == "add_order") {
                return handleAddOrder(root["data"]);
            } else if (action == "cancel_order") {
                return handleCancelOrder(root["data"]);
            } else if (action == "get_size") {
                response["size"] = static_cast<int>(orderbook_.Size());
                response["success"] = true;
            } else if (action == "get_orderbook") {
                return handleGetOrderBook();
            } else {
                response["error"] = "Unknown action: " + action;
            }
        } catch (const std::exception& e) {
            response["error"] = e.what();
            response["success"] = false;
        }

        return jsonToString(response);
    }

    std::string handleAddOrder(const Json::Value& data) {
        Json::Value response;
        
        // Extract order parameters
        OrderType orderType = static_cast<OrderType>(data.get("orderType", 0).asInt());
        OrderId orderId = data.get("orderId", 0).asUInt64();
        Side side = static_cast<Side>(data.get("side", 0).asInt());
        Price price = data.get("price", 0).asInt();
        Quantity quantity = data.get("quantity", 0).asInt();
        
        auto order = std::make_shared<Order>(orderType, orderId, side, price, quantity);
        Trades trades = orderbook_.AddOrder(order);
        
        response["success"] = true;
        response["trades_count"] = static_cast<int>(trades.size());
        
        // Add trade details
        Json::Value tradesJson(Json::arrayValue);
        for (const auto& trade : trades) {
            Json::Value tradeJson;
            tradeJson["bid_order_id"] = static_cast<Json::UInt64>(trade.getBidTrade().orderId_);
            tradeJson["ask_order_id"] = static_cast<Json::UInt64>(trade.getAskTrade().orderId_);
            tradeJson["price"] = trade.getBidTrade().price_;
            tradeJson["quantity"] = trade.getBidTrade().quantity_;
            tradesJson.append(tradeJson);
        }
        response["trades"] = tradesJson;
        
        return jsonToString(response);
    }

    std::string handleCancelOrder(const Json::Value& data) {
        Json::Value response;
        
        OrderId orderId = data.get("orderId", 0).asUInt64();
        orderbook_.CancelOrder(orderId);
        
        response["success"] = true;
        response["message"] = "Order cancelled";
        
        return jsonToString(response);
    }

    std::string handleGetOrderBook() {
        Json::Value response;
        
        auto orderBookInfo = orderbook_.getOrderInfos();
        
        // Add bids
        Json::Value bidsJson(Json::arrayValue);
        for (const auto& bid : orderBookInfo.getBids()) {
            Json::Value bidJson;
            bidJson["price"] = bid.price_;
            bidJson["quantity"] = bid.quantity_;
            bidsJson.append(bidJson);
        }
        
        // Add asks
        Json::Value asksJson(Json::arrayValue);
        for (const auto& ask : orderBookInfo.getAsks()) {
            Json::Value askJson;
            askJson["price"] = ask.price_;
            askJson["quantity"] = ask.quantity_;
            asksJson.append(askJson);
        }
        
        response["bids"] = bidsJson;
        response["asks"] = asksJson;
        response["success"] = true;
        
        return jsonToString(response);
    }

    std::string jsonToString(const Json::Value& json) {
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        return Json::writeString(builder, json);
    }

    void handleClient(int client_socket) {
        char buffer[4096];
        
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes_read <= 0) {
                break;
            }
            
            std::string request(buffer, bytes_read);
            std::string response = processRequest(request);
            
            send(client_socket, response.c_str(), response.length(), 0);
        }
        
        close(client_socket);
    }

public:
    OrderBookServer(int port = 9999) : port_(port) {}

    bool start() {
        // Create socket
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ == -1) {
            std::cerr << "Socket creation failed" << std::endl;
            return false;
        }

        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "Setsockopt failed" << std::endl;
            return false;
        }

        // Bind socket
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Bind failed" << std::endl;
            return false;
        }

        // Listen
        if (listen(server_fd_, 10) < 0) {
            std::cerr << "Listen failed" << std::endl;
            return false;
        }

        std::cout << "OrderBook TCP Server listening on port " << port_ << std::endl;

        // Accept connections
        while (true) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_socket = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                std::cerr << "Accept failed" << std::endl;
                continue;
            }

            std::cout << "Client connected" << std::endl;
            
            // Handle client in a separate thread
            std::thread client_thread(&OrderBookServer::handleClient, this, client_socket);
            client_thread.detach();
        }

        return true;
    }

    void stop() {
        if (server_fd_ != -1) {
            close(server_fd_);
        }
    }
};

int main() {
    OrderBookServer server(9999);
    
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    return 0;
} 