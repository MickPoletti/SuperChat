CXX=g++

CPPFLAGS=-I/media/mickpoletti/Linux/CSE3310/SuperChat/include

CXXFLAGS=-Wall -O0 -g -std=c++11

all: chat_client chat_server

COMMON_HEADER = chat_message.hpp

chat_client.o: ${COMMON_HEADER} chat_client.cpp

chat_client:chat_client.o
	${CXX} -o chat_client chat_client.o -lpthread -lncurses -lform

chat_server.o: ${COMMON_HEADER} chat_message.hpp chat_server.cpp

chat_server:chat_server.o
	${CXX} -o chat_server chat_server.o -lpthread

clean:
	-rm -f chat_client chat_server chat_client.o chat_server.o

