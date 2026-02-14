#pragma once

#include <iostream>

namespace transport_catalogue {
    class TransportCatalogue;
}

namespace json_reader {

    void ProcessRequests(std::istream& input, std::ostream& output,
        transport_catalogue::TransportCatalogue& catalogue);

} // namespace json_reader