all:	pl_phraser.dat

pl_phraser.dat: pl_phraser.dat.in pl_imiona.in
	cat pl_phraser.dat.in pl_imiona.in >pl_phraser.dat

install:
	mkdir -p $(DESTDIR)$(milena_dir)
	install -m 0644 *.dat $(DESTDIR)$(milena_dir)
clean:
	rm -f *~ pl_phraser.dat

uninstall:
	for i in pl_genpor_theme.dat pl_phraser.dat *.dat;do rm -f $(DESTDIR)$(milena_dir)/"$$i";done
	if [ -d $(DESTDIR)$(milena_dir) ] ; then rmdir --ignore-fail-on-non-empty $(DESTDIR)$(milena_dir);fi
