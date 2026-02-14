#include <iostream>

#include "transport_catalogue.h"
#include "json_reader.h"

int main() {
    transport_catalogue::TransportCatalogue catalogue;
    json_reader::ProcessRequests(std::cin, std::cout, catalogue);
    return 0;
}