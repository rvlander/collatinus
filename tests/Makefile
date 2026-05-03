CXX      = g++
CXXFLAGS = -std=c++11 -Wall -I../src

SRC = ../src
CORE_SRCS = $(SRC)/ch.cpp

LEMCORE_SRCS = $(SRC)/ch.cpp \
               $(SRC)/lemCore.cpp \
               $(SRC)/lemme.cpp \
               $(SRC)/modele.cpp \
               $(SRC)/irregs.cpp

all: test_ch test_lemcore

test_ch: test_ch.cpp test_utils.h $(CORE_SRCS)
	$(CXX) $(CXXFLAGS) $(CORE_SRCS) test_ch.cpp -o test_ch

test_lemcore: test_lemcore.cpp test_utils.h $(LEMCORE_SRCS)
	$(CXX) $(CXXFLAGS) $(LEMCORE_SRCS) test_lemcore.cpp -o test_lemcore

clean:
	rm -f test_ch test_lemcore
