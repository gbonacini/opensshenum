bin_PROGRAMS   = opensshenum
dist_man_MANS  = ../doc/opensshenum.1

nobase_include_HEADERS   = ../include/Crypto.hpp ../include/Inet.hpp ../include/Main.hpp ../include/StringUtils.hpp ../include/OpenSshEnum.hpp ../include/Types.hpp
opensshenum_SOURCES = Main.cpp InetImpl.cpp StringUtilsImpl.cpp OpenSshEnum.cpp CryptoImpl.cpp TypesImpl.cpp

TypesImpl.o : CXXFLAGS += -fpermissive

