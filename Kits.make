#
# Makefile for Visualization Toolkit sources. 
# 
#------------------------------------------------------------------------------
#

SHELL = /bin/sh
.SUFFIXES: .cxx

#------------------------------------------------------------------------------

CXX_FLAGS = ${CPPFLAGS} ${VTK_SHLIB_CFLAGS} ${XCFLAGS} ${CXXFLAGS} \
	${VTK_INLINE_FLAGS} ${VTK_TEMPLATE_FLAGS} -I${srcdir} \
	${XGLR_FLAGS} ${OGLR_FLAGS} ${SBR_FLAGS} ${GLR_FLAGS} \
	 -I${srcdir}/../core -I${TK_INCLUDE} -I${TCL_INCLUDE} \
	-D_HP_NO_FAST_MACROS ${HAVE_SETMON} ${WORDS_BIGENDIAN}

all:  ${VTK_LIB_FILE} ${BUILD_TCL}

.cxx.o:
	${CXX} ${CXX_FLAGS} -c $< -o $@


#------------------------------------------------------------------------------
# rules for the normal library
#
libVTK${ME}.a: ${SRC_OBJ} ${SBR_OBJ} ${XGLR_OBJ} ${OGLR_OBJ} ${GLR_OBJ}
	${AR} cr libVTK${ME}.a ${SRC_OBJ} ${SBR_OBJ} ${XGLR_OBJ} ${OGLR_OBJ} ${GLR_OBJ} 
	${RANLIB} libVTK$(ME).a


libVTK$(ME)$(SHLIB_SUFFIX)$(SHLIB_VERSION): ${SRC_OBJ} ${SBR_OBJ} ${XGLR_OBJ} ${OGLR_OBJ} ${GLR_OBJ}
	rm -f libVTK$(ME)$(SHLIB_SUFFIX)$(SHLIB_VERSION)
	$(SHLIB_LD) -o libVTK$(ME)$(SHLIB_SUFFIX)$(SHLIB_VERSION) \
	   ${SRC_OBJ} ${SBR_OBJ} ${XGLR_OBJ} ${OGLR_OBJ} ${GLR_OBJ} ${SHLIB_LD_LIBS}

#------------------------------------------------------------------------------
# rules for the tcl library
#
build_tcl: ${TCL_LIB_FILE}

tcl/${ME}Init.cxx: ../tcl/kit_init ${TCL_NEWS}
	../tcl/kit_init VTK${ME}Tcl ${TCL_NEWS} > tcl/${ME}Init.cxx

libVTK${ME}Tcl.a: tcl/${ME}Init.o ${KIT_LIBS} ${SRC_OBJ} ${TCL_OBJ} ${SBR_OBJ} ${XGLR_OBJ} ${OGLR_OBJ} ${GLR_OBJ}
	${AR} cr libVTK${ME}Tcl.a tcl/${ME}Init.o ${KIT_LIBS} ${TCL_OBJ} ${SRC_OBJ} ${SBR_OBJ} ${XGLR_OBJ} ${OGLR_OBJ} ${GLR_OBJ} 
	${RANLIB} libVTK$(ME)Tcl.a

libVTK$(ME)Tcl$(SHLIB_SUFFIX)$(SHLIB_VERSION): tcl/${ME}Init.o ${KIT_LIBS} ${TCL_OBJ} $(SRC_OBJ) ${SBR_OBJ} ${XGLR_OBJ} ${OGLR_OBJ} ${GLR_OBJ}
	rm -f libVTK$(ME)Tcl$(SHLIB_SUFFIX)$(SHLIB_VERSION)
	$(SHLIB_LD) -o libVTK$(ME)Tcl$(SHLIB_SUFFIX)$(SHLIB_VERSION) \
	   tcl/${ME}Init.o ${KIT_LIBS} ${TCL_OBJ} ${SRC_OBJ} ${SBR_OBJ} ${XGLR_OBJ} ${OGLR_OBJ} ${GLR_OBJ} ${SHLIB_LD_LIBS} 


#------------------------------------------------------------------------------
depend: 
	$(CXX) -MM $(CPPFLAGS) $(CXX_FLAGS) $(srcdir)/*.cxx > depend.make

#------------------------------------------------------------------------------
clean: ${CLEAN_TCL}
	-rm -f *.o *.a *.so *.sl *~ *.make Makefile

clean_tcl:
	-cd tcl; rm -f *



