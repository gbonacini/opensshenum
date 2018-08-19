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

#include <OpenSshEnum.hpp>
#include <Main.hpp>
#include <config.h> 

using namespace std;
using namespace opensshenum;

int main(int argc, char **argv){
   const char *SSH_PORT   = "22";
   string      usr,
               port,
               identityFile,
               host;
   #ifndef NOTRACE
   const char  flags[] = "p:i:hdV";
   #else
   const char  flags[] = "p:i:hV";
   #endif

   opterr = 0;
   int c;
   while ((c = getopt(argc, argv, flags)) != -1){
      switch (c){
         case 'p':
            port = optarg;
         break;
         case 'i':
            identityFile = optarg;
         break;
         #ifndef NOTRACE
         case 'd':
            stringutils::setDebug(true);
         break;
         #endif
         case 'V':
             #ifdef __GNUC__
                 #pragma GCC diagnostic push
                 #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
             #endif
            if(argc != 2) paramError(argv[0], "-V parameter must be present alone.");
            versionInfo();
         default:
            cerr << "Invalid parameter." << endl << endl;
            #ifdef __clang__
                [[clang::fallthrough]];
            #endif
         case 'h':
            paramError(argv[0], nullptr);
      }
   }
   #ifdef __GNUC__
       #pragma GCC diagnostic pop
   #endif

   if(port.empty())          port = SSH_PORT;
   if(identityFile.empty())  paramError(argv[0], "-i parameter must be present.");
   if(argc == optind)        paramError(argv[0], "missing honsname or address.");

   host = argv[optind];

   try{
      while( getline(cin, usr) ){
         if(!usr.empty()){
            SshConnection ssh(usr, host, port, identityFile);
            if(ssh.checkUsr())
                cout << usr << ":OK" << endl;
            else
                cout << usr << ":NOK" << endl;
         }
      }
   } catch(stringutils::StringUtilsException& e){
      cerr << "Exception Rised: " << e.what() << endl;
   } catch(inet::InetException& e){
      cerr << "Exception Rised: " << e.what() << endl;
   } catch(crypto::CryptoException& e){
      cerr << "Exception Rised: " << e.what() << endl;
   } catch(typeutils::TypesUtilsException& e){
      cerr << "Exception Rised: " << e.what() << endl;
   } catch(...){
      cerr << "Unexpected Exception Rised. "  << endl;
   }

   return 0;
}

void paramError(const char* progname, const char* err) noexcept{
   if(err != nullptr)
      cerr << err << endl << endl;
   cerr << "opensshenum - try to enumerate users on a openssh server using a dictionary. GBonacini - (C) 2018   " << endl;
   cerr << "Syntax: " << endl;
   #ifndef NOTRACE
   cerr << "       " << progname << " [-p port] [-i identity] [-d] host | [-h] | [-V]" << endl;
   #else
   cerr << "       " << progname << " [-p port] [-i identity] host | [-h] | [-V]" << endl;
   #endif
   #ifndef NOTRACE
   cerr << "       " << "-d enable debug mode." << endl;
   #endif
   cerr << "       " << "-h print this help message." << endl;
   cerr << "       " << "-V version information." << endl;
   exit(1);
}

void versionInfo(void) noexcept{
   cerr << PACKAGE << " version: " VERSION << endl;
   exit(1);
}
