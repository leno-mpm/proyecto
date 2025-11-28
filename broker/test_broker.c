#include <stdio.h>
#include <unistd.h>
#include "broker.h"

int main() {
    Broker broker;

    broker_init(&broker, 9000);
    broker_start(&broker);

    broker_cleanup(&broker);

    return 0;
}

