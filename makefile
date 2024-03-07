# Based off the GNU Make tutorial: https://www.gnu.org/software/make/manual/make.html#Introduction

db_objects = dbmanager.o cryptowrapper.o
main_objs = main.o parsecmd.o
cppstd = -std=c++14
db_libraries = -l sqlite3 cryptopp890/libcryptopp.a

All : runtests securedb

runtests : tests.o $(db_objects)
	g++ $(cppstd) tests.o $(db_objects) $(db_libraries) -o runtests

securedb : $(main_objs) $(db_objects)
	g++ $(cppstd) $(main_objs) $(db_objects) $(db_libraries) -o securedb 

main.o : main.cpp dbmanager.h cryptowrapper.h parsecmd.h
	g++ $(cppstd) -c main.cpp

tests.o : tests.cpp dbmanager.h cryptowrapper.h
	g++ $(cppstd) -c tests.cpp

dbmanager.o : dbmanager.cpp dbmanager.h cryptowrapper.h
	g++ $(cppstd) -c dbmanager.cpp

cryptowrapper.o : cryptowrapper.cpp cryptowrapper.h
	g++ $(cppstd) -c cryptowrapper.cpp

parsecmd.o : parsecmd.cpp parsecmd.h
	g++ $(cppstd) -c parsecmd.cpp

clean :
	rm securedb runtests main.o tests.o dbmanager.o cryptowrapper.o parsecmd.o