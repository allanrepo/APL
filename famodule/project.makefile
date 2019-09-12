TARGET_MINOR_VERSION = 1

SO_TARGET = fademo_hooks.so


# -- Start of project files --
PROJECT_SOURCES =	fademo_attach.cpp \
			fademo_hooks.cpp \

PROJECT_HEADERS   = 	fademo_hooks.h \


 
NO_PTHREAD=true

# -- lib64 is used for 64bit compile. --
# -- force 64bit compile by MBITS=64 --
MBITS=64
PROJECT_INCLUDE_PATHS = -DLINUX_TARGET -DLINUX -DUNISON -I. -I/ltx/include 
PROJECT_LIBRARIES=-Wl,-rpath,/ltx/lib$(MBITS) -L/ltx/lib$(MBITS) -levxa

ifeq ("$(BUILD_OS)", "Linux")
CFLAGS:=-DLINUX_TARGET
endif

LDFLAGS:=
