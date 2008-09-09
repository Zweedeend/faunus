# Specify compiler (gnu,gnu686,intel,sun,pgi,pathscale,debug)
MODEL = gnu

# Set to yes if you need Gromacs xtc file support
# (requires a working Gromacs installation)
GROMACS = no

# Set to "yes" to enable parallel execution on multi-core
# CPU's. OpenMP must be supported by the compiler.
OPENMP = no

###########################################
#  Normally you would not want to modify  #
#  things beyond this point.              #
###########################################

CXX=g++
CLASSDIR=src
INCDIR=-Iinclude
LDFLAGS=-L./src -lfaunus

ifeq ($(GROMACS), yes)
  INCDIR:=${INCDIR} -I/usr/local/gromacs/include/gromacs
  LDFLAGS:=${LDFLAGS} -L/usr/local/gromacs/lib/ -lgmx
  EXTRA=-DGROMACS
endif

ifeq ($(MODEL), debug)
  CXXFLAGS = -O0 -W -Winline -Wno-sign-compare -g $(INCDIR) $(EXTRA)
endif

ifeq ($(MODEL), gnu)
  ifeq ($(OPENMP), yes)
    EXTRA:=$(EXTRA) -fopenmp
  endif
  CXXFLAGS = -O3 -w -funroll-loops $(INCDIR) $(EXTRA)
endif

ifeq ($(MODEL), gnu686)
  ifeq ($(OPENMP), yes)
    EXTRA:=$(EXTRA) -fopenmp
  endif
  CXXFLAGS = -mtune=i686 -msse3 -O3 -w -funroll-loops $(INCDIR) $(EXTRA)
endif

ifeq ($(MODEL), intel)
  ifeq ($(OPENMP), yes)
    EXTRA:=$(EXTRA) -openmp
  endif
  CXX=icc
  CXXFLAGS = -O3 -w $(INCDIR) $(EXTRA)
endif

ifeq ($(MODEL), pathscale)
  CXX=pathCC
  CXXFLAGS = -Ofast $(INCDIR) $(EXTRA)
endif

ifeq ($(MODEL), pgi)
  CXX=pgCC
  ifeq ($(OPENMP), yes)
    EXTRA:=$(EXTRA) -mp
  endif 
  CXXFLAGS = -O3 -w $(INCDIR) $(EXTRA)
endif

ifeq ($(MODEL), sun)
  ifeq ($(OPENMP), yes)
    EXTRA:=$(EXTRA) -xopenmp
  endif
  CXX=sunCC
  CXXFLAGS = -fast -w $(INCDIR) $(EXTRA)
endif
	    
OBJS=$(CLASSDIR)/inputfile.o \
     $(CLASSDIR)/io.o\
     $(CLASSDIR)/titrate.o\
     $(CLASSDIR)/point.o \
     $(CLASSDIR)/physconst.o\
     $(CLASSDIR)/slump.o\
     $(CLASSDIR)/container.o\
     $(CLASSDIR)/potentials.o\
     $(CLASSDIR)/hardsphere.o\
     $(CLASSDIR)/group.o \
     $(CLASSDIR)/particles.o \
     $(CLASSDIR)/analysis.o \
     $(CLASSDIR)/species.o 
all:	classes examples libfaunus

classes:	$(OBJS)
libfaunus:      $(OBJS)
	ar cr src/libfaunus.a $(OBJS)

#$(CXX) $(CXXFLAGS) -current_version 1.0 -compatibility_version 1.0 -fvisibility=hidden -o lib/libfaunus.A.dylib -dynamiclib $(OBJS)

manual:
	doxygen doc/Doxyfile
manualul:
	scp -rC doc/html/* mikaellund@shell.sourceforge.net:/home/groups/f/fa/faunus/htdocs/
widom:	src/examples/widom/widom.C libfaunus
	$(CXX) $(CXXFLAGS) src/examples/widom/widom.C -o src/examples/widom/widom ${LDFLAGS}
ewald:		src/examples/ewald/ewald.C libfaunus
	$(CXX) $(CXXFLAGS) src/examples/ewald/ewald.C -o src/examples/ewald/ewald ${LDFLAGS}
twobody:	src/examples/twobody/twobody.C libfaunus
	$(CXX) $(CXXFLAGS) src/examples/twobody/twobody.C -o src/examples/twobody/twobody ${LDFLAGS}
twobody-hof:	src/examples/twobody-hofmeister/twobody-hof.C libfaunus
	$(CXX) $(CXXFLAGS) src/examples/twobody-hofmeister/twobody-hof.C -o src/examples/twobody-hofmeister/twobody-hof ${LDFLAGS}
manybody:	src/examples/manybody/manybody.C libfaunus 
	$(CXX) $(CXXFLAGS) src/examples/manybody/manybody.C -o src/examples/manybody/manybody ${LDFLAGS}
manybodyclust:	src/examples/manybody/manybodyclust.C libfaunus 
	$(CXX) $(CXXFLAGS) src/examples/manybody/manybodyclust.C -o src/examples/manybody/manybodyclust ${LDFLAGS}
isobaric:	src/examples/isobaric/isobaric.C libfaunus 
	$(CXX) $(CXXFLAGS) src/examples/isobaric/isobaric.C -o src/examples/isobaric/isobaric ${LDFLAGS}
tools:	src/examples/tools/printpotential.C src/examples/tools/aam2pqr.C libfaunus 
	$(CXX) $(CXXFLAGS) src/examples/tools/printpotential.C -o src/examples/tools/printpotential ${LDFLAGS}
	$(CXX) $(CXXFLAGS) src/examples/tools/aam2pqr.C -o src/examples/tools/aam2prq ${LDFLAGS}
pka:	src/examples/titration/pka.C libfaunus
	$(CXX) $(CXXFLAGS) src/examples/titration/pka.C -o src/examples/titration/pka $(LDFLAGS) 
GCpka:	src/examples/titration/GCpka.C libfaunus
	$(CXX) $(CXXFLAGS) src/examples/titration/GCpka.C -o src/examples/titration/GCpka $(LDFLAGS) 
binding:	src/examples/binding/binding.C libfaunus
	$(CXX) $(CXXFLAGS) src/examples/binding/binding.C -o src/examples/binding/binding $(LDFLAGS) 
undone:		undone/mikael/namespace.C libfaunus
	$(CXX) $(CXXFLAGS) \
	undone/mikael/namespace.C \
	-o undone/mikael/namespace ${LDFLAGS}
examples:	binding tools widom pka GCpka ewald twobody twobody-hof manybody isobaric
clean:
	rm -f $(OBJS) *.o \
	src/examples/titration/pka \
	src/examples/widom/widom \
	src/examples/ewald/ewald \
	src/examples/twobody/twobody \
	src/examples/manybody/manybody \
	src/examples/manybody/manybodyclust \
	src/examples/isobaric/isobaric \
	src/examples/twobody-hofmeister/twobody-hof \
	src/libfaunus.a
docclean:
	rm -fR doc/html doc/latex
babel:
	#curl -L -o openbabel-2.1.1.tar.gz http://downloads.sourceforge.net/openbabel/openbabel-2.1.1.tar.gz
	#tar -zxf openbabel-2.1.1.tar.gz
	cd openbabel-2.1.1 ; ./configure ;make
	
