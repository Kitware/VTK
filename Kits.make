#
# Makefile for Visualization Toolkit sources. 
# 
#------------------------------------------------------------------------------
#

SHELL = /bin/sh
.SUFFIXES: .cxx .java .class

#------------------------------------------------------------------------------

CC_FLAGS = ${CPPFLAGS} ${CFLAGS} ${VTK_SHLIB_CFLAGS}

CXX_FLAGS = ${CPPFLAGS} ${VTK_SHLIB_CFLAGS} ${XCFLAGS} ${CXXFLAGS} \
	${VTK_INLINE_FLAGS} ${VTK_TEMPLATE_FLAGS} -I${srcdir} \
	${KIT_FLAGS} -I. \
	 -I${srcdir}/../common -I${TK_INCLUDE} -I${TCL_INCLUDE} \
	-D_HP_NO_FAST_MACROS ${HAVE_SETMON} ${WORDS_BIGENDIAN}

all: ${VTK_LIB_FILE} ${BUILD_TCL} ${BUILD_JAVA}

.c.o:
	${CC} ${CC_FLAGS} -c $< -o $@
.cxx.o:
	${CXX} ${CXX_FLAGS} -c $< -o $@

targets.make: ../targets Makefile
	../targets concrete $(CONCRETE) abstract $(ABSTRACT) concrete_h $(CONCRETE_H) abstract_h $(ABSTRACT_H)

#------------------------------------------------------------------------------
# rules for the normal library
#
libVTK${ME}.a: ${SRC_OBJ} ${KIT_OBJ}
	${AR} cr libVTK${ME}.a ${KIT_OBJ}
	${RANLIB} libVTK$(ME).a


libVTK$(ME)$(SHLIB_SUFFIX)$(SHLIB_VERSION): ${KIT_OBJ}
	rm -f libVTK$(ME)$(SHLIB_SUFFIX)$(SHLIB_VERSION)
	$(SHLIB_LD) -o libVTK$(ME)$(SHLIB_SUFFIX)$(SHLIB_VERSION) \
	   ${KIT_OBJ} ${SHLIB_LD_LIBS}

#------------------------------------------------------------------------------
# rules for the tcl library
#
build_tcl: ${TCL_LIB_FILE}

tcl/${ME}Init.cxx: ../tcl/kit_init ${KIT_NEWS} Makefile
	../tcl/kit_init VTK${ME}Tcl ${KIT_NEWS} > tcl/${ME}Init.cxx

libVTK${ME}Tcl.a: tcl/${ME}Init.o ${KIT_LIBS} ${KIT_TCL_OBJ} 
	${AR} cr libVTK${ME}Tcl.a tcl/${ME}Init.o ${KIT_LIBS} ${KIT_TCL_OBJ}
	${RANLIB} libVTK$(ME)Tcl.a

libVTK$(ME)Tcl$(SHLIB_SUFFIX)$(SHLIB_VERSION): tcl/${ME}Init.o ${KIT_LIBS} ${KIT_TCL_OBJ}
	rm -f libVTK$(ME)Tcl$(SHLIB_SUFFIX)$(SHLIB_VERSION)
	$(SHLIB_LD) -o libVTK$(ME)Tcl$(SHLIB_SUFFIX)$(SHLIB_VERSION) \
	   tcl/${ME}Init.o ${KIT_LIBS} ${KIT_TCL_OBJ}

#------------------------------------------------------------------------------
# rules for the java library
#
build_java: ${JAVA_CLASSES} ${JAVA_CODE} ${JAVA_CODE_ADD} ${JAVA_O} ${JAVA_O_ADD} ${JAVA_WRAP} libVTK${ME}Java${SHLIB_SUFFIX}${SHLIB_VERSION}

.java.class:
	${JAVAC} -d ${JAVA_CLASS_HOME} $< 

libVTK$(ME)Java$(SHLIB_SUFFIX)$(SHLIB_VERSION): ${KIT_OBJ} ${JAVA_O} ${JAVA_O_ADD} ${JAVA_WRAP}
	rm -f libVTK$(ME)Java$(SHLIB_SUFFIX)$(SHLIB_VERSION)
	$(SHLIB_LD) -o libVTK$(ME)Java$(SHLIB_SUFFIX)$(SHLIB_VERSION) \
	  ${KIT_OBJ} ${JAVA_O} ${JAVA_O_ADD} ${JAVA_WRAP}

#------------------------------------------------------------------------------
depend: 
	-$(MAKE_DEPEND_COMMAND)

#------------------------------------------------------------------------------
clean: ${CLEAN_TCL} $(CLEAN_JAVA)
	-rm -f *.o *.a *.so *.sl *~ *.make Makefile

clean_tcl:
	-cd tcl; rm -f *

clean_java:
	-cd java; rm -f *



