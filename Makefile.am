AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = \
  -I$(includedir) \
  -I$(OFFLINE_MAIN)/include \
  -I$(ROOTSYS)/include

AM_LDFLAGS = \
  -L$(libdir) \
  -L$(OFFLINE_MAIN)/lib \
  -L$(OFFLINE_MAIN)/lib64

AM_CXXFLAGS = -O1 -g

pkginclude_HEADERS = \
  CaloAlignment.h

lib_LTLIBRARIES = \
  libCaloAlignment.la

libCaloAlignment_la_SOURCES = \
  CaloAlignment.cc

libCaloAlignment_la_LIBADD = \
  -lphool \
  -lSubsysReco \
  -lfun4all \
  -lNoRootEvent


BUILT_SOURCES = testexternals.cc

noinst_PROGRAMS = \
  testexternals

testexternals_SOURCES = testexternals.cc
testexternals_LDADD   = libCaloAlignment.la

testexternals.cc:
	echo "//*** this is a generated file. Do not commit, do not edit" > $@
	echo "int main()" >> $@
	echo "{" >> $@
	echo "  return 0;" >> $@
	echo "}" >> $@

clean-local:
	rm -f $(BUILT_SOURCES)
