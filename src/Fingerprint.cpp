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

#include <Fingerprint.hpp>

using std::string;
using std::stringstream;
using std::sort;
using std::pair;
using std::cout;
using std::endl;
using std::istream;
using std::map;
using std::set;
using std::pair;
using std::getline;
using std::ostream;
using std::fstream;
using std::make_pair;
using std::cerr;

namespace opensshenum{ 

ostream& operator<<(ostream& os, OSs& csv){
    for(auto elem : csv)
        os << elem << ";";
    return os;
}

ostream& operator<<(ostream& os, FingerprintDb& database){
    for(auto elem : database)
        os << elem.first << ";" << elem.second << endl;
    return os;
}

 istream& operator>>(istream &is, FingerprintDb& database) {

    string         line,
                   elem,
                   key;

    bool           iskey;

    stringstream   iss;

    while(getline( is >> std::ws, line)){
        iskey = true;
        iss << line;
        while(getline( iss, elem, ';' )){
           if(!iskey){
              database[key].emplace(elem);
           }else{
              key = elem;
              database.emplace(key, set<string>());
              iskey = false;
           }
        }
        iss.clear();
    }

    return is;
}

bool Fingerprinting::insertOccurence(const User& oc) noexcept{
   bool  ret  =  true;

   try{
       if(database.find(oc) != database.end()){   
           for(auto item: database[oc]){
                 if(fingerprint.find(item) == fingerprint.end())
                     fingerprint[item] = 1; 
                 else 
                     fingerprint[item] = fingerprint[item] + 1;           
           }        
       }
   }catch(...){
       ret  =  false;
   }

   return ret;
}

 bool  Fingerprinting::getReport(void)  noexcept{
     bool ret = true;
     try{
         for(auto item : fingerprint){
            cerr << item.first << " - " << item.second << endl;
            statistics.push_back(make_pair(item.second, item.first));
         }

         sort(statistics.begin(), statistics.end(), 
              [](pair<int, string> a, pair<int, string> b ){  return a.first > b.first; });
     
         cout << endl << "=======================" << endl;
         cout << "Fingerprintings statistic:" << endl;
         cout << "=======================" << endl;
         for(auto  sitem : statistics)
           cout << sitem.first << " # " << sitem.second << endl;

     }catch(...){
         ret = false;
     }

     return ret;
 }

 bool Fingerprinting::loadDatabase(const string& lstFilesDirectory)  noexcept{
     bool  ret = true;
     try{
         fstream fs(lstFilesDirectory, fstream::in);
         database.clear();
         fs >> database;
         fs.close();
     }catch(...){
         ret = false;
     }
    
     return ret;
 }

 void  Fingerprinting::printDatabase(void) noexcept{
      cout << "  ============ DBDump ==================== " << endl;
      if(!database.empty()) cout << database;
      cout << "  ============= End   ==================== " << endl;
 }

} //End Namespace