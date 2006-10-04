#-----------------------------------------------------------------------------
# Discover the name of the runtime library path environment variable,
# which varies from platform to platform.  Since it depends on the compiler
# options (32-bit vs. 64-bit) on some platforms, a TRY_RUN is needed.

# The result is put in SHARED_LIBRARY_PATH_VAR_NAME

TRY_RUN(SHARED_LIBRARY_PATH_TYPE SHARED_LIBRARY_PATH_INFO_COMPILED
        ${PROJECT_BINARY_DIR}/CMakeTmp
        ${PROJECT_SOURCE_DIR}/CMake/SharedLibraryPathInfo.cxx
        OUTPUT_VARIABLE OUTPUT
        ARGS "LDPATH")

STRING(REGEX MATCH "([a-zA-Z][a-zA-Z0-9_]*)(\r|\n)*$" OUTPUT "${OUTPUT}")
STRING(REGEX MATCH "[a-zA-Z][a-zA-Z0-9_]*" OUTPUT "${OUTPUT}")
SET(SHARED_LIBRARY_PATH_VAR_NAME "${OUTPUT}" CACHE INTERNAL "runtime library path variable name.")

