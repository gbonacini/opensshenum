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

namespace opensshenum{
   using  std::string;
   using  std::vector;
   using  std::initializer_list;
   using  std::tuple;
   using  std::to_string;
   using  std::fill;
   using  std::equal;
   using  std::cerr;
   using  std::cin;
   using  std::cout;
   using  std::endl;
   using  std::get;
   using  std::regex;
   using  std::regex_match;
   using  std::regex_replace;

   using  stringutils::addVarLengthDataCCharStr;
   using  stringutils::addVarLengthDataString;
   using  stringutils::appendVectBuffer;
   using  stringutils::charToUint32;
   using  stringutils::decodeB64;
   using  stringutils::encodeB64;
   using  stringutils::getPassword;
   using  stringutils::getVariableLengthRawValue;
   using  stringutils::getVariableLengthSingleBignum;
   using  stringutils::getVariableLengthValueCsv;
   using  stringutils::insArrayVals;
   using  stringutils::loadFileMem;
   using  stringutils::secureZeroing;
   using  stringutils::trace;
   using  stringutils::getDebug;
   using  stringutils::uint32ToUChars;
   using  stringutils::StringUtilsException;

   using  typeutils::safePtrdiff;
   using  typeutils::safeUint32;
   using  typeutils::safeSizeT;
   using  typeutils::safeInt;
   using  typeutils::safeUInt;
   using  typeutils::safeULong;
   using  typeutils::safeSizeT;

   using  inet::InetException;
   using  inet::InetClosedByHostException;

   using  crypto::Crypto;

   static const char INITIAL_IV_C_TO_S                = 'A';
   static const char INITIAL_IV_S_TO_C                = 'B';
   static const char ENCR_KEY_C_TO_S                  = 'C';
   static const char ENCR_KEY_S_TO_C                  = 'D';
   static const char INTEGRITY_KEY_C_TO_S             = 'E';
   static const char INTEGRITY_KEY_S_TO_C             = 'F';
   
   static const char *SSH_CONF_DIRECTORY              = ".ssh";
   static const char *SSH_KNOWN_HOST_FILE             = "known_hosts";
   static const char *SSH_SHELL_REQ                   = "shell";
   
   static const char *SSH_ID_STRING_FOOT              = "\r\n";
   static const char *SSH_ID_STRING                   = "SSH-2.0-enum\r\n";
   static const char *SSH_HEADER_ID                   = "SSH-2.0";
   static const char *RAND_FILE                       = "/dev/urandom";
   
   static const char *SSH_USERAUTH_STRING             = "ssh-userauth";
   static const char *SSH_CONNECT_STRING              = "ssh-connection";
   static const char *SSH_PUBKEY_AUTH_REQ             = "publickey";
   static const char *SSH_SESSION_SPEC                = "session";

   VarData::~VarData(void){}

   VarDataBin::VarDataBin(vector<uint8_t>& val) : data(val){}
   
   void VarDataBin::appendData(vector<uint8_t>& dest) anyexcept{
         try{
            dest.insert(dest.end(), data.begin(), data.end());
         }catch(...){
            throw(InetException("appendData : a : Data Error."));
         }
   }
   
   size_t VarDataBin::size(void)  noexcept{
         return data.size();
   }
   
   VarDataChar::VarDataChar(char val) : data(val){}
   
   void VarDataChar::appendData(vector<uint8_t>& dest) anyexcept{
         try{
            dest.push_back(static_cast<uint8_t>(data));
         }catch(...){
            throw(InetException("appendData : b : Data Error."));
         }
   }
   
   size_t VarDataChar::size(void)  noexcept{
         return sizeof(uint8_t);
   }
   
   VarDataUint32::VarDataUint32(uint32_t val) : data(val){}
   
   void VarDataUint32::appendData(vector<uint8_t>& dest)  anyexcept{
         uint32ToUChars(dest, data);
   }
   
   size_t VarDataUint32::size(void)  noexcept{
         return sizeof(uint32_t);
   }
   
   template<class T>
   VarDataString<T>::VarDataString(T& val) : data(val){}
   
   template<class T>
   void VarDataString<T>::appendData(vector<uint8_t>& dest)  anyexcept{
         addVarLengthDataString(data, dest);
   }
   
   template<class T>
   size_t VarDataString<T>::size(void)  noexcept{
         return data.size();
   }
   
   VarDataCharArr::VarDataCharArr(const char* val) : data(val){}
   
   VarDataCharArr::~VarDataCharArr(void){}
   
   void VarDataCharArr::appendData(vector<uint8_t>& dest)  anyexcept{
         addVarLengthDataCCharStr(data, dest);
   }
   
   size_t VarDataCharArr::size(void)  noexcept{
         return strlen(data);
   }
   
   VarDataRecursive::VarDataRecursive(initializer_list<VarData*>&& sList) : subList(move(sList)), globalSize(0){}
   
   VarDataRecursive::~VarDataRecursive(void){}
   
   void VarDataRecursive::appendData(vector<uint8_t>& dest)  anyexcept{
         uint32ToUChars(dest, 0);
         for(auto elem : subList) {
            addSize(elem->size());
            elem->appendData(dest);
            delete elem;
         }

         #if defined  __clang_major__ && __clang_major__ >= 4 && !defined __APPLE__ && __clang_major__ >= 4
         #pragma clang diagnostic push
         #pragma clang diagnostic ignored "-Wundefined-func-template"
         #endif

         uint32ToUChars(&(*(dest.end() - safePtrdiff(globalSize + sizeof(uint32_t)))), 
                        safeUint32(globalSize));

         #if defined  __clang_major__ && __clang_major__ >= 4 && !defined __APPLE__ && __clang_major__ >= 4
         #pragma clang diagnostic pop
         #endif
   }
   
   size_t VarDataRecursive::size(void)  noexcept{
         return globalSize;
   }
   
   void VarDataRecursive::addSize(size_t len)  noexcept{
         globalSize += len + sizeof(uint32_t);
   }

   VarDataIn::~VarDataIn(){}

   template<class T>
   VarDataBlob<T>::VarDataBlob(T& dest, string dsc) : data(dest), descr(dsc){}
   
   template<class T>
   size_t VarDataBlob<T>::insertData(vector<uint8_t>& buff, size_t offset)  anyexcept{
         TRACE(" \n  ** " + descr + ":\n", &buff, offset, 
               sizeof(uint32_t) + offset + charToUint32(buff.data() + offset) ); 
         return getVariableLengthRawValue(buff, offset, data);
   }

   VarDataBNum::VarDataBNum(BIGNUM* dest, string dsc) : data(dest), descr(dsc){}
   
   size_t VarDataBNum::insertData(vector<uint8_t>& buff, size_t offset)  anyexcept{
         TRACE(" \n  ** " + descr + ":\n", &buff, offset,
               sizeof(uint32_t) + offset + charToUint32(buff.data() + offset) ); 
         return getVariableLengthSingleBignum(buff, offset, data);
   }

   #if defined  __clang_major__ && !defined __APPLE__ && __clang_major__ >= 4
   #pragma clang diagnostic push
   #pragma clang diagnostic ignored "-Wundefined-func-template"
   #endif

   SshTransport::SshTransport(string host, string port)
             :   InetClient(host.c_str(), port.c_str()), hostname(host), 
                 clientIdString(SSH_ID_STRING), haveKeys(false)
   { 
  
      rndFd =  open(RAND_FILE, O_RDONLY);
      if(rndFd == -1)
         throw(InetException("SshTransport: Can't open random generator."));
   
      packetsRcvCount = std::numeric_limits<uint32_t>::max();   
      packetsSndCount = std::numeric_limits<uint32_t>::max();

      initBuffer(SSH_MAX_PACKET_SIZE);
      try{
         incomingEnc.resize(SSH_MAX_PACKET_SIZE * 10);  
         outcomingEnc.resize(SSH_MAX_PACKET_SIZE);
         currentHashS.resize(SSH_MAX_PACKET_SIZE);
         keys.resize(SSH_STD_KEYS_NUMBER);
         message.resize(SSH_MAX_PACKET_SIZE);
         partialRead.resize(SSH_MAX_PACKET_SIZE * 10);
      }catch(...){
         throw(InetException("SshTransport: Data error."));
      }
   
      setTimeoutMin(5, 0);

      initializer_list<uint8_t> ktypes = { INITIAL_IV_C_TO_S, INITIAL_IV_S_TO_C,    ENCR_KEY_C_TO_S, 
                                           ENCR_KEY_S_TO_C,   INTEGRITY_KEY_C_TO_S, INTEGRITY_KEY_S_TO_C };
      size_t idx = 0;
      for( auto k : ktypes){
         get<KEYTYPE>(keys[idx]) = k;
         idx++;
      }
   
      get<BN_KEYF>(dhReplyPacket)     = BN_new();
      get<BN_EXPONENT>(dhReplyPacket) = BN_new();
      get<BN_MODULUS>(dhReplyPacket)  = BN_new();
   }
   
   SshTransport::~SshTransport(){
   
      if(get<BN_KEYF>(dhReplyPacket) != nullptr) 
          BN_free(get<BN_KEYF>(dhReplyPacket));        
      if(get<BN_EXPONENT>(dhReplyPacket) != nullptr) 
          BN_free(get<BN_EXPONENT>(dhReplyPacket));        
      if(get<BN_MODULUS>(dhReplyPacket) != nullptr) 
          BN_free(get<BN_MODULUS>(dhReplyPacket));        

      if(rndFd > 0) close(rndFd);
   }

   SshTransport::SshTransport(string host, string port, string id, unsigned int maxTime)
             :   InetClient(host.c_str(), port.c_str(), true, maxTime), 
                 rndFd{-128}, hostname(host), haveKeys{false}
   { 
      clientIdString = id.empty() ? SSH_ID_STRING : id.append(SSH_ID_STRING_FOOT);
   
      packetsRcvCount = std::numeric_limits<uint32_t>::max();   
      packetsSndCount = std::numeric_limits<uint32_t>::max();

      initBuffer(SSH_MAX_PACKET_SIZE);
      try{
         incomingEnc.resize(SSH_MAX_PACKET_SIZE * 10);  
         outcomingEnc.resize(SSH_MAX_PACKET_SIZE);
         // currentHashS.resize(SSH_MAX_PACKET_SIZE);
         // keys.resize(SSH_STD_KEYS_NUMBER);
         // message.resize(SSH_MAX_PACKET_SIZE);
         partialRead.resize(SSH_MAX_PACKET_SIZE * 10);
      }catch(...){
         throw(InetException("SshTransport: Data error."));
      }
   
      setTimeoutMin(5, 0);

      get<BN_KEYF>(dhReplyPacket)     = nullptr;
      get<BN_EXPONENT>(dhReplyPacket) = nullptr;
      get<BN_MODULUS>(dhReplyPacket)  = nullptr;
   }
   
   vector<uint8_t>&  SshTransport::setKexMsg(void)  anyexcept{
      addHeader(SSH_MSG_KEXINIT, clientKexInit); 
      addRandomBytes(COOKIE_LEN, clientKexInit, clientKexInit.size()); 
  
      initializer_list<const string> al ={crypto.getKexAlgs(),     crypto.getHKeyAlgs(),   crypto.getBlkAlgsCtS(),
                                          crypto.getBlkAlgsStC(),  crypto.getMacAlgsCtS(), crypto.getMacAlgsStC(),
                                          crypto.getComprAlgCtS(), crypto.getComprAlgStC() };

      for(auto elem : al) addVarLengthDataString(elem, clientKexInit);   

      try{
         clientKexInit.insert(clientKexInit.end(), sizeof(uint32_t), 0);  // Language client -> server: not supp.
         clientKexInit.insert(clientKexInit.end(), sizeof(uint32_t), 0);  // Language server -> client: not supp.
         clientKexInit.push_back(0);                                      // First_kex_packet_follows = false
         clientKexInit.insert(clientKexInit.end(), KEX_RESERVED_BYTES_LEN, 0);  // Adding reserved-for-future 4 bytes
      }catch(...){
         throw InetException("setKexMsg: Error setting server id.");
      }

      return clientKexInit;
   }
   
   const string& SshTransport::getServerId(void) const noexcept{
      return serverIdString;
   }
   
   const string& SshTransport::getClientId(void) const noexcept{
      return clientIdString;
   }
   
   void SshTransport::getStatistics(void) const noexcept{
      cerr << "* Received " << packetsRcvCount << " ssh packets." << endl 
           << "* Sent     " << packetsSndCount << " ssh packets." << endl;
   }
  
   void SshTransport::readSsh(void) anyexcept{
      packetsRcvCount++;
      TRACE("\n* Rcv Sequence: " + to_string(packetsRcvCount));

      buffCopy.clear();     
      size_t availableBytes         = 0,
             deltaBytes             = 0;
      while(availableBytes < sizeof(uint32_t)){
         deltaBytes                 = safeSizeT(readBuffer());
         if(deltaBytes > 0){
            availableBytes         += deltaBytes;
            getBufferCopy(buffCopy, true);
         }
      }
      
      size_t requiredBytes          = charToUint32(buffCopy.data()) + sizeof(uint32_t);
      if(requiredBytes  > SSH_MAX_PACKET_SIZE - sizeof(uint32_t))
            throw InetException("readSsh: incoming packet greater than max ssh packet size.");

      TRACE("\n* Rcv Packet: length: " +  to_string(availableBytes) + 
            " - Required: " + to_string(requiredBytes), &buffCopy );

      while(requiredBytes > availableBytes){
          TRACE("\n ** Read again." );
          deltaBytes                = safeSizeT(readBuffer());
          if(deltaBytes > 0){
             availableBytes         += deltaBytes;
             getBufferCopy(buffCopy, true);
             TRACE("\n* Rcv Packet - remaining bytes: ", &buffCopy );
          }
      }
   }

   bool SshTransport::readSshEnc(int chan) anyexcept{
      int           incomingEncLen,
                    plainTextLen;
      size_t        reassembledLen  = 0,
                    availableBytes;
      bool          status          = true;

      partialRead.clear();
      reassembledLen += safeSizeT(readBuffer(currentBlockLenD));
      if(reassembledLen == 0){
         status      =  false;
         goto SRV_INTERRUPTED_BY_SIGNAL;
      }
      try{
         partialRead.insert(partialRead.end(), buffer.begin(), 
                            buffer.begin() + safePtrdiff(currentBlockLenD));
      }catch(...){
            throw InetException("readSshEnc: a : Data error.");
      }

      while( reassembledLen < currentBlockLenD){
         reassembledLen += safeSizeT(readBuffer(currentBlockLenD - reassembledLen));
         try{
            partialRead.insert(partialRead.end(), buffer.begin(), 
                               buffer.begin() + safePtrdiff(currentBlockLenD));
         }catch(...){
               throw InetException("readSshEnc: b : Data error.");
         }
      }
      if(chan != -1){
         createSendPacket(SSH_MSG_CHANNEL_WINDOW_ADJUST,
                          {new VarDataUint32(safeUint32(chan)),
                           new VarDataUint32(safeUint32(currentBlockLenD))
                          });
         TRACE("* Sent SSH_MSG_CHANNEL_WINDOW_ADJUST.\n"); 
      }

      try{
         fill(incomingEnc.begin(), incomingEnc.end(), 0);
      }catch(...){
            throw InetException("readSshEnc: Buffer init error.");
      }
      crypto.decr(partialRead.data(), safeInt(currentBlockLenD), incomingEnc.data(), &incomingEncLen);

      plainTextLen    = incomingEncLen;
      reassembledLen  = charToUint32(incomingEnc.data()) + sizeof(uint32_t) + 
                        currentHashCLen - currentBlockLenD;
      if(reassembledLen  > SSH_MAX_PACKET_SIZE - sizeof(uint32_t) - crypto.getDhHashSize())
            throw InetException("readSshEnc: incoming packet greater than max ssh packet size.");

      TRACE(string("\n* Read Expected Bytes - ") + to_string(reassembledLen + currentBlockLenD) + 
                   " Processed: " + to_string(currentBlockLenD)); 
       
      availableBytes  =  0; 
      while( availableBytes < reassembledLen){
         availableBytes += safeSizeT(readBuffer(reassembledLen - availableBytes));
         try{
            partialRead.insert(partialRead.end(),
                               buffer.begin(), buffer.begin() + readLen);
         }catch(...){
                throw InetException("readSshEnc: c : Data error.");
         }
      }

      TRACE("\n* Rcv Enc Packet - Size: :" + to_string(partialRead.size()) + 
            "\n* Read Expected Bytes - Processed remaining: " + to_string(availableBytes),
            &partialRead);

      if(chan != -1){
         createSendPacket(SSH_MSG_CHANNEL_WINDOW_ADJUST,
                          {new VarDataUint32(safeUint32(chan)),
                           new VarDataUint32(safeUint32(availableBytes))
                          });
         TRACE("* Sent SSH_MSG_CHANNEL_WINDOW_ADJUST.\n"); 
      }
       
      crypto.decr(partialRead.data() + currentBlockLenD,
                  safeInt(partialRead.size() - currentBlockLenD - currentHashCLen),
                  incomingEnc.data() + currentBlockLenD, &incomingEncLen);
           
      plainTextLen    += incomingEncLen;
      
      crypto.decrFin(incomingEnc.data() + plainTextLen, &incomingEncLen);

      plainTextLen    += incomingEncLen;
      packetsRcvCount++;
        
      uint32ToUChars(currentHashS.data(), packetsRcvCount);
      try{
         currentHashS.insert(currentHashS.begin() + sizeof(uint32_t), incomingEnc.begin(), 
                             incomingEnc.begin() + plainTextLen);
      }catch(...){
             throw InetException("readSshEnc: d : Data error.");
      }
   
      crypto.hmacStC(currentHashS.data(), safeInt(sizeof(uint32_t) + safeULong(plainTextLen)),
                     currentHashC.data(), &currentHashCLen);

      TRACE("* Calculating Hash - Rcv Unecrypted and Sequence: " + to_string(packetsRcvCount) + 
            " - Len: " + to_string(sizeof(uint32_t) + safeULong(plainTextLen)), &currentHashS, 
            0, sizeof(uint32_t), currentHashCLen + safeSizeT(plainTextLen));
      TRACE("* Calculated Hash - Len: " + to_string(currentHashCLen), &currentHashC);
 
      if(!equal(currentHashC.cbegin(), currentHashC.cend(),
          partialRead.cbegin() + safePtrdiff(partialRead.size() - currentHashCLen)))
            throw InetException("readSshEnc: Invalid Hash On Incoming Packet");

      TRACE("\n* Rcv Sequence: " + to_string(packetsRcvCount));

      SRV_INTERRUPTED_BY_SIGNAL:

      return status;
   }
 
   void SshTransport::writeSshEnc(vector<uint8_t>& msg,  uint8_t allign) anyexcept{
      size_t        encrTextLen      = 0;
      int           outcomingEncLen  = 0;   
      uint8_t       remind           = (msg.size() + 4) % allign,
                    padding          = allign - remind + 4 ;
      unsigned int  hashLen          = 0; 
   
      msg[sizeof(uint32_t)]       = padding;
      addRandomBytes(padding, msg, msg.size()); 

      uint32ToUChars(msg.data(), safeUint32(msg.size() - sizeof(uint32_t)));

      crypto.encr(msg.data(), safeInt(msg.size()), outcomingEnc.data(), &outcomingEncLen);
      encrTextLen = static_cast<size_t>(outcomingEncLen);

      crypto.encrFin(outcomingEnc.data() + safeSizeT(outcomingEncLen), &outcomingEncLen);

      encrTextLen += static_cast<size_t>(outcomingEncLen);

      packetsSndCount++;
      uint32ToUChars(currentHashS.data(), packetsSndCount);

      insArrayVals(msg, 0,  currentHashS, sizeof(uint32_t));

      TRACE("\n* Snd - Sequence: " + to_string(packetsSndCount) + 
            "\n* Encr. payload length : " + to_string(encrTextLen) + "\n* HMAC Buffer: ",
            &currentHashS, 0, sizeof(uint32_t), msg.size()+sizeof(uint32_t));

      crypto.hmacCtS(currentHashS.data(), safeInt(msg.size() + sizeof(uint32_t)),
                     outcomingEnc.data() + encrTextLen, &hashLen);

      TRACE("* HMAC Len: " + to_string(hashLen) + "\n* Encr + HMAC payload: ", 
                      &outcomingEnc, encrTextLen, encrTextLen + hashLen, 
                      encrTextLen + hashLen);

      writeBuffer(outcomingEnc.data(), encrTextLen + hashLen);
   }
   
   void SshTransport::disconnect(void) anyexcept{
      if(haveKeys){
         string descr = "Closed by client";
         addHeader(SSH_MSG_DISCONNECT, message);
   
         uint32ToUChars(message, SSH_DISCONNECT_BY_APPLICATION);
         
         addVarLengthDataString(descr, message);
         // Lang
         uint32ToUChars(message, 0);
   
         TRACE("* Sending Disconnect. ");
   
         writeSshEnc(message, AES_BLOCK_LEN_ALLIGN);
      }
   }

   void SshTransport::writeSsh(const uint8_t* msg, size_t size) const anyexcept{
      packetsSndCount++;
      writeBuffer(msg, size);

      TRACE("* Snd Sequence: " + to_string(packetsSndCount));
   }
   
   void SshTransport::writeSsh(const string& msg) const anyexcept{
      packetsSndCount++;
      writeBuffer(msg);

      TRACE("* Snd Sequence: " + to_string(packetsSndCount));
   }
   
   void SshTransport::checkSshHeader(void)   anyexcept{
      static_cast<void>(readLineTimeout(SSH_MAX_ID_STRING_SIZE));

      serverIdString = currentLine;
      TRACE("* Server Id String: ", reinterpret_cast<const uint8_t*>(serverIdString.c_str()), 
            serverIdString.size());

      if(serverIdString[serverIdString.size() - 2] != 0x0D) 
         throw InetException(string("checkSshHeader: Invalid Server Id String Footer: ") + serverIdString);

      if(!checkHeader(SSH_HEADER_ID))
         throw InetException("checkSshHeader: Unexpected Header");
   }
   
   void SshTransport::addRandomBytes(size_t bytes, vector<uint8_t>& target, 
                                     size_t offset) const anyexcept{
      try{
         target.insert(target.end(), bytes, 0);
      }catch(...){
         throw InetException("addRandomBytes: Data error");
      }
      if(read(rndFd, target.data() + offset, bytes) < 0)
         throw(InetException("addRandomBytes: Can't read random bytes"));
   }

   void SshTransport::checkServerAlgList(void) anyexcept{
      sshKexPacket   packet;
      
      try{
         serverKexInit.insert(serverKexInit.end(), buffCopy.begin(),
                              buffCopy.end());  
      }catch(...){
         throw InetException("checkServerAlgList: a : Data error");
      }
  
      packet.packet_length   = charToUint32(buffCopy.data());
      try{
         packet.padding_length  = buffCopy.at(PADDING_LEN_OFFSET);
         packet.kex_packet_type = buffCopy.at(PACKET_TYPE_OFFSET);
      }catch(...){
         throw InetException("checkServerAlgList: a :  Invalid index.");
      }

      if(packet.kex_packet_type != SSH_MSG_KEXINIT){
          trace("Unexpected Packet Type: ", &buffCopy, 0, 0, 
                charToUint32(buffCopy.data()) + sizeof(uint32_t));
          throw InetException(string("checkServerAlgList: Invalid Packet type, expected: ") +
                              to_string(SSH_MSG_KEXINIT) + " - Received: " +
                              to_string(packet.kex_packet_type));
      }

      try{
         packet.server_cookie.insert(packet.server_cookie.end(), 
                                     buffCopy.data() + 6, buffCopy.data() + 16);
      }catch(...){
         throw InetException("checkServerAlgList: b :  Data error.");
      }
   
      TRACE("* Server Alg. List: \n ** Received bytes: " + to_string(buffCopy.size()) +
            "\n ** Packet Length: " + to_string(packet.padding_length) +
            "\n ** Padding: " + to_string(packet.padding_length) + "\n ** Kex Packet Type: " + 
            to_string(packet.kex_packet_type) + "\n ** Server Cookie: ", &(packet.server_cookie));
   
      size_t offset = 22;
      vector<char>buff(buffCopy.size() + 1);
      for(int i=0;i<10;i++){
         offset += getVariableLengthValueCsv(buffCopy, buff,
                   packet.algorithmStrings, i, offset);
      }

      try{ 
         packet.kex_first_pkt_follow  = buffCopy.at(offset + 1);
      }catch(...){
         throw InetException("checkServerAlgList: b :  Invalid index.");
      }

      if(packet.kex_first_pkt_follow != 0){
         TRACE("* Server KEXFOLLOW: " + to_string(buffCopy[offset + 1]));
         throw InetException("checkServerAlgList: Unexpected additional data in kex packet.");
      }
  
      try{ 
         packet.reserved.insert(packet.reserved.end(), buffCopy.data() + offset + 2, 
                                buffCopy.data() + offset + 9);
      }catch(...){
         throw InetException("checkServerAlgList: c :  Data error.");
      }
   
      TRACE("* Reserver Bytes: ", &(packet.reserved));

      crypto.initServerAlgs(packet.algorithmStrings);

      currentBlockLenE             = safeUInt(crypto.getBlockLenE());
      currentBlockLenD             = safeUInt(crypto.getBlockLenD());
      currentHashCLen              = safeUInt(crypto.getDhHashSize());

      try{ 
         currentHashC.resize(currentHashCLen);
      }catch(...){
         throw InetException("checkServerAlgList: d :  Data error.");
      }
   }
   
   void SshTransport::checkServerDhReply(void) anyexcept{
   
      get<PACKET_LENGTH>(dhReplyPacket)      = charToUint32(buffCopy.data());

      try{
         get<PADDING_LENGTH>(dhReplyPacket)  = buffCopy.at(PADDING_LEN_OFFSET);
         get<KEX_PACKT_TYPE>(dhReplyPacket)  = buffCopy.at(PACKET_TYPE_OFFSET);
      }catch(...){
         throw InetException("checkServerDhReply: a :  Invalid index.");
      }

      try{ 
         get<SERVER_COOKIE>(dhReplyPacket).insert(get<SERVER_COOKIE>(dhReplyPacket).end(), 
                                                  buffCopy.begin() + DATA_OFFSET,
                                                  buffCopy.begin() + DATA_OFFSET + COOKIE_LEN);
      }catch(...){
         throw InetException("checkServerDhReply: a :  Data error.");
      }
   
      TRACE("* Rcv SSH_MSG_NEWKEYS packet - DH Reply: \n ** Received bytes: " + to_string(buffCopy.size()) +
            "\n ** Payload Length: "  + to_string(get<PACKET_LENGTH>(dhReplyPacket) - sizeof(uint32_t)) + 
            "\n ** Padding Length: "  + to_string(get<PADDING_LENGTH>(dhReplyPacket)) +
            "\n ** Kex Packet Type: " + to_string(get<KEX_PACKT_TYPE>(dhReplyPacket)),
            &buffCopy);
   
      genericBuffer.clear();

      try{ 
         genericBuffer.insert(genericBuffer.end(), buffCopy.begin() + safePtrdiff(DATA_OFFSET + sizeof(uint32_t)), 
                              buffCopy.begin() + safePtrdiff(DATA_OFFSET + sizeof(uint32_t) +
                              charToUint32(buffCopy.data() + DATA_OFFSET))); 
      }catch(...){
         throw InetException("checkServerDhReply: b :  Data error.");
      }

      encodeB64(genericBuffer, get<CERTIFICATE_B64>(dhReplyPacket));
   
      TRACE("\n ** Host Blob (Certificate) in B64: \n" + get<CERTIFICATE_B64>(dhReplyPacket) + "\n");
   
      initializer_list<VarDataIn*> outerList    = 
                       {new VarDataBlob<vector<uint8_t>>(get<PUBKEY_BLOB>(dhReplyPacket),    "PK Blob"),
                        new VarDataBNum(get<BN_KEYF>(dhReplyPacket),                         "Key F")}; 
      initializer_list<VarDataIn*> innerList[2] = 
                      {{new VarDataBlob<string>(get<CERTIFICATE_ID>(dhReplyPacket),          "Cert. Id"),
                        new VarDataBNum(get<BN_EXPONENT>(dhReplyPacket),                     "Exponent"),
                        new VarDataBNum(get<BN_MODULUS>(dhReplyPacket),                      "Modulus")}, 
                       {new VarDataBlob<vector<uint8_t>>(get<SIGNATURE_ID>(dhReplyPacket),   "Sign. Id"),
                        new VarDataBlob<vector<uint8_t>>(get<HASH_SIGNATURE>(dhReplyPacket), "Sign. Hash")}};

      size_t offset      = DATA_OFFSET, 
             innerOffset = DATA_OFFSET + sizeof(uint32_t);
      int    mainField   = 0;
  
      for(auto outerElem : outerList){
         offset += outerElem->insertData(buffCopy, offset);
         if(mainField == 1) innerOffset = offset + sizeof(uint32_t);
         for(auto innerElem : innerList[mainField]){
             innerOffset += innerElem->insertData(buffCopy, innerOffset);
             delete innerElem;
         }
         mainField++; 
         delete outerElem;
      }
   
      if(BN_num_bits(get<BN_MODULUS>(dhReplyPacket)) < SSH_RSA_MIN_MODULUS_LENGTH)
         throw InetException("checkServerDhReply: Invalid Modulus size: " + 
                             to_string(BN_num_bits(get<BN_MODULUS>(dhReplyPacket))));
   
      crypto.setDhSharedKey(get<BN_KEYF>(dhReplyPacket));

      vector<uint8_t>  hashBuffer; 
      appendVectBuffer(hashBuffer, clientIdString.c_str(), clientIdString.size()-2, 0, 
                       clientIdString.size() - 3);
      appendVectBuffer(hashBuffer, serverIdString.c_str(), serverIdString.size()-2, 0, 
                       serverIdString.size() - 3);
      appendVectBuffer(hashBuffer, clientKexInit, 5, clientKexInit[4]);
      appendVectBuffer(hashBuffer, serverKexInit, 5, serverKexInit[4]);
      appendVectBuffer(hashBuffer, get<PUBKEY_BLOB>(dhReplyPacket));
   
      // E, F, Shared
      initializer_list<BIGNUM*> list = { crypto.getE(), get<BN_KEYF>(dhReplyPacket), 
                                            crypto.getSharedKey() };

      for(auto elem : list) {
         try{
            genericBuffer.resize(static_cast<size_t>(BN_num_bytes(elem)));
         }catch(...){
            throw InetException("checkServerDhReply: c :  Data error.");
         }
         if( BN_bn2bin(elem, genericBuffer.data()) == 0 ) 
             throw InetException("checkServerDhReply: Wrong Key Element size.");
         if(elem == crypto.getSharedKey())  appendVectBuffer(sharedKey, genericBuffer);
         else                               appendVectBuffer(hashBuffer, genericBuffer);
      }
   
      TRACE("* SK dump : ", &sharedKey);
   
      // Shared
      appendVectBuffer(hashBuffer, genericBuffer);
   
      TRACE("* Hash buffer - added : clientId, serverId, clientKex, \n  serverKex, blob, E, F, SK",
            &hashBuffer);
  
      try{    
         sessionIdHash.resize(SHA_DIGEST_LENGTH);
      }catch(...){
         throw InetException("checkServerDhReply: d :  Data error.");
      }

      crypto.dhHash(hashBuffer, sessionIdHash);
   
      TRACE("* Hash buffer: added SK (Session Id).\n* Session Id dump : ", &sessionIdHash);
      try{ 
         currentSessionHash.insert(currentSessionHash.end(), sessionIdHash.begin(), sessionIdHash.end());
      }catch(...){
         throw InetException("checkServerDhReply: e :  Data error.");
      }
   
      if(sessionIdHash.size() != currentHashCLen)
         throw InetException("checkServerDhReply: Invalid Hash Size.");
  
      crypto.signDH(sessionIdHash,  get<HASH_SIGNATURE>(dhReplyPacket), 
                    get<BN_MODULUS>(dhReplyPacket), get<BN_EXPONENT>(dhReplyPacket));

      checkServerSignature();
   
      if(get<PACKET_LENGTH>(dhReplyPacket) < buffCopy.size()){
         uint32_t next = charToUint32(buffCopy.data() + get<PACKET_LENGTH>(dhReplyPacket) +
                                      sizeof(uint32_t));
   
         TRACE("* Next Len: " + to_string(next));
       
         try{ 
            if(buffCopy.at(get<PACKET_LENGTH>(dhReplyPacket) + 2*sizeof(uint32_t) + sizeof(uint8_t)) 
               != SSH_MSG_NEWKEYS ) {
                  trace("Unexpected Packet Type: ", &buffCopy, 0, 0, 
                        charToUint32(buffCopy.data()) + sizeof(uint32_t));
                  throw InetException(string("checkServerDhReply: Invalid Packet type, expected: ") +
                                      to_string(SSH_MSG_NEWKEYS) + " - Received: " +
                                      to_string(buffCopy[get<PACKET_LENGTH>(dhReplyPacket) + 
                                      sizeof(uint32_t) + sizeof(uint8_t)]));
            }
         }catch(...){
            throw InetException("checkServerDhReply: b :  Invalid index.");
         }
      } else {
         throw InetException("checkServerDhReply: SSH_MSG_NEWKEYS packet not received.");
      }

      packetsRcvCount++;
      haveKeys = true;
   }
   
   void SshTransport::checkServerSignature(void) anyexcept{
      size_t   idx        = 0;
      Id       line;
      bool     notPresent = true;
      int      fd         = open(knownHosts.c_str(), O_RDWR | O_CREAT, S_IRWXU);
      if(fd == -1)
         throw InetException(string("checkServerSignature: Error opening in the file: ") + knownHosts);
   
      while(notPresent){
         ssize_t size = read(fd, message.data(), SSH_MAX_PACKET_SIZE);
         if(size <= 0) break;
         if(size <  0) throw InetException("checkServerSignature: Error reading knownhost file.");
         try{
            for(size_t i=0; i<static_cast<size_t>(size) && notPresent; i++){
               switch(message[i]){
                  case ' ':
                     idx++;
                  break;
                  case '\n':
                     if(get<CERTIFICATE_B64>(dhReplyPacket).compare(get<2>(line)) == 0){
                        if(hostname.compare(get<0>(line)) == 0 && 
                           get<CERTIFICATE_ID>(dhReplyPacket).compare(get<1>(line)) == 0)
                               notPresent =  false;
                     }
                     get<0>(line).clear();
                     get<1>(line).clear();
                     get<2>(line).clear();
                     idx = 0;
                  break;
                  default:
                     switch(idx){
                        case 0:
                           get<0>(line).push_back(static_cast<char>(message[i]));
                        break;
                        case 1:
                           get<1>(line).push_back(static_cast<char>(message[i]));
                        break;
                        case 2:
                           get<2>(line).push_back(static_cast<char>(message[i]));
                     }
               }
            }
         }catch(...){
            throw InetException("checkServerSignature: Data error.");
         }
      }
   
      string confirm;
      if(notPresent){
         vector<uint8_t> srvIdSign;
         crypto.serverKeyHash(get<CERTIFICATE_B64>(dhReplyPacket), srvIdSign);
   
         if(lseek(fd, 0, SEEK_END) == -1)
            throw InetException(string("checkServerSignature: Error positioning in the file: ") +
                                          knownHosts);
         string newEntry;
         try{
            newEntry.append(hostname).append(" ").append(get<CERTIFICATE_ID>(dhReplyPacket));
            newEntry.append(" ").append(get<CERTIFICATE_B64>(dhReplyPacket)).append("\n");;
         }catch(...){
            throw InetException("checkServerSignature: String error.");
         }
            
         if(write(fd, newEntry.c_str(), newEntry.size()) < 0)
            throw InetException("checkServerSignature: Error writing knownhost file.");
   
         TRACE("* Added entry to the file:"  + knownHosts + "\n" + newEntry);

         cerr << "Warning: Permanently added '" + hostname + 
                 "' (" + get<1>(line) + ") to the list of known hosts." 
              << endl;
      }
      close(fd);
   }
   
   void SshTransport::createKeys(size_t keyLen) anyexcept{
      // RFC 4253
   
      for(auto i = keys.begin(); i != keys.end(); ++i){
         get<KEYTEXT>(*i).clear();
         try{ 
            get<KEYTEXT>(*i).insert(get<KEYTEXT>(*i).end(), sharedKey.begin(), sharedKey.end());
            get<KEYTEXT>(*i).insert(get<KEYTEXT>(*i).end(), currentSessionHash.begin(), currentSessionHash.end());
            get<KEYTEXT>(*i).push_back(get<KEYTYPE>(*i));
            get<KEYTEXT>(*i).insert(get<KEYTEXT>(*i).end(), sessionIdHash.begin(), sessionIdHash.end());
         }catch(...){
            throw InetException("createKeys: a : Data error.");
         }
   
         size_t currentKeyLen  = currentHashCLen;
         size_t currentHashLen = EVP_MAX_MD_SIZE;
         try{
            get<KEYHASH>(*i).resize(currentKeyLen);
         }catch(...){
            throw InetException("createKeys: b : Data error.");
         }
         crypto.dhHash(get<KEYTEXT>(*i), get<KEYHASH>(*i));
   
         while(get<KEYHASH>(*i).size() < keyLen ){
            currentKeyLen  += currentHashCLen;
            currentHashLen += currentHashCLen;
            try{
               get<KEYTEXT>(*i).resize(currentHashLen);
               get<KEYTEXT>(*i).insert((get<KEYTEXT>(*i).end() - currentHashCLen), 
                                       (get<KEYHASH>(*i).end() - currentHashCLen),
                                       get<KEYHASH>(*i).end());
               get<KEYHASH>(*i).resize(currentKeyLen);
            }catch(...){
               throw InetException("createKeys: c : Data error.");
            }

            crypto.dhHash(get<KEYTEXT>(*i), get<KEYHASH>(*i).data() + (currentKeyLen - currentHashCLen));
         }
         get<KEYHASH>(*i).erase(get<KEYHASH>(*i).end() - 
                                safePtrdiff(get<KEYHASH>(*i).size() - keyLen), 
                                get<KEYHASH>(*i).end());
   
         TRACE("* Hash - " +  string(1, static_cast<char>(get<KEYTYPE>(*i))) + 
               ":", &get<KEYTEXT>(*i));
         TRACE("* Key  - " + string(1, static_cast<char>(get<KEYTYPE>(*i))) + 
               ":", &get<KEYHASH>(*i));
      }
   
      crypto.initBlkEnc(get<KEYHASH>(keys[ENCR_KEY_C_TO_S_IDX]), get<KEYHASH>(keys[INITIAL_IV_C_TO_S_IDX]));
      crypto.initBlkDec(get<KEYHASH>(keys[ENCR_KEY_S_TO_C_IDX]), get<KEYHASH>(keys[INITIAL_IV_S_TO_C_IDX]));

      crypto.initMacCtS(&(get<KEYHASH>(keys[INTEGRITY_KEY_C_TO_S_IDX])));
      crypto.initMacStC(&(get<KEYHASH>(keys[INTEGRITY_KEY_S_TO_C_IDX])));
   }
   
   void SshTransport::addHeader(uint8_t packetType, vector<uint8_t>& buff) const anyexcept{
         buff.clear();
         try{
            buff.insert(buff.end(), sizeof(uint32_t), 0);  // 4 bytes reserved for packet length
            buff.push_back(0);                             // 1 byte reserved for padding length
            buff.push_back(packetType);                    // 1 byte message type
         }catch(...){
            throw InetException("addHeader : Data error.");
         }
   }

   void  SshTransport::sendWithHeader(vector<uint8_t>& buff, uint8_t allign) const anyexcept{
         const uint8_t* vectHandler   = buff.data();
         uint8_t        remind        = (buff.size() + 4) % allign,
                        padding       = allign - remind + 4 ;
   
         buff[PADDING_LEN_OFFSET] = padding;
         try{
            buff.insert(buff.end(), padding, 0);  // Added padding
         }catch(...){
            throw InetException("sendWithHeader : Data error.");
         }
   
         size_t         bufferLength  = buff.size() ;
         uint32ToUChars(buff.data(), safeUint32(buff.size() - sizeof(uint32_t)));
               
         writeSsh(vectHandler, bufferLength);
   
         TRACE("* Send With Header - Msg type: " + to_string(buff[PACKET_TYPE_OFFSET]) +
               "\n ** Calculated padding: " + to_string(padding) + 
               "\n ** Length: " + to_string(bufferLength), &buff);
   }
  
   void SshTransport::createSendPacket(const uint8_t packetType, 
                                       initializer_list<VarData*>&& list) anyexcept{
      addHeader(packetType, message); 
      for(auto elem : list) {
         elem->appendData(message);
         delete elem;
      }
      writeSshEnc(message, AES_BLOCK_LEN_ALLIGN);
      TRACE("* Buffer Capacity: " + to_string(message.capacity()));
   }

   void Fsm::setInitStat(unsigned int status) noexcept{
      currStat = status;
   }

   void Fsm::setTree(StatusTree* tree) noexcept{ 
      statuses = tree;
   }

   void Fsm::checkStatus(unsigned int newStat) anyexcept{
      auto i  = statuses->find(newStat);
      if( i  == statuses->end())
         throw InetException("checkStatus: Fsm Error: invalid status " + to_string(newStat) + ".");

      auto ii = i->second.find(currStat);
      if( ii == i->second.cend())
         throw InetException("checkStatus: Packet sequence error: unexpected packet sequence: " +
                             to_string(currStat) + " --> " + to_string(newStat));

      currStat = newStat;
   }

   SshConnection::SshConnection(string& usr, string& host, string& port,
                                 std::string& identity, uint32_t chan)  
        :   SshTransport(host, port), user{usr}, idFilePref{identity}, 
            channelNumber{chan}, remoteChannelNumber{0}, initialWindowsSize{0}, 
            maxPacketSize{0} 
   {
      try{
         extData.reserve(SSH_MAX_PACKET_SIZE);
         keybInputData.reserve(SSH_MAX_PACKET_SIZE);
      }catch(...){
         throw InetException("SshConnection: Data error.");
      }
      static_cast<void>(sigfillset(&sigsetBlockAll));
      if(sigprocmask(0, nullptr, &sigsetBackup) != 0)
         throw InetException("SshConnection: Error getting the signal mask.");
   }

   SshConnection::SshConnection(string& usr,          string& host,   string& port,
                                string& id,           string& rexp,   string& clientConnStr,
                                unsigned int maxTime, uint32_t chan)  
        :   SshTransport(host, port, id, maxTime), user{usr}, 
            channelNumber{chan}, remoteChannelNumber{0}, initialWindowsSize{0}, 
            maxPacketSize{0}, sshSrvRegexp{rexp}, regexptext{rexp}
   {
      if(clientConnStr.empty())
            clientConnectionId = SSH_ID_STRING;
      else 
            clientConnectionId = clientConnStr;

      try{
         extData.reserve(SSH_MAX_PACKET_SIZE);
         keybInputData.reserve(SSH_MAX_PACKET_SIZE);
      }catch(...){
         throw InetException("SshConnection: Data error.");
      }
   }

   SshConnection::~SshConnection(){
      ERR_clear_error();  
   }

   void  SshConnection::createAuthSign(vector<uint8_t>& msg, initializer_list<VarData*>&& list) anyexcept{
      genericBuffer.clear();
      for(auto elem : list) {
         elem->appendData(genericBuffer);
         delete elem;
      }
      crypto.signMessage(privKey, genericBuffer, msg);
   }
   
   void  SshConnection::createSendShellData() anyexcept{
      addHeader(SSH_MSG_CHANNEL_DATA, message); 
      uint32ToUChars(message, channelNumber);
      addVarLengthDataString(keybInputData, message);
      writeSshEnc(message, AES_BLOCK_LEN_ALLIGN);
   }

   void SshConnection::getUserKeyFiles(void) anyexcept{
      try{
         pubKey.append(getenv("HOME")).append("/").append(SSH_CONF_DIRECTORY);
         privKey = pubKey;
         knownHosts.append(pubKey).append("/").append(SSH_KNOWN_HOST_FILE);
      }catch(...){
         throw InetException("getUserKeyFiles: a : String error.");
      }
   
      if(mkdir(pubKey.c_str(), 0700) != 0 && errno != EEXIST)
         throw InetException(string("getUserKeyFiles: Error Checking Conf Dir: ") + strerror(errno));
      errno = 0;
   
      try{
         pubKey.append("/").append(idFilePref.empty() ? crypto.getKeyFilePrefix() : idFilePref).append(".pub");
         privKey.append("/").append(idFilePref.empty() ? crypto.getKeyFilePrefix() : idFilePref);
      }catch(...){
         throw InetException("getUserKeyFiles: b : String error.");
      }
   }

   void SshConnection::getUserPubK(void) anyexcept{
      try{
         loadFileMem(pubKey, genericBuffer, true);
      }catch(StringUtilsException& e){
         TRACE("* Public Key - " + e.what() + "\n  Using Null Key.");
         genericBuffer.clear();
         try{
            string nullKey = crypto.getDhId() + " " + crypto.getNullKey() + " " + user;
            genericBuffer.insert(genericBuffer.end(), nullKey.begin(), nullKey.end());
            genericBuffer.push_back(0);
         }catch(...){
            throw InetException("getUserPubK: Data error.");
         }
      }
      
      char*       flag              = strtok(reinterpret_cast<char*>(genericBuffer.data()), " ");
      get<PUBKEYTYPE>(clientPubKey) = flag;
      flag = strtok(nullptr, " ");
      decodeB64(string(flag), get<PUBKEYBLOB>(clientPubKey));
      flag = strtok(nullptr, " ");
      get<PUBKEYUSR>(clientPubKey)  = flag;
   
      TRACE("* Client PubK Type: " + get<PUBKEYTYPE>(clientPubKey) + "\n ** Client PubK Usr: " + 
            get<PUBKEYUSR>(clientPubKey) + "\n ** Client PubK Blob: ", &get<PUBKEYBLOB>(clientPubKey));
   }

   bool SshConnection::connectionLoop(void) anyexcept{
      bool              again             = true,
                        isPresent         = false;
      uint32_t          errCode,
                        confirmedChannel;
      string            tmp;
      size_t            offset            = 0;
      StatusTree        tree              = 
                        { 
                          {SSH_MSG_SERVICE_ACCEPT,            {SSH_CONN_START}},  
                          {SSH_MSG_USERAUTH_FAILURE,          {SSH_MSG_SERVICE_ACCEPT,
                                                               SSH_MSG_USERAUTH_FAILURE,
                                                               SSH_MSG_USERAUTH_INFO_REQUEST}},  
                          {SSH_MSG_USERAUTH_INFO_REQUEST,     {SSH_MSG_SERVICE_ACCEPT,
                                                               SSH_MSG_USERAUTH_INFO_REQUEST,
                                                               SSH_MSG_USERAUTH_FAILURE}},
                          {SSH_MSG_USERAUTH_SUCCESS,          {SSH_MSG_USERAUTH_INFO_REQUEST,
                                                               SSH_MSG_USERAUTH_FAILURE}},
                          {SSH_MSG_CHANNEL_OPEN_CONFIRMATION, {SSH_MSG_USERAUTH_SUCCESS}},
                          {SSH_MSG_CHANNEL_WINDOW_ADJUST,     {SSH_MSG_CHANNEL_OPEN_CONFIRMATION}}
                        };

      fsm.setInitStat(SSH_CONN_START);
      fsm.setTree(&tree);
      createSendPacket(SSH_MSG_SERVICE_REQUEST, {new VarDataCharArr(SSH_USERAUTH_STRING)});

      try{
          while(again){
             static_cast<void>(readSshEnc());

             switch(incomingEnc[PACKET_TYPE_OFFSET]){
                case SSH_MSG_IGNORE:
                   TRACE( "* Received SSH_MSG_IGNORE: nothing to do.");
                break;
                case SSH_MSG_USERAUTH_INFO_REQUEST:
                   TRACE("* Received SSH_MSG_USERAUTH_INFO_REQUEST.");
                   fsm.checkStatus(SSH_MSG_USERAUTH_INFO_REQUEST);
                   {
                      vector<uint8_t> sign; 
                      createAuthSign(sign,
                         {new VarDataString<vector<uint8_t>>
                              (sessionIdHash),
                          new VarDataChar(SSH_MSG_USERAUTH_REQUEST),
                          new VarDataString<string>(user),
                          new VarDataCharArr(SSH_CONNECT_STRING),
                          new VarDataCharArr(SSH_PUBKEY_AUTH_REQ),
                          new VarDataChar(1), 
                          new VarDataString<string>
                             (get<PUBKEYTYPE>(clientPubKey)),
                          new VarDataString<vector<uint8_t> > 
                             (get<PUBKEYBLOB>(clientPubKey))
                          });
          
                       createSendPacket(SSH_MSG_USERAUTH_REQUEST, 
                          {new VarDataString<string>(user),
                           new VarDataCharArr(SSH_CONNECT_STRING),
                           new VarDataCharArr(SSH_PUBKEY_AUTH_REQ),
                           new VarDataChar(1),
                           new VarDataString<string>
                                  (get<PUBKEYTYPE>(clientPubKey)),
                           new VarDataString<vector<uint8_t> > 
                               (get<PUBKEYBLOB>(clientPubKey)),
                           new VarDataRecursive(
                              { new VarDataString<string>
                                   (get<PUBKEYTYPE>(clientPubKey)),
                                new VarDataString<vector<uint8_t> >
                                    (sign)
                              })
                           });
                   }
                break;
                case SSH_MSG_SERVICE_ACCEPT:
                    TRACE("* Received SSH_MSG_SERVICE_ACCEPT: Trying pubkey.");
                    fsm.checkStatus(SSH_MSG_SERVICE_ACCEPT);
                    createSendPacket(SSH_MSG_USERAUTH_REQUEST, 
                       {new VarDataString<string>(user),
                        new VarDataCharArr(SSH_CONNECT_STRING),
                        new VarDataCharArr(SSH_PUBKEY_AUTH_REQ),
                        new VarDataChar(3), // Should be 0: so it's malformed
                        new VarDataString<string>(
                            get<PUBKEYTYPE>(clientPubKey)),
                        new VarDataString<vector<uint8_t>>(
                            get<PUBKEYBLOB>(clientPubKey))
                        });
                break;
                case SSH_MSG_USERAUTH_SUCCESS:
                    TRACE( "* Received SSH_MSG_USERAUTH_SUCCESS: auth ok.");
                    fsm.checkStatus(SSH_MSG_USERAUTH_SUCCESS);
                    createSendPacket(SSH_MSG_CHANNEL_OPEN,
                       { new VarDataCharArr(SSH_SESSION_SPEC),
                         new VarDataUint32(channelNumber),
                         new VarDataUint32(SSH_MAX_PACKET_SIZE * 4),
                         new VarDataUint32(SSH_MAX_PACKET_SIZE / 2)
                       });
                break;
                case SSH_MSG_USERAUTH_FAILURE:
                   fsm.checkStatus(SSH_MSG_USERAUTH_FAILURE);
                   isPresent  = false;
                   goto USR_NOT_PRESENT;
                case SSH_MSG_USERAUTH_BANNER:
                   // Any time: info banner.
                   TRACE("* Received SSH_MSG_USERAUTH_BANNER.");
                break;
                case SSH_MSG_UNIMPLEMENTED:
                    throw InetException("connectionLoop: Error: SSH_MSG_UNIMPLEMENTED.");
                case SSH_MSG_DISCONNECT:
                    offset  = DATA_OFFSET;
                    errCode = charToUint32(incomingEnc.data() + offset);
                    offset += sizeof(uint32_t);
                    getVariableLengthRawValue(incomingEnc, offset, tmp);
                    trace("SSH_MSG_DISCONNECT packet: ", &incomingEnc, 0, 0, 
                          charToUint32(incomingEnc.data()) + sizeof(uint32_t));
                    throw InetException(string("connectionLoop: Received SSH_MSG_DISCONNECT: ") +
                                        to_string(errCode) + " Description: " + tmp);
                case SSH_MSG_GLOBAL_REQUEST:
                    // Any time : WindowsSize, max_packet_size.
                    TRACE( "* Received SSH_MSG_GLOBAL_REQUEST.");
                    createSendPacket(SSH_MSG_REQUEST_FAILURE, {} );
                    createSendPacket(SSH_MSG_CHANNEL_OPEN,
                          { new VarDataCharArr(SSH_SESSION_SPEC),
                            new VarDataUint32(channelNumber),
                            new VarDataUint32(SSH_MAX_PACKET_SIZE * 4),
                            new VarDataUint32(SSH_MAX_PACKET_SIZE / 2)
                          });
                break;
                case SSH_MSG_CHANNEL_OPEN_CONFIRMATION:
                   fsm.checkStatus(SSH_MSG_CHANNEL_OPEN_CONFIRMATION);
                    offset  = DATA_OFFSET;
                    confirmedChannel = charToUint32(incomingEnc.data() + offset);
                    offset += sizeof(uint32_t);
                    remoteChannelNumber = charToUint32(incomingEnc.data() + offset);
                    offset += sizeof(uint32_t);
                    initialWindowsSize = charToUint32(incomingEnc.data() + offset);
                    offset += sizeof(uint32_t);
                    maxPacketSize = charToUint32(incomingEnc.data() + offset);
                    TRACE("* Received SSH_MSG_CHANNEL_OPEN_CONFIRMATION:\n ** Local Channel Number: " +
                          to_string(confirmedChannel) + "\n ** Remote Channel Number: " +
                          to_string(remoteChannelNumber) + "\n ** Initial Window Size: " +
                          to_string(initialWindowsSize) + "\n ** Max Packet Size: " +
                          to_string(maxPacketSize) + "\n");
                    if(channelNumber != confirmedChannel)
                       throw InetException("connectionLoop: Local channel number mismatch: " + 
                                           to_string(channelNumber));
                    TRACE("* Sent shell request.");
                    createSendPacket(SSH_MSG_CHANNEL_REQUEST,
                          { new VarDataUint32(remoteChannelNumber),
                            new VarDataCharArr(SSH_SHELL_REQ),
                            new VarDataChar(1) 
                          });
    
                    again = false;
                break;
                case SSH_MSG_CHANNEL_WINDOW_ADJUST:
                    offset  = DATA_OFFSET;
                    confirmedChannel = charToUint32(incomingEnc.data() + offset);
                    offset += sizeof(uint32_t);
                    bytesToAdd = charToUint32(incomingEnc.data() + offset);
                    TRACE("* Received SSH_MSG_CHANNEL_WINDOW_ADJUST:\n ** Local Channel: " + 
                          to_string(confirmedChannel) + "\n ** Bytes to Add : " +
                          to_string(bytesToAdd) + "\n");
                   initialWindowsSize += bytesToAdd;
                break;
                default:
                    trace("Dump: ", &incomingEnc, 0, 0, 
                          charToUint32(incomingEnc.data()) + sizeof(uint32_t));
                    throw InetException(string("connectionLoop: Shell Connection - Unespected Packet Type: ") + 
                                        to_string(incomingEnc[PACKET_TYPE_OFFSET]));
             }
          }
       }catch(InetClosedByHostException& ex){
            static_cast<void>(ex);
            isPresent         = true;
       }

       USR_NOT_PRESENT:

       return  isPresent;
   }

   // static SshConnection* sigRef;
   
   bool SshConnection::checkUsr() anyexcept{
      writeBuffer(getClientId());
      checkSshHeader();
   
      // Handshake
      sendWithHeader(setKexMsg(), BEGINNING_BLOCK_LEN_ALLIGN);
      readSsh();
      checkServerAlgList();

      getUserKeyFiles();
      
      vector<uint8_t> msg;
      try{
         msg.reserve(10240);
      }catch(...){
         throw InetException("checkUsr: b : Data error.");
      }
   
      //DH 
      addHeader(SSH_MSG_KEX_DH_GEX_REQUEST_OLD, msg);

      crypto.setDhKeys(genericBuffer, msg);
   
      sendWithHeader(msg, BEGINNING_BLOCK_LEN_ALLIGN);
      readSsh();

      checkServerDhReply();
   
      createKeys(currentHashCLen);
      addHeader(SSH_MSG_NEWKEYS, msg);
      sendWithHeader(msg, BEGINNING_BLOCK_LEN_ALLIGN);
   
      getUserPubK();  
  
      // Connect 
      return connectionLoop();
   }

   bool SshConnection::checkPort(string& out) anyexcept{ 
      writeBuffer(getClientId());

      checkSshHeader();
      string serverId = getServerId();
      bool   ret      = false;
      if(regex_search(serverId, sshSrvRegexp)){
          for(auto it = serverId.end(); it != serverId.begin(); --it)
              if(*it == '\r' || *it == '\n')
                   serverId.erase(it);
    
          ret         =  true;
      }
      out         = serverId;
      return ret;
   }

   Fingerprinting SshConnectionFprint::fp;

   bool SshConnectionFprint::init(const std::string& fpdb) noexcept{
         return fp.loadDatabase(fpdb);
   }

   bool  SshConnectionFprint::insertUser(const User& oc) noexcept{
          return fp.insertOccurence(oc);
   }

   bool  SshConnectionFprint::getReport(void) noexcept{
          return fp.getReport();
   }

   SshConnectionFprint::SshConnectionFprint(string& usr,      string& host,       string& port, 
                                            string& identity, uint32_t chan)
         : SshConnection(usr, host, port, identity, chan)
   {}

   #if defined  __clang_major__ && !defined __APPLE__ && __clang_major__ >= 4
   #pragma clang diagnostic pop 
   #endif
}
