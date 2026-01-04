#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"

int main() {
    transport_catalogue::TransportCatalogue catalogue;

    input_reader::ReadAndProcessBaseRequests(std::cin, catalogue);
    stat_reader::ProcessAndPrintStatRequests(std::cin, std::cout, catalogue);

    return 0;
}