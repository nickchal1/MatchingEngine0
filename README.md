C++ Order Book: MatchingEngine0

This is v0 of the Order Book I am making in C++. In later dev stages, I will implement a complete high-performance matching engine.

I am building this project to get experience with the newest C++ standards (C++ 20 and C++ 23), as well as get more comfortable with larger scale projects. I am interested in coding and quant finance and I think that this project is a great start and experience to have.

The current milestone is to have a functional Order Book


Design Specs:

The base of the OrderBook is constructed using two containers: 
    1) map that maps Prices to vectors of IDs (one for each of bid/ask)
    2) hashmap that maps IDs to Order objects

The Order is going to be a struct with ID, Price, Bid/Ask, Quantity

The OrderBook owns the Orders and assigns IDs

The Bid/Ask is made through an enum 

The price is going to be INT type to avoid float point comparisons


Expected Behavior of v0:

    1) Add orders
    2) Cancel orders
    3) Maintain bid/ask ordering
    4) Maintain arrival ordering