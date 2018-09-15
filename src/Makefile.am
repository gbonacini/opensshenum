bin_PROGRAMS     = opensshenum
dist_man_MANS    = ../doc/opensshenum.1
dist_bin_SCRIPTS = parallenum.sh

nobase_include_HEADERS   = ../include/Crypto.hpp ../include/Inet.hpp ../include/Main.hpp ../include/StringUtils.hpp ../include/OpenSshEnum.hpp ../include/Types.hpp ../include/parseCmdLine.hpp
opensshenum_SOURCES = Main.cpp InetImpl.cpp StringUtilsImpl.cpp OpenSshEnum.cpp CryptoImpl.cpp TypesImpl.cpp parseCmdLine.cpp

TypesImpl.o : CXXFLAGS += -fpermissive

