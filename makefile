
CC=/usr/bin/g++
CFLAGS=-m64 -fpermissive -g -Wall -Wno-return-type -Wno-unknown-pragmas -DLINUX_TARGET
LFLAGS=-m64 -lcrypt -lnsl -lm -lrt -levxa -L/ltx/customer/lib64 -Wl,-rpath /ltx/customer/lib64

INCDIR=-I.  -I/ltx/customer/include -I/ltx/customer/include/evxa

DEMO_PROGS= demo

CEX_FILES= demo.cpp app.cpp utility.cpp tester.cpp arg.cpp fd.cpp notify.cpp

CEX_OBJS = $(CEX_FILES:.cpp=.o)

all : $(DEMO_PROGS)

demo.o : $(CEX_FILES)
	$(CC) -c $(CFLAGS) $(INCDIR) $(CEX_FILES)

demo : $(CEX_OBJS)
	$(CC) -o _cex  $(CEX_OBJS) $(LIB_FILES) $(LFLAGS)



