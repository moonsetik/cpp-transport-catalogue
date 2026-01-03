#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace stat_reader {

    void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue,
        std::string_view request, std::ostream& output);

} // namespace stat_reader