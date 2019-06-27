
CC=/usr/bin/g++
CFLAGS=-m64 -fpermissive -g -Wall -Wno-return-type -Wno-unknown-pragmas -DLINUX_TARGET
LFLAGS=-m64 -lcrypt -lnsl -lm -lrt -levxa -L/ltx/customer/lib64 -Wl,-rpath /ltx/customer/lib64

INCDIR=-I.  -I/ltx/customer/include -I/ltx/customer/include/evxa

DEMO_PROGS= demo test clean

APL_FILES  = demo.cpp app.cpp utility.cpp tester.cpp fd.cpp notify.cpp stdf.cpp socket.cpp xml.cpp state.cpp 

APL_OBJS = $(APL_FILES:.cpp=.o)

TEST_FILES = test.cpp app.cpp utility.cpp tester.cpp fd.cpp notify.cpp stdf.cpp socket.cpp xml.cpp state.cpp

TEST_OBJS = $(TEST_FILES:.cpp=.o)


all : $(DEMO_PROGS)

demo.o : $(APL_FILES)
	$(CC) -c $(CFLAGS) $(INCDIR) $(APL_FILES)

demo : $(APL_OBJS)
	$(CC) -o apl_exec  $(APL_OBJS) $(LIB_FILES) $(LFLAGS)


test.o : $(TEST_FILES)
	$(CC) -c $(CFLAGS) $(INCDIR) $(TEST_FILES)

test : $(TEST_OBJS)
	$(CC) -o test_exec  $(TEST_OBJS) $(LIB_FILES) $(LFLAGS)


clean :
	-rm *.o
