PROJECT(VTKOGGTHEORA)

SET(OGG_VERSION 1.1.4)
SET(THEORA_VERSION 1.1.1)

# do some type checks
INCLUDE(CheckTypeSize)
CHECK_TYPE_SIZE(int16_t     VTKOGGTHEORA_INT16_T)
CHECK_TYPE_SIZE(int32_t     VTKOGGTHEORA_INT32_T)
CHECK_TYPE_SIZE(uint32_t    VTKOGGTHEORA_UINT32_T)
CHECK_TYPE_SIZE(uint16_t    VTKOGGTHEORA_UINT16_T)
CHECK_TYPE_SIZE(u_int32_t   VTKOGGTHEORA_U_INT32_T)
CHECK_TYPE_SIZE(u_int16_t   VTKOGGTHEORA_U_INT16_T)
CHECK_TYPE_SIZE(int64_t     VTKOGGTHEORA_INT64_T)
CHECK_TYPE_SIZE(short       VTKOGGTHEORA_SHORT)
CHECK_TYPE_SIZE(int         VTKOGGTHEORA_INT)
CHECK_TYPE_SIZE(long        VTKOGGTHEORA_LONG)
CHECK_TYPE_SIZE("long long" VTKOGGTHEORA_LONG_LONG)

IF(HAVE_VTKOGGTHEORA_INT16_T)
  SET(SIZE16 int16_t)
ELSEIF(VTKOGGTHEORA_SHORT EQUAL 2)
  SET(SIZE16 short)
ELSEIF(VTKOGGTHEORA_INT EQUAL 2)
  SET(SIZE16 int)
ELSE(HAVE_VTKOGGTHEORA_INT16_T)
  MESSAGE(SEND_ERROR "No 16 bit type found")
ENDIF(HAVE_VTKOGGTHEORA_INT16_T)

IF(HAVE_VTKOGGTHEORA_INT32_T)
  SET(SIZE32 int32_t)
ELSEIF(VTKOGGTHEORA_SHORT EQUAL 4)
  SET(SIZE32 short)
ELSEIF(VTKOGGTHEORA_INT EQUAL 4)
  SET(SIZE32 int)
ELSEIF(VTKOGGTHEORA_LONG EQUAL 4)
  SET(SIZE32 long)
ELSE(HAVE_VTKOGGTHEORA_INT32_T)
  MESSAGE(SEND_ERROR "No 32 bit type found")
ENDIF(HAVE_VTKOGGTHEORA_INT32_T)

IF(HAVE_VTKOGGTHEORA_UINT32_T)
  SET(USIZE32 uint32_t)
ELSEIF(VTKOGGTHEORA_SHORT EQUAL 4)
  SET(USIZE32 "unsigned short")
ELSEIF(VTKOGGTHEORA_INT EQUAL 4)
  SET(USIZE32 "unsigned int")
ELSEIF(VTKOGGTHEORA_LONG EQUAL 4)
  SET(USIZE32 "unsigned long")
ELSE(HAVE_VTKOGGTHEORA_UINT32_T)
  MESSAGE(SEND_ERROR "No unsigned 32 bit type found")
ENDIF(HAVE_VTKOGGTHEORA_UINT32_T)

IF(HAVE_VTKOGGTHEORA_UINT16_T)
  SET(USIZE16 uint16_t)
ELSEIF(VTKOGGTHEORA_SHORT EQUAL 2)
  SET(USIZE16 "unsigned short")
ELSEIF(VTKOGGTHEORA_INT EQUAL 2)
  SET(USIZE16 "unsigned int")
ELSEIF(LONG EQUAL 2)
  SET(USIZE16 "unsigned long")
ELSE(HAVE_VTKOGGTHEORA_UINT16_T)
  MESSAGE(SEND_ERROR "No unsigned 16 bit type found")
ENDIF(HAVE_VTKOGGTHEORA_UINT16_T)

IF(HAVE_VTKOGGTHEORA_INT64_T)
  SET(SIZE64 int64_t)
ELSEIF(VTKOGGTHEORA_INT EQUAL 8)
  SET(SIZE64 int)
ELSEIF(VTKOGGTHEORA_LONG EQUAL 8)
  SET(SIZE64 long)
ELSEIF(VTKOGGTHEORA_LONG_LONG EQUAL 8)
  SET(SIZE64 "long long")
ELSE(HAVE_VTKOGGTHEORA_INT64_T)
  MESSAGE(SEND_ERROR "No 64 bit type found")
ENDIF(HAVE_VTKOGGTHEORA_INT64_T)

# check whether compiler supports named ASM params
SET(VTKOGGTHEORA_COMPILER_SUPPORTS_NAMED_ASM_PARAMS TRUE)
# for gcc anything below 3.1 does not, so don't even ask user
IF(CMAKE_COMPILER_IS_GNU_C)
  MESSAGE(STATUS "Checking whether GCC ${CMAKE_C_COMPILER} is older than 3.1")
  IF(NOT VTKOGGTHEORA_GCC_VERSION)
    EXECUTE_PROCESS(
      COMMAND ${CMAKE_C_COMPILER} -dumpversion
      OUTPUT_VARIABLE VTKOGGTHEORA_GCC_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    SET(VTKOGGTHEORA_GCC_VERSION ${VTKOGGTHEORA_GCC_VERSION} CACHE INTERNAL "The gcc version")
    IF(${VTKOGGTHEORA_GCC_VERSION} VERSION_LESS 3.1.0)
      SET(VTKOGGTHEORA_COMPILER_SUPPORTS_NAMED_ASM_PARAMS FALSE)
    ENDIF(${VTKOGGTHEORA_GCC_VERSION} VERSION_LESS 3.1.0)
  ENDIF(NOT VTKOGGTHEORA_GCC_VERSION)
ENDIF(CMAKE_COMPILER_IS_GNU_C)

# options
IF(VTKOGGTHEORA_COMPILER_SUPPORTS_NAMED_ASM_PARAMS)
  OPTION(VTKOGGTHEORA_DISABLE_ASM "Disable assemby optimizations" OFF)
ENDIF(VTKOGGTHEORA_COMPILER_SUPPORTS_NAMED_ASM_PARAMS)
OPTION(VTKOGGTHEORA_DISABLE_FLOAT "Disable the use of floating point code in theora" OFF)
MARK_AS_ADVANCED(VTKOGGTHEORA_DISABLE_ASM VTKOGGTHEORA_DISABLE_FLOAT)

# we have special code for x86 and x86_64
SET(OC_X86_ASM FALSE)
SET(OC_X86_64_ASM FALSE)
IF(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]|x86_64")
  IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(OC_X86_ASM TRUE)
  ELSE(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(OC_X86_64_ASM TRUE)
  ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
ENDIF(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]|x86_64")

# set up compiler flags (only for gcc)
IF(CMAKE_COMPILER_IS_GNU_C)
  SET(VTKOGGTHEORA_C_FLAGS_RELEASE
    "-fast -fforce-addr -fomit-frame-pointer -finline-functions -funroll-loops"
    CACHE STRING "Additional compiler flags for the vtkoggtheora library (release build)")
  MARK_AS_ADVANCED(VTKOGGTHEORA_C_FLAGS_RELEASE)
  SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${VTKOGGTHEORA_C_FLAGS_RELEASE}")
ENDIF(CMAKE_COMPILER_IS_GNU_C)

# set up linker flags
IF(APPLE)
  # Note: as an optimization, oggtheora also passes '-read_only_relocs suppress' on i386 only;
  # it is omitted here because including it unconditionally causes a warning on x86_64,
  # where the flag cannot be used.
  SET( VTKOGGTHEORA_SHARED_LINKER_FLAGS
    "-Wl,-exported_symbols_list,\"${VTKOGGTHEORA_SOURCE_DIR}/vtkoggtheora.exp\""
    )
ENDIF(APPLE)
# assume that -Wl,--version-script works on UNIX systems (excluding Mac, Cygwin and Solaris SPARC).
# otherwise the user can always set to an empty string
IF(UNIX AND NOT APPLE AND NOT CYGWIN)
  IF("${CMAKE_SYSTEM_NAME}" MATCHES "SunOS")
    SET( VTKOGGTHEORA_SHARED_LINKER_FLAGS "" )
  ELSE("${CMAKE_SYSTEM_NAME}" MATCHES "SunOS")
    SET( VTKOGGTHEORA_SHARED_LINKER_FLAGS
      "-Wl,--version-script=\"${VTKOGGTHEORA_SOURCE_DIR}/vtkoggtheora.vscript\""
    )
  ENDIF("${CMAKE_SYSTEM_NAME}" MATCHES "SunOS")
ENDIF(UNIX AND NOT APPLE AND NOT CYGWIN)
SET(VTKOGGTHEORA_SHARED_LINKER_FLAGS "${VTKOGGTHEORA_SHARED_LINKER_FLAGS}"
  CACHE STRING "Additional linker flags for vtkoggtheora when building as a shared library"
  )
MARK_AS_ADVANCED(VTKOGGTHEORA_SHARED_LINKER_FLAGS)
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${VTKOGGTHEORA_SHARED_LINKER_FLAGS}")

SET(USE_ASM FALSE)
IF(NOT VTKOGGTHEORA_DISABLE_ASM)
  IF(OC_X86_ASM OR OC_X86_64_ASM OR APPLE)
    SET(USE_ASM TRUE)
  ENDIF(OC_X86_ASM OR OC_X86_64_ASM OR APPLE)
ENDIF(NOT VTKOGGTHEORA_DISABLE_ASM)

CONFIGURE_FILE(libogg-${OGG_VERSION}/include/ogg/config_types.h.in
  ${VTKOGGTHEORA_BINARY_DIR}/include/ogg/config_types.h @ONLY)

CONFIGURE_FILE(libtheora-${THEORA_VERSION}/config.h.in
  ${VTKOGGTHEORA_BINARY_DIR}/include/theora/config.h @ONLY)

CONFIGURE_FILE(libogg-${OGG_VERSION}/include/ogg/ogg.h
  ${VTKOGGTHEORA_BINARY_DIR}/include/ogg/ogg.h COPYONLY)
CONFIGURE_FILE(libogg-${OGG_VERSION}/include/ogg/os_types.h
  ${VTKOGGTHEORA_BINARY_DIR}/include/ogg/os_types.h COPYONLY)
CONFIGURE_FILE(libtheora-${THEORA_VERSION}/include/theora/codec.h
  ${VTKOGGTHEORA_BINARY_DIR}/include/theora/codec.h COPYONLY)
CONFIGURE_FILE(libtheora-${THEORA_VERSION}/include/theora/theora.h
  ${VTKOGGTHEORA_BINARY_DIR}/include/theora/theora.h COPYONLY)
CONFIGURE_FILE(libtheora-${THEORA_VERSION}/include/theora/theoradec.h
  ${VTKOGGTHEORA_BINARY_DIR}/include/theora/theoradec.h COPYONLY)
CONFIGURE_FILE(libtheora-${THEORA_VERSION}/include/theora/theoraenc.h
  ${VTKOGGTHEORA_BINARY_DIR}/include/theora/theoraenc.h COPYONLY)

CONFIGURE_FILE(vtk_oggtheora_mangle.h
  ${VTKOGGTHEORA_BINARY_DIR}/include/vtk_oggtheora_mangle.h COPYONLY)

SET(VTKOGGTHEORA_SRCS
  # ogg sources
  libogg-${OGG_VERSION}/src/framing.c libogg-${OGG_VERSION}/src/bitwise.c
  # theora sources (general)
  libtheora-${THEORA_VERSION}/lib/analyze.c
  libtheora-${THEORA_VERSION}/lib/apiwrapper.c
  libtheora-${THEORA_VERSION}/lib/bitpack.c
  libtheora-${THEORA_VERSION}/lib/decapiwrapper.c
  libtheora-${THEORA_VERSION}/lib/decinfo.c
  libtheora-${THEORA_VERSION}/lib/decode.c
  libtheora-${THEORA_VERSION}/lib/dequant.c
  libtheora-${THEORA_VERSION}/lib/encapiwrapper.c
  libtheora-${THEORA_VERSION}/lib/encfrag.c
  libtheora-${THEORA_VERSION}/lib/encinfo.c
  libtheora-${THEORA_VERSION}/lib/encode.c
  libtheora-${THEORA_VERSION}/lib/enquant.c
  libtheora-${THEORA_VERSION}/lib/fdct.c
  libtheora-${THEORA_VERSION}/lib/fragment.c
  libtheora-${THEORA_VERSION}/lib/huffdec.c
  libtheora-${THEORA_VERSION}/lib/huffenc.c
  libtheora-${THEORA_VERSION}/lib/idct.c
  libtheora-${THEORA_VERSION}/lib/info.c
  libtheora-${THEORA_VERSION}/lib/internal.c
  libtheora-${THEORA_VERSION}/lib/mathops.c
  libtheora-${THEORA_VERSION}/lib/mcenc.c
  libtheora-${THEORA_VERSION}/lib/quant.c
  libtheora-${THEORA_VERSION}/lib/rate.c
  libtheora-${THEORA_VERSION}/lib/state.c
  libtheora-${THEORA_VERSION}/lib/tokenize.c
  )
IF(OC_X86_ASM OR OC_X86_64_ASM)
  SET(VTKOGGTHEORA_SRCS
    ${VTKOGGTHEORA_SRCS}
    # x86/x86_64 shared sources
    libtheora-${THEORA_VERSION}/lib/x86/mmxencfrag.c
    libtheora-${THEORA_VERSION}/lib/x86/mmxfdct.c
    libtheora-${THEORA_VERSION}/lib/x86/mmxfrag.c
    libtheora-${THEORA_VERSION}/lib/x86/mmxidct.c
    libtheora-${THEORA_VERSION}/lib/x86/mmxstate.c
    libtheora-${THEORA_VERSION}/lib/x86/x86enc.c
    libtheora-${THEORA_VERSION}/lib/x86/x86state.c
    )
  IF(MSVC)
    SET(VTKOGGTHEORA_SRCS
      ${VTKOGGTHEORA_SRCS}
      # MSVC specific x86 sources
      libtheora-${THEORA_VERSION}/lib/x86_vc/mmxencfrag.c
      libtheora-${THEORA_VERSION}/lib/x86_vc/mmxfdct.c
      libtheora-${THEORA_VERSION}/lib/x86_vc/mmxfrag.c
      libtheora-${THEORA_VERSION}/lib/x86_vc/mmxidct.c
      libtheora-${THEORA_VERSION}/lib/x86_vc/mmxstate.c
      libtheora-${THEORA_VERSION}/lib/x86_vc/x86enc.c
      libtheora-${THEORA_VERSION}/lib/x86_vc/x86state.c
      )
  ENDIF(MSVC)
ENDIF(OC_X86_ASM OR OC_X86_64_ASM)
IF(OC_X86_ASM)
  SET(VTKOGGTHEORA_SRCS
    ${VTKOGGTHEORA_SRCS}
    # x86_64 encoder sources
    libtheora-${THEORA_VERSION}/lib/x86/sse2fdct.c
    )
ENDIF(OC_X86_ASM)

IF(MSVC)
  IF(BUILD_SHARED_LIBS)
    LIST(APPEND VTKOGGTHEORA_SRCS vtkoggtheora.def vtkoggtheora.rc)
  ELSE(BUILD_SHARED_LIBS)
    LIST(REMOVE_ITEM VTKOGGTHEORA_SRCS vtkoggtheora.def vtkoggtheora.rc)
  ENDIF(BUILD_SHARED_LIBS)
ENDIF(MSVC)

ADD_DEFINITIONS(-DHAVE_CONFIG_H)

VTK_ADD_LIBRARY(vtkoggtheora ${VTKOGGTHEORA_SRCS})

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${VTKOGGTHEORA_SOURCE_DIR}/vtk_oggtheora_mangle.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/vtkoggtheora/include
    COMPONENT Development)
  INSTALL(FILES
    ${VTKOGGTHEORA_SOURCE_DIR}/libogg-${OGG_VERSION}/include/ogg/ogg.h
    ${VTKOGGTHEORA_SOURCE_DIR}/libogg-${OGG_VERSION}/include/ogg/os_types.h
    ${VTKOGGTHEORA_BINARY_DIR}/include/ogg/config_types.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/vtkoggtheora/include/ogg
    COMPONENT Development)
  INSTALL(FILES
    ${VTKOGGTHEORA_SOURCE_DIR}/libtheora-${THEORA_VERSION}/include/theora/codec.h
    ${VTKOGGTHEORA_SOURCE_DIR}/libtheora-${THEORA_VERSION}/include/theora/theora.h
    ${VTKOGGTHEORA_SOURCE_DIR}/libtheora-${THEORA_VERSION}/include/theora/theoradec.h
    ${VTKOGGTHEORA_SOURCE_DIR}/libtheora-${THEORA_VERSION}/include/theora/theoraenc.h
    ${VTKOGGTHEORA_SOURCE_DIR}/libtheora-${THEORA_VERSION}/include/theora/theoraenc.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/vtkoggtheora/include/theora
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
