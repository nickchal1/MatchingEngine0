#ifndef SUBMISSIONRESULT_H
#define SUBMISSIONRESULT_H

#include "Order.h"
#include "Trade.h"
#include <vector>

struct SubmissionResult
{
    OrderId orderId;
    std::vector<Trade> trades;
};

#endif