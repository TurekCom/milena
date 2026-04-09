
export prefix=/usr/local
export milena_dir=$(prefix)/share/milena
export doc_dir=$(prefix)/share/doc/milena
export speechd_dir=$(shell ./find_speechd)
export distro=$(shell uname -o)
export mbrola=$(shell if [ -f ./config.dat ] ; then cat ./config.dat | awk -F= '/mbrola=/ {print $$2}'; fi)
export mbrola_voice=$(shell if [ -f ./config.dat ] ; then cat ./config.dat | awk -F= '/mbrola_voice=/ {print $$2}'; fi)
export contrast=$(shell if [ -f ./config.dat ] ; then cat ./config.dat | awk -F= '/contrast=/ {print $$2}'; fi)
export morfologik=true
export intonator=false

all: config.dat
	make -C src all
	make -C data all
	make -C utils all

milena.exe: config.dat
	make -C src milena.exe


config.dat:
	sh ./configme.sh


clean:
	rm -f config.dat
	make -C src clean
	make -C data clean
	make -C utils clean
		
install:
	if [ -f pl_basewords.dat.gz ] ; then mkdir -p $(milena_dir)-words;zcat pl_basewords.dat.gz > $(milena_dir)-words/pl_basewords.dat; chmod 0755 $(milena_dir)-words;chmod 0644 $(milena_dir)-words/pl_basewords.dat;fi
	make -C src install
	ldconfig
	mkdir -p $(doc_dir)
	install -m 644 README* $(doc_dir)
	make -C data install
	make -C utils install

uninstall:
	if [ -f pl_basewords.dat.gz ] ; then rm -f $(milena_dir)-words/pl_basewords.dat;if [ -d $(milena_dir)-words ] ; then rmdir --ignore-fail-on-non-empty $(milena_dir)-words;fi;fi
	for i in README*;do rm -f $(doc_dir)/"$$i";done
	if [ -d $(doc_dir) ] ; then rmdir --ignore-fail-on-non-empty $(doc_dir);fi
	make -C src uninstall
	ldconfig
	make -C data uninstall
	make -C utils uninstall
