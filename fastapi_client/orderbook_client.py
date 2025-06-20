import socket
import json
from typing import Optional, List, Dict, Any
from enum import IntEnum


class OrderType(IntEnum):
    GOOD_TILL_CANCEL = 0
    FILL_AND_KILL = 1
    FILL_OR_KILL = 2
    MARKET = 3
    GOOD_FOR_DAY = 4


class Side(IntEnum):
    BUY = 0
    SELL = 1


class OrderBookClient:
    def __init__(self, host: str = "localhost", port: int = 9999):
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None

    def connect(self) -> bool:
        """Establish connection to the C++ OrderBook server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            return True
        except Exception as e:
            print(f"Failed to connect: {e}")
            return False

    def disconnect(self):
        """Close connection to the server"""
        if self.socket:
            self.socket.close()
            self.socket = None

    def _send_request(self, action: str, data: Dict[str, Any] = None) -> Dict[str, Any]:
        """Send request to server and return response"""
        if not self.socket:
            raise Exception("Not connected to server")

        request = {
            "action": action,
            "data": data or {}
        }

        # Send request
        request_json = json.dumps(request)
        self.socket.send(request_json.encode())

        # Receive response
        response_data = self.socket.recv(4096)
        response = json.loads(response_data.decode())

        return response

    def add_order(
        self,
        order_id: int,
        side: Side,
        price: int,
        quantity: int,
        order_type: OrderType = OrderType.GOOD_TILL_CANCEL
    ) -> Dict[str, Any]:
        """Add an order to the order book"""
        data = {
            "orderId": order_id,
            "side": int(side),
            "price": price,
            "quantity": quantity,
            "orderType": int(order_type)
        }
        
        return self._send_request("add_order", data)

    def cancel_order(self, order_id: int) -> Dict[str, Any]:
        """Cancel an order"""
        data = {"orderId": order_id}
        return self._send_request("cancel_order", data)

    def get_orderbook_size(self) -> int:
        """Get the number of orders in the book"""
        response = self._send_request("get_size")
        return response.get("size", 0)

    def get_orderbook(self) -> Dict[str, Any]:
        """Get the current order book state"""
        return self._send_request("get_orderbook")

    def print_orderbook(self):
        """Print a formatted view of the order book"""
        orderbook = self.get_orderbook()
        
        if not orderbook.get("success"):
            print("Failed to get order book")
            return

        print("\n=== ORDER BOOK ===")
        print(f"{'BID QTY':<10} {'BID PRICE':<10} | {'ASK PRICE':<10} {'ASK QTY':<10}")
        print("-" * 45)

        bids = orderbook.get("bids", [])
        asks = orderbook.get("asks", [])
        
        max_levels = max(len(bids), len(asks))
        
        for i in range(max_levels):
            bid_qty = str(bids[i]["quantity"]) if i < len(bids) else ""
            bid_price = str(bids[i]["price"]) if i < len(bids) else ""
            ask_price = str(asks[i]["price"]) if i < len(asks) else ""
            ask_qty = str(asks[i]["quantity"]) if i < len(asks) else ""
            
            print(f"{bid_qty:<10} {bid_price:<10} | {ask_price:<10} {ask_qty:<10}")
        
        print("-" * 45)
        print(f"Total Orders: {self.get_orderbook_size()}")
        print("==================")


# Example usage
if __name__ == "__main__":
    client = OrderBookClient()
    
    if not client.connect():
        print("Failed to connect to OrderBook server")
        exit(1)
    
    try:
        # Add some test orders
        print("Adding buy order at price 100, quantity 10")
        result = client.add_order(1, Side.BUY, 100, 10)
        print(f"Result: {result}")
        
        print("\nAdding sell order at price 101, quantity 8")
        result = client.add_order(2, Side.SELL, 101, 8)
        print(f"Result: {result}")
        
        print("\nCurrent order book:")
        client.print_orderbook()
        
        print("\nAdding sell order at price 100, quantity 5 (should match)")
        result = client.add_order(3, Side.SELL, 100, 5)
        print(f"Result: {result}")
        print(f"Trades generated: {len(result.get('trades', []))}")
        
        print("\nOrder book after trade:")
        client.print_orderbook()
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        client.disconnect() 