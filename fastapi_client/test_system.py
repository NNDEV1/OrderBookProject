#!/usr/bin/env python3
"""
Test script for the OrderBook TCP/FastAPI system
"""

import requests
import time
import subprocess
import json
from fastapi_client.orderbook_client import OrderBookClient, Side, OrderType


def test_direct_tcp_client():
    """Test the direct TCP client"""
    print("🔌 Testing Direct TCP Client")
    print("=" * 50)
    
    client = OrderBookClient()
    
    if not client.connect():
        print("❌ Failed to connect to TCP server")
        return False
    
    try:
        # Test adding orders
        print("📝 Adding buy order: ID=1, Side=BUY, Price=100, Qty=10")
        result = client.add_order(1, Side.BUY, 100, 10)
        print(f"✅ Result: {result}")
        
        print("\n📝 Adding sell order: ID=2, Side=SELL, Price=101, Qty=8")
        result = client.add_order(2, Side.SELL, 101, 8)
        print(f"✅ Result: {result}")
        
        print("\n📊 Current OrderBook:")
        client.print_orderbook()
        
        print("\n📝 Adding matching sell order: ID=3, Side=SELL, Price=100, Qty=5")
        result = client.add_order(3, Side.SELL, 100, 5)
        print(f"✅ Result: {result}")
        print(f"💰 Trades generated: {len(result.get('trades', []))}")
        
        print("\n📊 OrderBook after trade:")
        client.print_orderbook()
        
        return True
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    finally:
        client.disconnect()


def test_fastapi_endpoints():
    """Test the FastAPI HTTP endpoints"""
    print("\n🌐 Testing FastAPI HTTP Endpoints")
    print("=" * 50)
    
    base_url = "http://localhost:8000"
    
    try:
        # Test health check
        print("🏥 Testing health check...")
        response = requests.get(f"{base_url}/health")
        print(f"✅ Health: {response.json()}")
        
        # Test adding orders via HTTP
        print("\n📝 Adding buy order via HTTP...")
        order_data = {
            "order_id": 10,
            "side": 0,  # BUY
            "price": 99,
            "quantity": 15,
            "order_type": 0
        }
        response = requests.post(f"{base_url}/orders", json=order_data)
        print(f"✅ HTTP Add Order: {response.json()}")
        
        # Test convenience endpoint
        print("\n📝 Adding sell order via convenience endpoint...")
        response = requests.post(f"{base_url}/orders/sell", params={
            "order_id": 11,
            "price": 102,
            "quantity": 20
        })
        print(f"✅ HTTP Sell Order: {response.json()}")
        
        # Test getting orderbook
        print("\n📊 Getting orderbook via HTTP...")
        response = requests.get(f"{base_url}/orderbook")
        orderbook = response.json()
        print(f"✅ HTTP OrderBook: {json.dumps(orderbook, indent=2)}")
        
        # Test cancel order
        print("\n❌ Cancelling order via HTTP...")
        response = requests.delete(f"{base_url}/orders/11")
        print(f"✅ HTTP Cancel: {response.json()}")
        
        return True
        
    except requests.exceptions.ConnectionError:
        print("❌ FastAPI server not running. Start it with: python fastapi_server.py")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False


def main():
    print("🚀 OrderBook TCP + FastAPI Integration Test")
    print("=" * 60)
    
    print("\n⚠️  Make sure to start the C++ TCP server first:")
    print("   ./orderbook_server")
    print("\n⚠️  And the FastAPI server in another terminal:")
    print("   python fastapi_server.py")
    
    input("\nPress Enter when both servers are running...")
    
    # Test direct TCP client
    tcp_success = test_direct_tcp_client()
    
    # Test FastAPI endpoints
    http_success = test_fastapi_endpoints()
    
    print("\n" + "=" * 60)
    print("📋 TEST SUMMARY")
    print("=" * 60)
    print(f"Direct TCP Client: {'✅ PASS' if tcp_success else '❌ FAIL'}")
    print(f"FastAPI HTTP API:  {'✅ PASS' if http_success else '❌ FAIL'}")
    
    if tcp_success and http_success:
        print("\n🎉 All tests passed! Your OrderBook system is working!")
    else:
        print("\n😞 Some tests failed. Check the servers are running.")


if __name__ == "__main__":
    main() 