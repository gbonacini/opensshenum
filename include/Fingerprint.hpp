// -----------------------------------------------------------------
// OpenSshEnum - try to enumerate users on a openssh server 
//               using a dictionary.
// Copyright (C) 2018  Gabriele Bonacini
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
// -----------------------------------------------------------------

#ifndef OPENSSHENUM_FINGERPRINT
#define OPENSSHENUM_FINGERPRINT

#include <string>
#include <map>
#include <set>
#include <vector>
#include <utility>
#include <algorithm>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>

#include <anyexcept.hpp>

namespace opensshenum{ 

    using User=std::string;
    using OsName=std::string;
    using Occurences=unsigned int;
    using OSs=std::set<OsName>; 
    using FingerprintDb=std::map<User,    OSs>;
    using Statistic=std::pair<Occurences, OsName>;
    using StatisticsResult=std::vector<Statistic>;
    using Fingerprint=std::map<std::string, unsigned int>;

    std::istream& operator>>(std::istream& is, FingerprintDb& database);
    std::ostream& operator<<(std::ostream& os, OSs&           csv);
    std::ostream& operator<<(std::ostream& os, FingerprintDb& database);

    class Fingerprinting{
        public:
           Fingerprinting(void)                                     = default;
           ~Fingerprinting(void)                                    = default;

           bool  loadDatabase(const std::string& lstFilesDirectory) noexcept;
           void  printDatabase(void)                                noexcept;
           bool  insertOccurence(const User& oc)                    noexcept;
           bool  getReport(void)                                    noexcept;

        private:
           FingerprintDb      database;
           Fingerprint        fingerprint;
           StatisticsResult   statistics;
    };

} // End Namespace

#endif