Description:
============

Opensshenum is an OpenSsh user enumerator permitting to verify an arbitrary list of user names against an OpenSsh server, to know those actually presents on that remote machine. This program  exploits a bug ( corrected in July 2018  ) proven to be effective at least starting from OpenSSH 2.3.0 ( released in November 2000).

I wrote the expoit starting from Tssh, a SSH 2 client I wrote in C++11 from scratch starting from the RFCs.

For more information read here:

http://seclists.org/oss-sec/2018/q3/124

Prerequisites:
==============

The program is intended to be used in a *nix environment and it is tested on various Linux distributions and OS X:

- Ubuntu 17.10;
- OS X 10.11.6;

using, as compiler, one in this list:

- gcc version7.2.0 (Ubuntu 7.2.0-8ubuntu3.2);
- Apple LLVM version 8.0.0 (clang-800.0.42.1)

and, as ssh server, one of the following:

- OpenSSH_7.5p1 

The only external dependency is the OpenSSL library, used for the cryptographic functions.
I could introduce alternatives to OpenSSL in the next versions.
This program is intended to be used with an OpenSSL version equal or superior to:

- OpenSSL 1.0.2h;

( This means that with OS X, an upgrade is mandatory).

To compile the program, this tools/libraries are necessary:

- a c++ compiler ( with c++11 support);
- automake/autoconf;
- libtool;
- OpenSSL 1.0.2h or superior ("dev" packages);

Installation:
=============

- launch the configure script:
  ./configure
- Compile the program:
  make
- Install the program and the man page:
  sudo make install

Instructions:
=============

It elaborates the user list passed in pipe, example:

cat user.txt | ./opensshenum -i dummy_cert target_addr<BR>
xxx:NOK
gabriel:OK

where user.txt is a file containing a list of users, one for each line 
and dummy_cert is a key generate with the commang ssh-keygen of OpenSsh 
( the directory of the keys is ~/.shh as default).

A program could be used to geerate le list:

printf "xxx\ngabriel" | ./src/opensshenum   -i dummy_rsa localhost <BR>
xxx:NOK
gabriel:OK

See the man page included in this release.

