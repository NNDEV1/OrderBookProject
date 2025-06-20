# OrderBook TCP + FastAPI Integration

A high-performance C++ OrderBook with Python FastAPI interface using TCP sockets and JSON.

## Architecture

```
HTTP Requests → FastAPI Server → TCP Client → C++ OrderBook Server
     ↓              ↓               ↓              ↓
   Port 8000     Python Code    TCP Socket    C++ OrderBook
```

## Quick Start

### 1. Compile the C++ TCP Server
```bash
g++ -std=c++17 $(pkg-config --cflags jsoncpp) -O3 \
    tcp_server.cpp OrderBook.cpp \
    $(pkg-config --libs jsoncpp) -pthread \
    -o orderbook_server
```

### 2. Install Python Dependencies
```bash
pip install fastapi uvicorn requests
```

### 3. Start the C++ OrderBook Server
```bash
./orderbook_server
# Should output: "OrderBook TCP Server listening on port 9999"
```

### 4. Start the FastAPI Server (in another terminal)
```bash
python fastapi_server.py
# Should output: "Connected to OrderBook server"
# FastAPI will run on http://localhost:8000
```

### 5. Test Everything
```bash
python test_system.py
```

## API Endpoints

### FastAPI HTTP Endpoints (Port 8000)

- **GET** `/` - Root endpoint
- **GET** `/health` - Health check
- **GET** `/orderbook` - Get full order book
- **GET** `/orderbook/size` - Get number of orders
- **POST** `/orders` - Add order (JSON body)
- **POST** `/orders/buy` - Add buy order (query params)
- **POST** `/orders/sell` - Add sell order (query params)
- **DELETE** `/orders/{order_id}` - Cancel order

### Example HTTP Requests

**Add a buy order:**
```bash
curl -X POST "http://localhost:8000/orders" \
     -H "Content-Type: application/json" \
     -d '{
       "order_id": 1,
       "side": 0,
       "price": 100,
       "quantity": 10,
       "order_type": 0
     }'
```

**Get order book:**
```bash
curl "http://localhost:8000/orderbook"
```

**Add sell order (convenience endpoint):**
```bash
curl -X POST "http://localhost:8000/orders/sell?order_id=2&price=101&quantity=8"
```

## Direct TCP Client Usage

```python
from orderbook_client import OrderBookClient, Side, OrderType

client = OrderBookClient()
client.connect()

# Add orders
result = client.add_order(1, Side.BUY, 100, 10)
result = client.add_order(2, Side.SELL, 101, 8)

# View order book
client.print_orderbook()

client.disconnect()
```

## Order Types & Sides

### Sides
- `0` = BUY
- `1` = SELL

### Order Types
- `0` = GOOD_TILL_CANCEL
- `1` = FILL_AND_KILL  
- `2` = FILL_OR_KILL
- `3` = MARKET
- `4` = GOOD_FOR_DAY

## Performance

- **Direct TCP**: ~microsecond latency
- **FastAPI Layer**: Adds ~1-2ms HTTP overhead
- **Multithreading**: C++ server handles multiple clients
- **JSON Parsing**: Minimal overhead with jsoncpp

## Files

- `tcp_server.cpp` - C++ TCP server wrapping OrderBook
- `orderbook_client.py` - Python TCP client
- `fastapi_server.py` - FastAPI HTTP server
- `test_system.py` - Integration tests
- `OrderBook.cpp/h` - Your existing OrderBook implementation

## Troubleshooting

**Port already in use:**
```bash
lsof -ti:9999 | xargs kill  # Kill C++ server
lsof -ti:8000 | xargs kill  # Kill FastAPI server
```

**Connection refused:**
- Make sure C++ server is running first
- Check no firewall blocking ports 8000/9999

**JSON errors:**
- Verify jsoncpp installed: `pkg-config --modversion jsoncpp`
- Try reinstalling: `brew reinstall jsoncpp`

## Next Steps

1. **Production**: Add a frontend!
2. **Security**: Add authentication/rate limiting
3. **Monitoring**: Add metrics and logging
4. **Scaling**: Load balance multiple C++ servers
5. **WebSocket**: Add real-time order book streaming 