# Perl modules for Configure

DIRS+=ucw/perl/UCW/Configure
EXTRA_RUNDIRS+=lib/perl5/UCW/Configure
UCW_CONFIGURE_PERL_MODULES=$(addsuffix .pm, C Doc Paths Pkg Build Perl)
CONFIGURE_MODULES=$(addprefix $(o)/ucw/perl/UCW/Configure/,$(UCW_CONFIGURE_PERL_MODULES))
PROGS+=$(CONFIGURE_MODULES)

$(CONFIGURE_MODULES) : PERL_MODULE_DIR=UCW/Configure

INSTALL_TARGETS+=install-perl-ucw-configure

# XXX: This is a hack. UCW::Configure is a part of the build system,
# not of LibUCW proper. Nevertheless, we should avoid moving things
# around in point releases, so we are keeping it here with a special
# installation target.
install-perl-ucw-configure:
	install -d -m 755 $(DESTDIR)$(INSTALL_PERL_DIR)/UCW/Configure
	install -m 644 $(s)/ucw/perl/UCW/Configure.pm $(DESTDIR)$(INSTALL_PERL_DIR)/UCW
	install -m 644 $(addprefix $(s)/ucw/perl/UCW/Configure/,$(UCW_CONFIGURE_PERL_MODULES)) $(DESTDIR)$(INSTALL_PERL_DIR)/UCW/Configure
.PHONY: install-perl-ucw-configure
