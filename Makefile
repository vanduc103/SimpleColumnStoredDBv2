CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 -std=c++11 -lstdc++ -L/usr/local/lib/ \
			-I/usr/local/boost_1_61_0 -I/sql-parser -I/server

OBJS =		Table.o porter2_stemmer.o Dictionary.o Column.o ColumnBase.o PackedArray.o Util.o Transaction.o GarbageCollector.o \
			$(patsubst %.o,server/%.o,ServerSocket.o Socket.o) App.o server_main.o 

LIBS =		-L/usr/local/lib/ -lsqlparser

OTHERS =	-pthread -std=c++11

TARGET =	serverApp

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS) $(OTHERS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
