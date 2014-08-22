module_LTLIBRARIES += modules/systemd-journal/libsdjournal.la
modules_systemd_journal_libsdjournal_la_SOURCES = \
	modules/systemd-journal/systemd-journal.c \
	modules/systemd-journal/systemd-journal.h \
	modules/systemd-journal/systemd-journal-grammar.y \
	modules/systemd-journal/systemd-journal-parser.c \
	modules/systemd-journal/systemd-journal-parser.h \
	modules/systemd-journal/systemd-journal-plugin.c

BUILT_SOURCES += modules/systemd-journal/systemd-journal-grammar.y \
		 modules/systemd-journal/systemd-journal-grammar.c \
		 modules/systemd-journal/systemd-journal-grammar.h

EXTRA_DIST += $(BUILT_SOURCES) modules/systemd-journal/systemd-journal-grammar.ym

modules_systemd_journal_libsdjournal_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/modules/systemd-journal -I$(top_builddir)/modules/systemd-journal
modules_systemd_journal_libsdjournal_la_LIBADD = $(MODULE_DEPS_LIBS)
modules_systemd_journal_libsdjournal_la_LDFLAGS = $(MODULE_LDFLAGS)
