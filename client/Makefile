CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 -std=c++11 -lstdc++ -pthread

OBJS =		Socket.o ClientSocket.o client_thread.o

OTHERS =	-pthread -std=c++11

TARGET = 	client

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS) $(OTHERS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
