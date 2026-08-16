C++ Order Book: MatchingEngine0

This is v0 of the Order Book I am making in C++. In later dev stages, I will implement a complete high-performance matching engine.

I am building this project to get experience with the newest C++ standards (C++ 20 and C++ 23), as well as get more comfortable with larger scale projects. I am interested in coding and quant finance and I think that this project is a great start and experience to have.

The current milestone is to have a functional Order Book


Design Specs:

The base of the OrderBook is constructed using two containers: 
    1) map that maps Prices to vectors of IDs (one for each of bid/ask)
    2) hashmap that maps IDs to Order objects

The Order is going to be a struct with ID, Price, Bid/Ask, Quantity

The OrderBook owns the Orders and assigns IDs:
The interface is initially as follows:

    1) addOrder
    2) cancelOrder
    3) printOrderBook


The Bid/Ask is made through an scoped enum 

The Order member vars is going to be 32 and 64 bit unsigned and signed int type to avoid float point comparisons. 
    ID: uint64 //no arithmetic so unsinged is not dangerous here
    Price: int32 //max value could be 50mil << 2bil INT_MAX
    Quantity: int64 //can get really large with cheap stocks

    Padding optimization:

        

Type alliases are used for the Order components for readability
Order is a struct since all its variables are deafult public and no significant API is needed


Decisions / Tradeoffs

    1) I am debating on whether to have default init member vars for the Order struct, though my concern is that the values will be meaningless, so I am going to explicitly be assigning EVERY parameter to make sure nothing stays unitialized / initialized to garbage



Expected Behavior of v0:

    1) Add orders
    2) Cancel orders
    3) Maintain bid/ask ordering
    4) Maintain arrival ordering