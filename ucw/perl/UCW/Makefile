# More Perl modules

DIRS+=ucw/perl/UCW
EXTRA_RUNDIRS+=lib/perl5/UCW
UCW_PERL_MODULES=$(addsuffix .pm,Config Log CGI)
# XXX: Configure.pm omitted intentionally, see install-perl-ucw-configure
PROGS+=$(addprefix $(o)/ucw/perl/UCW/,$(UCW_PERL_MODULES))

include $(s)/ucw/perl/UCW/CGI/Makefile
include $(s)/ucw/perl/UCW/Configure/Makefile

UCW_PERL_INSTALL+=install-perl-ucw-base
install-perl-ucw-base:
	install -d -m 755 $(DESTDIR)$(INSTALL_PERL_DIR)/UCW
	install -m 644 $(addprefix $(s)/ucw/perl/UCW/,$(UCW_PERL_MODULES)) $(DESTDIR)$(INSTALL_PERL_DIR)/UCW
.PHONY: install-perl-ucw-base
