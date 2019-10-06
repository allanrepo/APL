# APL variables
CC=/usr/bin/g++
CFLAGS=-m64 -fpermissive -g -Wall -Wno-return-type -Wno-unknown-pragmas -DLINUX_TARGET
LFLAGS=-m64 -lcrypt -lnsl -lm -lrt -levxa -L/ltx/customer/lib64 -Wl,-rpath /ltx/customer/lib64
INCDIR = -I/ltx/customer/include -I/ltx/customer/include/evxa -I./ -I./inc -I.
APL=apl_exec
APL_FILES  = demo.cpp app.cpp utility.cpp tester.cpp fd.cpp notify.cpp stdf.cpp socket.cpp xml.cpp state.cpp 
APL_OBJS = $(APL_FILES:.cpp=.o)


APP = ./.widget
EXT = 
SRCDIR = ./
#INCDIR = ./inc /ltx/customer/include /ltx/customer/include/evxa ./
TOP_SRCDIR = /home/localuser/wxWidgets-3.0.4/
EX_TOP_BUILDDIR = /home/localuser/wxWidgets-3.0.4/cnt7StaticBuild
CXX = g++
CXXC = $(BK_DEPS) $(CXX)
BK_DEPS = /home/localuser/wxWidgets-3.0.4/cnt7StaticBuild/bk-deps
LIBDIRNAME = $(EX_TOP_BUILDDIR)/lib
LDFLAGS = -pthread   
LIBS = -lz -ldl -lm
CXXFLAGS = -DWX_PRECOMP -pthread -g -O0 -pthread -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/include/libpng15 -pthread -I/usr/include/gtk-unix-print-2.0 -I/usr/include/gtk-2.0 -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/lib64/gtk-2.0/include -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng15 -I/usr/include/libdrm -I/usr/include/harfbuzz 
CPPFLAGS = -D_FILE_OFFSET_BITS=64 ${INCDIR} -I${EX_TOP_BUILDDIR}/lib/wx/include/gtk2-unicode-static-3.0 -I${TOP_SRCDIR}/include -pthread -I/usr/include/gtk-2.0 -I/usr/lib64/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng15 -I/usr/include/libdrm -I/usr/include/harfbuzz -pthread -I/usr/include/gstreamer-1.0 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include

WXWIDGETS_CXXFLAGS = -D__WX$(TOOLKIT)__ $(__WXUNIV_DEFINE_p) $(__DEBUG_DEFINE_p) $(__EXCEPTIONS_DEFINE_p) $(__RTTI_DEFINE_p) $(__THREAD_DEFINE_p) -I$(SRCDIR) $(__DLLFLAG_p) -I$(SRCDIR)/../../samples $(CXXWARNINGS) $(CPPFLAGS) $(CXXFLAGS)
APP_OBJECTS =  \
	$(APP).o \

WIDGET_FILES = widget.cpp 
WIDGET_OBJS = $(WIDGET_FILES:.cpp=.o)

WX_LIB_FLAVOUR = 
TOOLKIT = GTK
TOOLKIT_LOWERCASE = gtk
TOOLKIT_VERSION = 2
TOOLCHAIN_FULLNAME = gtk2-unicode-static-3.0
EXTRALIBS = -pthread    -lz -ldl -lm 
EXTRALIBS_XML =  -lexpat
EXTRALIBS_GUI = -pthread -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfontconfig -lgobject-2.0 -lfreetype -lgthread-2.0 -lglib-2.0 -lX11 -lXxf86vm -lSM -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfontconfig -lgobject-2.0 -lglib-2.0 -lfreetype -lnotify -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lpng -lz -ljpeg -ltiff
CXXWARNINGS = -Wall -Wundef -Wunused-parameter -Wno-ctor-dtor-privacy -Woverloaded-virtual
HOST_SUFFIX = 

### Variables: ###

DESTDIR = 
WX_RELEASE = 3.0
WX_VERSION = $(WX_RELEASE).4

### Conditionally set variables: ###

PORTNAME = $(TOOLKIT_LOWERCASE)$(TOOLKIT_VERSION)
WXUNICODEFLAG = u
EXTRALIBS_FOR_BASE = $(EXTRALIBS)
EXTRALIBS_FOR_GUI = $(EXTRALIBS_GUI)
COND_MONOLITHIC_0___WXLIB_CORE_p = \
	-lwx_$(PORTNAME)$(WXUNIVNAME)$(WXUNICODEFLAG)$(WXDEBUGFLAG)$(WX_LIB_FLAVOUR)_core-$(WX_RELEASE)$(HOST_SUFFIX)
__WXLIB_CORE_p = $(COND_MONOLITHIC_0___WXLIB_CORE_p)
COND_MONOLITHIC_0___WXLIB_BASE_p = \
	-lwx_base$(WXBASEPORT)$(WXUNICODEFLAG)$(WXDEBUGFLAG)$(WX_LIB_FLAVOUR)-$(WX_RELEASE)$(HOST_SUFFIX)
__WXLIB_BASE_p = $(COND_MONOLITHIC_0___WXLIB_BASE_p)
COND_MONOLITHIC_1___WXLIB_MONO_p = \
	-lwx_$(PORTNAME)$(WXUNIVNAME)$(WXUNICODEFLAG)$(WXDEBUGFLAG)$(WX_LIB_FLAVOUR)-$(WX_RELEASE)$(HOST_SUFFIX)
COND_wxUSE_REGEX_builtin___LIB_REGEX_p = \
	-lwxregex$(WXUNICODEFLAG)$(WXDEBUGFLAG)$(WX_LIB_FLAVOUR)-$(WX_RELEASE)$(HOST_SUFFIX)
__LIB_REGEX_p = $(COND_wxUSE_REGEX_builtin___LIB_REGEX_p)

### Targets: ###

all: $(APP) $(APL)

clean: 
	rm -f ./*.o
#	rm -f $(APP) $(APL)

$(APP): $(WIDGET_OBJS) utility.o socket.o
	$(CXX) -o $@ $(WIDGET_OBJS) utility.o socket.o ${INCDIR} -L$(LIBDIRNAME) $(LDFLAGS)  $(__WXLIB_CORE_p)  $(__WXLIB_BASE_p)  $(__WXLIB_MONO_p) $(__LIB_SCINTILLA_IF_MONO_p) $(__LIB_TIFF_p) $(__LIB_JPEG_p) $(__LIB_PNG_p)  $(EXTRALIBS_FOR_GUI) $(__LIB_ZLIB_p) $(__LIB_REGEX_p) $(__LIB_EXPAT_p) $(EXTRALIBS_FOR_BASE) $(LIBS)
	
$(WIDGET_OBJS): $(SRCDIR)/$(WIDGET_FILES) 
	$(CXX) -c $(WXWIDGETS_CXXFLAGS) $(SRCDIR)/$(WIDGET_FILES) ${INCDIR}

$(APL): $(APL_OBJS)
	$(CC) -o $@ $(APL_OBJS) $(CFLAGS) $(LFLAGS)

$(APL_OBJS): $(APL_FILES)
	$(CC) -c $(CFLAGS) ${INCDIR} $(APL_FILES) 

apl_srvr: server.o utility.o socket.o
	$(CC) -o $@ $< $(CFLAGS) utility.o  socket.o $(LFLAGS)

server.o: $(SRCDIR)/server.cpp 
	$(CXX) -c  $< -I${INCDIR}


