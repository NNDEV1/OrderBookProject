from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from typing import Optional, List, Dict, Any
import uvicorn
from fastapi_client.orderbook_client import OrderBookClient, OrderType, Side


# Pydantic models for request/response
class OrderRequest(BaseModel):
    order_id: int
    side: int  # 0 = BUY, 1 = SELL
    price: int
    quantity: int
    order_type: int = 0  # Default to GOOD_TILL_CANCEL


class CancelOrderRequest(BaseModel):
    order_id: int


class OrderResponse(BaseModel):
    success: bool
    message: Optional[str] = None
    trades_count: Optional[int] = None
    trades: Optional[List[Dict[str, Any]]] = None


class OrderBookSnapshot(BaseModel):
    bids: List[Dict[str, int]]
    asks: List[Dict[str, int]]
    total_orders: int


# FastAPI app
app = FastAPI(title="OrderBook API", version="1.0.0")

# Global client - in production, use connection pooling
orderbook_client = OrderBookClient()


@app.on_event("startup")
async def startup_event():
    """Connect to the C++ OrderBook server on startup"""
    if not orderbook_client.connect():
        raise RuntimeError("Failed to connect to OrderBook server")
    print("Connected to OrderBook server")


@app.on_event("shutdown")
async def shutdown_event():
    """Disconnect from the server on shutdown"""
    orderbook_client.disconnect()
    print("Disconnected from OrderBook server")


@app.get("/")
async def root():
    return {"message": "OrderBook API is running"}


@app.get("/health")
async def health_check():
    """Health check endpoint"""
    try:
        size = orderbook_client.get_orderbook_size()
        return {"status": "healthy", "orderbook_size": size}
    except Exception as e:
        raise HTTPException(status_code=503, detail=f"OrderBook server unavailable: {str(e)}")


@app.post("/orders", response_model=OrderResponse)
async def add_order(order: OrderRequest):
    """Add a new order to the order book"""
    try:
        # Validate side and order_type
        if order.side not in [0, 1]:
            raise HTTPException(status_code=400, detail="side must be 0 (BUY) or 1 (SELL)")
        
        if order.order_type not in [0, 1, 2, 3, 4]:
            raise HTTPException(status_code=400, detail="Invalid order_type")
        
        result = orderbook_client.add_order(
            order_id=order.order_id,
            side=Side(order.side),
            price=order.price,
            quantity=order.quantity,
            order_type=OrderType(order.order_type)
        )
        
        if not result.get("success", True):  # Some operations don't return success field
            raise HTTPException(status_code=400, detail=result.get("error", "Order failed"))
        
        return OrderResponse(
            success=True,
            message="Order added successfully",
            trades_count=result.get("trades_count", 0),
            trades=result.get("trades", [])
        )
        
    except Exception as e:
        if isinstance(e, HTTPException):
            raise e
        raise HTTPException(status_code=500, detail=f"Internal error: {str(e)}")


@app.delete("/orders/{order_id}")
async def cancel_order(order_id: int):
    """Cancel an order"""
    try:
        result = orderbook_client.cancel_order(order_id)
        
        if not result.get("success", False):
            raise HTTPException(status_code=400, detail=result.get("error", "Cancel failed"))
        
        return {"success": True, "message": f"Order {order_id} cancelled"}
        
    except Exception as e:
        if isinstance(e, HTTPException):
            raise e
        raise HTTPException(status_code=500, detail=f"Internal error: {str(e)}")


@app.get("/orderbook", response_model=OrderBookSnapshot)
async def get_orderbook():
    """Get the current order book state"""
    try:
        orderbook = orderbook_client.get_orderbook()
        
        if not orderbook.get("success", False):
            raise HTTPException(status_code=500, detail="Failed to retrieve order book")
        
        return OrderBookSnapshot(
            bids=orderbook.get("bids", []),
            asks=orderbook.get("asks", []),
            total_orders=orderbook_client.get_orderbook_size()
        )
        
    except Exception as e:
        if isinstance(e, HTTPException):
            raise e
        raise HTTPException(status_code=500, detail=f"Internal error: {str(e)}")


@app.get("/orderbook/size")
async def get_orderbook_size():
    """Get the number of orders in the book"""
    try:
        size = orderbook_client.get_orderbook_size()
        return {"size": size}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Internal error: {str(e)}")


# Convenience endpoints
@app.post("/orders/buy")
async def add_buy_order(order_id: int, price: int, quantity: int, order_type: int = 0):
    """Convenience endpoint for buy orders"""
    order = OrderRequest(
        order_id=order_id,
        side=0,  # BUY
        price=price,
        quantity=quantity,
        order_type=order_type
    )
    return await add_order(order)


@app.post("/orders/sell")
async def add_sell_order(order_id: int, price: int, quantity: int, order_type: int = 0):
    """Convenience endpoint for sell orders"""
    order = OrderRequest(
        order_id=order_id,
        side=1,  # SELL
        price=price,
        quantity=quantity,
        order_type=order_type
    )
    return await add_order(order)


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000) 