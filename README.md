# Order Book Project

## Project Description

This Order Book project is a C++ implementation of a basic financial order book. It supports two types of orders: Market Orders and Limit Orders. The OrderBook efficiently matches buy and sell orders, maintaining a record of all transactions.


## Features

- Order Book: The order book is a centralized list of all buy and sell orders for a particular financial instrument. It maintains a queue of unmatched orders, sorted by price and time of entry. The order book is essential for facilitating trading as it provides transparency and liquidity to the market.

- Market Orders: A market order is an instruction to buy or sell a security immediately at the best available current price. Market orders are executed instantly, making them ideal for traders who need to enter or exit positions quickly.

- Limit Orders: A limit order is an instruction to buy or sell a security at a specific price or better. Unlike market orders, limit orders are not executed immediately but are placed in the order book until the specified price conditions are met. This allows traders to control the prices at which they transact, providing greater precision and protection against unfavorable price movements.

- Extensible Code Structure: The code is structured to facilitate easy extension to additional types of orders. This modular design ensures that new order types can be integrated with minimal changes to the existing system, promoting scalability and adaptability.

