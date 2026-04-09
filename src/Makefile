export version=0.3.8


mbrola_sources = mod_mbrola.c milena_mbrola.h milena_inton.h \
	milena_rintoni.c milena_intoni.c milena_strtod.c

winmilena_sources = milena.c milena.h milena_phrasi.c milena_prestri.c \
	milena_udici.c milena_transi.c milena_recogni.c milena_toiso.c \
	milena_cyrillic.h milena_keychar.c milena_ivonizer.c milena_strtod.c

ifeq "$(morfologik)" "true"
milena_sources = $(winmilena_sources) milena_morfologik.c int_morfologik.c int_morfologik.h
CFLAGS = -DHAVE_MORFOLOGIK=20
export mordef = \#define MILENA_HAS_MORFOLOGIK
else
ifeq "$(morfologik)" "mini"
milena_sources = $(winmilena_sources) milena_morfologik.c minimorf.c minimorf_cache.c minimorf.h worddancer.c
CFLAGS = -DHAVE_MORFOLOGIK=10
export mordef = \#define MILENA_HAS_MORFOLOGIK
else
milena_sources = $(winmilena_sources) def_morfologik.h
CFLAGS =
mordef = 
endif
endif

ifeq "$(intonator)" "true"
CFLAGS = -DHAVE_MORFOLOGIK=20 -DDYNAMIC_INTONATOR=1
endif

main_sources = main.c milena.h milena_mbrola.h input.c input.h serwer.c serwer.h
winmain_sources = main.c milena.h milena_mbrola.h input.c input.h

wingcc = i686-w64-mingw32-gcc

PIC =
ARCH = $(shell uname -m)
LIBDIR = $(prefix)/lib


ifeq ("$(ARCH)",$(filter "$(ARCH)","aarch64" "arm64" "x86_64"))   
PIC = -fPIC
ifneq "$(distro)" "debian"
LIBDIR = $(prefix)/lib64
ifneq "$(shell if [ -d $(prefix)/lib64 ]; then echo yes; fi)" "yes"
LIBDIR = $(prefix)/lib
endif
endif
endif



milena_cyrillic.h:	milena_cyrillic.in milena_cyrillic.rules
	bash ./mkcyr.sh
milena.h:	milena.h.in
	sed -e s/{VERSION}/$(version)/ -e s\\{DATAPATH}\\$(milena_dir)\\ \
		-e s/{MORFOL}/"$(mordef)"/ \
		 < milena.h.in > milena.h
milena.pc:	milena.pc.in
	sed -e s/{VERSION}/$(version)/ -e s\\{PREFIX}\\$(prefix)\\ \
		-e s\\{LIBDIR}\\$(LIBDIR)\\ < milena.pc.in > milena.pc



int_morfologik.h: morfologik/libmorfologik.h
	sed -e s/extern/static/<morfologik/libmorfologik.h \
	    >int_morfologik.h

int_morfologik.c: morfologik/libmorfologik.c
	sed -re 's/^(void|int|struct morfologik_data \*|u_int64_t)/static \1/'\
	    -e s/libmorfologik\.h/int_morfologik.h/ \
	    <morfologik/libmorfologik.c \
	    >int_morfologik.c

def_morfologik.h: morfologik/libmorfologik.h
	awk 'BEGIN {\
		print "#ifndef def_morfologik_h";\
		print "#define def_morfologik_h";\
		print "#define WM_m (WM_m1 | WM_m2 | WM_m3)";\
		print "#define WM_n (WM_n1 | WM_n2)";\
		}\
		/^#define W/ {print $0}\
		END {print "#endif"}' < morfologik/libmorfologik.h > def_morfologik.h

mod_mbrola.o:	$(mbrola_sources)
	gcc -c -o mod_mbrola.o mod_mbrola.c $(CFLAGS) $(PIC)

my_stdio.o:		my_stdio.c my_stdio.h
	$(wingcc) -c -o my_stdio.o my_stdio.c

win_mbrola.o:	$(mbrola_sources) 
	$(wingcc) -c -o win_mbrola.o mod_mbrola.c

milena.o:	$(milena_sources)
	gcc -c -o milena.o milena.c $(PIC) $(CFLAGS)

win_milena.o:	$(winmilena_sources)  def_morfologik.h int_morfologik.c int_morfologik.h
	$(wingcc) -c -o win_milena.o milena.c $(CFLAGS)

libmilena.so:	milena.o
	gcc -o libmilena.so -shared -rdynamic milena.o

libmilena_mbrola.so:	mod_mbrola.o
	gcc -o libmilena_mbrola.so -shared -rdynamic mod_mbrola.o

input.o: input.c input.h
	gcc -c -o input.o input.c

serwer.o: serwer.c serwer.h
	gcc -c -o serwer.o -fsigned-char serwer.c


main.o:	main.c milena.h milena_mbrola.h 
	gcc -c -o main.o main.c $(CFLAGS)

milena:	libmilena_mbrola.so libmilena.so main.o input.o serwer.o
	gcc -o milena main.o input.o serwer.o -L. -lmilena -lmilena_mbrola

all:	milena milena.pc

clean:
	rm -f *~ *.o *.so *.exe milena milena.h milena.pc milena_cyrillic.h \
		int_morfologik.c int_morfologik.h def_morfologik.h

install:	milena
	install -m 0755 milena $(DESTDIR)$(prefix)/bin
	install -m 0755 libmilena.so libmilena_mbrola.so $(DESTDIR)$(LIBDIR)
	install -m 0644 milena.h milena_mbrola.h $(DESTDIR)$(prefix)/include/
	if [ -d $(LIBDIR)/pkgconfig ] ; then install -m 0644 milena.pc $(DESTDIR)$(LIBDIR)/pkgconfig; fi

uninstall:
	rm -f $(DESTDIR)$(prefix)/bin/milena
	rm -f $(DESTDIR)$(LIBDIR)/libmilena.so $(DESTDIR)$(LIBDIR)/libmilena_mbrola.so
	rm -f $(DESTDIR)$(prefix)/include/milena.h $(DESTDIR)$(prefix)/include/milena_mbrola.h
	rm -f $(DESTDIR)$(LIBDIR)/pkgconfig/milena.pc

milena.exe: my_stdio.o win_mbrola.o win_milena.o main.c
	$(wingcc) -o milena.exe main.c input.c my_stdio.o win_mbrola.o win_milena.o $(CFLAGS)
