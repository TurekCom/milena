EXE = dykcjonarz milenizer milena_convert milena_book milena_say milena_kadu milena_nokia
MILOBJS = milenizer.o readfile.o unformat.o unrtf.o myctype.o splitter.o convert.o pdfxml.o

DEFS = -DPDF_PARSER_STANDALONE=1
ifeq "$(shell if pkg-config enca --exists; then echo yes;else echo no;fi)" "yes"

INCS = $(shell pkg-config enca --cflags)
LIBS = $(shell pkg-config enca --libs)
DEFS = -DHAVE_ENCA=1 -DPDF_PARSER_STANDALONE=1
endif

ifeq "$(prefix)" "/usr"
etcdir="/etc/milena"
else
etcdir="$(prefix)/etc/milena"
endif

hash=
ifeq "$(distro)" "PLD Linux"

hash=\#

endif

all: $(EXE) milena.conf

milena_say: milena_say.in
	sed -e s:%PLDNOS%:$(hash): \
		-e s:%PREFIX%:$(prefix): \
		-e s:%ETCDIR%:$(etcdir): \
		-e s:%MBROLA%:$(mbrola): \
		-e s:%VOICE%:$(mbrola_voice): \
		< milena_say.in \
		> milena_say
	chmod 755 milena_say

milena_book: milena_book.in
	sed -e s:%PLDNOS%:$(hash): \
		-e s:%PREFIX%:$(prefix): \
		-e s:%ETCDIR%:$(etcdir): \
		-e s:%MBROLA%:$(mbrola): \
		-e s:%VOICE%:$(mbrola_voice): \
		< milena_book.in \
		> milena_book
	chmod 755 milena_book

milena_kadu: milena_kadu.in
	sed -e s:%PREFIX%:$(prefix): \
		-e s:%ETCDIR%:$(etcdir): \
		-e s:%DATADIR%:$(milena_dir): \
		-e s:%MBROLA%:$(mbrola): \
		-e s:%VOICE%:$(mbrola_voice): \
		< milena_kadu.in \
		> milena_kadu
	chmod 755 milena_kadu

milena_nokia: milena_nokia.in
	sed -e s:%PLDNOS%:$(hash): \
		-e s:%PREFIX%:$(prefix): \
		-e s:%ETCDIR%:$(etcdir): \
		-e s:%MBROLA%:$(mbrola): \
		-e s:%VOICE%:$(mbrola_voice): \
		< milena_nokia.in \
		> milena_nokia
	chmod 755 milena_nokia

milena.conf: milena.conf.in
	sed -e s:%PLDNOS%:$(hash): \
		-e s:%PREFIX%:$(prefix): \
		-e s:%MBROLA%:$(mbrola): \
		-e s:%VOICE%:$(mbrola_voice): \
		-e s:%DATADIR%:$(milena_dir): \
		< milena.conf.in \
		> milena.conf


dykcjonarz: dykcjonarz.o morfologik.o
	gcc -o dykcjonarz dykcjonarz.o morfologik.o -L../src -lmilena

dykcjonarz.o: dykcjonarz.c morfologik.h
	gcc -o dykcjonarz.o -Wall -c  -I ../src dykcjonarz.c

morfologik.o: morfologik.c morfologik.h
	gcc -o morfologik.o -Wall -c  morfologik.c

milena_convert: filter.o convert.o
	gcc -o milena_convert filter.o convert.o -L../src -lmilena

milenizer: $(MILOBJS)
	gcc -o milenizer $(MILOBJS) $(LIBS) -L../src -lmilena

milenizer.o: milenizer.c milenizer.h
	gcc -c -Wall -I ../src -o milenizer.o milenizer.c

readfile.o: readfile.c milenizer.h
	gcc -c -Wall -I ../src -o readfile.o readfile.c  $(DEFS) $(INCS)

unformat.o: unformat.c milenizer.h
	gcc -c -Wall -I ../src -o unformat.o unformat.c

unrtf.o: unrtf.c milenizer.h
	gcc -c -Wall -I ../src -o unrtf.o unrtf.c

myctype.o: myctype.c
	gcc -c -Wall -I ../src -o myctype.o myctype.c

splitter.o: splitter.c milenizer.h
	gcc -c -Wall -I ../src -o splitter.o splitter.c

convert.o: convert.c milenizer.h
	gcc -c -Wall -I ../src -o convert.o convert.c

filter.o: filter.c milenizer.h
	gcc -c -Wall -I ../src  -o filter.o filter.c

pdfxml.o: pdfxml.c
	gcc -c -Wall $(DEFS) -I ../src -o pdfxml.o pdfxml.c

clean:
	rm -f $(EXE) *~ *.o milena.conf

install:
	install -m 0755 $(EXE) $(DESTDIR)$(prefix)/bin/
	install -d $(DESTDIR)$(etcdir)
	install -m 0644 milena.conf $(DESTDIR)$(etcdir)

uninstall:
	for i in $(EXE);do rm -f $(DESTDIR)$(prefix)/bin/"$$i";done
	rm -f $(DESTDIR)$(etcdir)/milena.conf
	rmdir --ignore-fail-on-non-empty $(DESTDIR)$(etcdir)
