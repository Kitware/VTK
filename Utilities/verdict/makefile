# Copyright (c) 2003 Sandia National Laboratories <cubit@sandia.gov>

# makefile for verdict library

# If you change the version here, change it in include/verdict.h too.
VERDICT_MAJOR_VERSION = 1
VERDICT_MINOR_VERSION = 1
VERDICT_BUILD_VERSION = 2
VERDICT_VERSIONA = ${VERDICT_MAJOR_VERSION}${VERDICT_MINOR_VERSION}${VERDICT_BUILD_VERSION}
VERDICT_VERSIONB = ${VERDICT_MAJOR_VERSION}.${VERDICT_MINOR_VERSION}.${VERDICT_BUILD_VERSION}

include  ./VERDICT.$(MACHINE_TYPE)

SHELL = /bin/sh

SRCS = \
	V_EdgeMetric.cpp \
	V_GaussIntegration.cpp \
	V_HexMetric.cpp \
	V_KnifeMetric.cpp \
	V_PyramidMetric.cpp \
	V_QuadMetric.cpp \
	V_TetMetric.cpp \
	V_TriMetric.cpp \
	VerdictVector.cpp \
	V_WedgeMetric.cpp

HEADERS = \
	verdict_defines.hpp \
	include/verdict.h \
	VerdictVector.hpp \
	v_vector.h \
	V_GaussIntegration.hpp

MISC_FILES = \
	LICENSE \
	README \
	Verdict.htm \
	Verdict.doc \
	verdict_test.cpp \
        docs/hex.tex \
        docs/hex.eps \
        docs/tri.tex \
        docs/tri.eps \
        docs/quad.tex\
        docs/quad.eps 
        

BUILD_FILES = \
        makefile \
        VERDICT.hp \
        VERDICT.hp11 \
        VERDICT.da \
        VERDICT.sg \
        VERDICT.lin \
        VERDICT.ss \
        verdict.dsp

OBJS = ${SRCS:.cpp=.o}

# build options

default: ${VERDICT_LIB_NAME}

${VERDICT_LIB_NAME} : ${OBJS}
	${LINKER} ${VERDICT_LIB_NAME} ${LFLAGS} ${OBJS} ${LIBS}

test : ${VERDICT_LIB_NAME}
	@ echo Building verdict_test
	@ ${CXX} verdict_test.cpp ${CXXFLAGS} ${FLOAT} -L. -lverdict${VERDICT_VERSIONA} \
		-o verdict_test
clean_all :
	@- rm -f *.o *~ ./#*  *.a .#* *.so
	@- rm -rf ${TEMPLATE_DIR}
	@- rm -f ${SOURCE_DIR}/*~ 
	@- rm -f ${SOURCE_DIR}/#*



.SUFFIXES: .cpp

.cpp.o:
	${CXX} -I include ${CXXFLAGS} ${FLOAT} -c $<

clean :
	@ rm -rf *.o verde_test ${VERDICT_LIB_NAME}


DIST_FILES = ${SRCS} ${HEADERS} ${MISC_FILES} ${BUILD_FILES}
TAR_DIR_NAME = verdict${VERDICT_VERSIONB}

# assemble a source distribution package
dist :
	mkdir ${TAR_DIR_NAME} ; \
	cp ${DIST_FILES} ${TAR_DIR_NAME} ; \
	tar cf verdict${VERDICT_VERSIONA}.tar ${TAR_DIR_NAME} ; \
	gzip verdict${VERDICT_VERSIONA}.tar ; \
	rm -rf ${TAR_DIR_NAME}

# assemble a kit for application developers to use verdict
# contains include/verdict.h and the library
userkit :
	mkdir ${TAR_DIR_NAME} ; \
	mkdir ${TAR_DIR_NAME}/lib ; \
	cp libverdict*.* ${TAR_DIR_NAME}/lib ;\
	mkdir ${TAR_DIR_NAME}/include ; \
	cp include/verdict.h ${TAR_DIR_NAME}/include ; \
	tar cf verdict${VERDICT_VERSIONA}-${MACHINE_TYPE}.tar ${TAR_DIR_NAME} ; \
	gzip verdict${VERDICT_VERSIONA}-${MACHINE_TYPE}.tar ; \
	rm -rf ${TAR_DIR_NAME}


#dependencies

${OBJS} : include/verdict.h verdict_defines.hpp v_vector.h
verdict_test.o : include/verdict.h v_vector.h

V_EdgeMetric.o: include/verdict.h 
V_GaussIntegration.o: V_GaussIntegration.hpp
V_HexMetric.o: VerdictVector.hpp include/verdict.h
V_HexMetric.o: V_GaussIntegration.hpp verdict_defines.hpp v_vector.h
V_KnifeMetric.o: include/verdict.h VerdictVector.hpp
V_PyramidMetric.o: include/verdict.h VerdictVector.hpp
V_QuadMetric.o: include/verdict.h VerdictVector.hpp
V_QuadMetric.o: verdict_defines.hpp v_vector.h V_GaussIntegration.hpp
V_TetMetric.o: include/verdict.h verdict_defines.hpp
V_TetMetric.o: v_vector.h
V_TetMetric.o: VerdictVector.hpp V_GaussIntegration.hpp
V_TriMetric.o: include/verdict.h verdict_defines.hpp
V_TriMetric.o: v_vector.h
V_TriMetric.o: VerdictVector.hpp V_GaussIntegration.hpp
VerdictVector.o: VerdictVector.hpp include/verdict.h
V_WedgeMetric.o: include/verdict.h VerdictVector.hpp

