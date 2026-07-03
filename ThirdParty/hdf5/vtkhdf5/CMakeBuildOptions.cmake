# Put all top-level build options into one place
# This file will be included at the beginning of the root CMakeLists.txt
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_USE_FOLDERS "Enable folder grouping of projects in IDEs." ON)
mark_as_advanced (HDF5_USE_FOLDERS)

option (HDF5_NO_PACKAGES "CPACK - Disable packaging" OFF)
mark_as_advanced (HDF5_NO_PACKAGES)
option (HDF5_ALLOW_UNSUPPORTED "Allow unsupported combinations of configure options" OFF)
mark_as_advanced (HDF5_ALLOW_UNSUPPORTED)

option (HDF5_ONLY_SHARED_LIBS "Only Build Shared Libraries" OFF)
mark_as_advanced (HDF5_ONLY_SHARED_LIBS)
option (BUILD_STATIC_LIBS "Build Static Libraries" ON)
option (BUILD_SHARED_LIBS "Build Shared Libraries" ON)

option (HDF5_BUILD_STATIC_TOOLS "Build Static Tools NOT Shared Tools" OFF)
mark_as_advanced (HDF5_BUILD_STATIC_TOOLS)

option (BUILD_STATIC_EXECS "Build Static Executables" OFF)
mark_as_advanced (BUILD_STATIC_EXECS)

option (HDF5_ENABLE_ANALYZER_TOOLS "enable the use of Clang tools" OFF)
mark_as_advanced (HDF5_ENABLE_ANALYZER_TOOLS)
option (HDF5_ENABLE_SANITIZERS "execute the Clang sanitizer" OFF)
mark_as_advanced (HDF5_ENABLE_SANITIZERS)
option (HDF5_ENABLE_FORMATTERS "format source files" OFF)
mark_as_advanced (HDF5_ENABLE_FORMATTERS)

option (HDF5_ENABLE_COVERAGE "Enable code coverage for Libraries and Programs" OFF)
mark_as_advanced (HDF5_ENABLE_COVERAGE)

option (HDF5_ENABLE_USING_MEMCHECKER "Indicate that a memory checker is used" OFF)
mark_as_advanced (HDF5_ENABLE_USING_MEMCHECKER)

option (HDF5_ENABLE_PREADWRITE "Use pread/pwrite in sec2/log/core VFDs in place of read/write (when available)" ON)
mark_as_advanced (HDF5_ENABLE_PREADWRITE)

option (HDF5_ENABLE_DEPRECATED_SYMBOLS "Enable deprecated public API symbols" ON)

option (HDF5_MINGW_STATIC_GCC_LIBS "Statically link libgcc/libstdc++" OFF)
mark_as_advanced (HDF5_MINGW_STATIC_GCC_LIBS)

option (HDF5_ENABLE_TRACE "Enable API tracing capability" OFF)
mark_as_advanced (HDF5_ENABLE_TRACE)

option (HDF5_ENABLE_EMBEDDED_LIBINFO "Embed library info into executables" ON)
mark_as_advanced (HDF5_ENABLE_EMBEDDED_LIBINFO)

option (HDF5_ENABLE_HDFS "Enable HDFS" OFF)

option (HDF5_ENABLE_PARALLEL "Enable parallel build (requires MPI)" OFF)

option (HDF5_ENABLE_SZIP_SUPPORT "Use SZip Filter" OFF)
option (HDF5_ENABLE_ZLIB_SUPPORT "Enable Zlib Filters" OFF)

option (HDF5_PACKAGE_EXTLIBS "CPACK - include external libraries" OFF)
mark_as_advanced (HDF5_PACKAGE_EXTLIBS)

option (HDF5_ENABLE_THREADSAFE "Enable thread-safety" OFF)

option (HDF5_ENABLE_CONCURRENCY "Enable multi-threaded concurrency" OFF)

option (HDF5_ENABLE_MAP_API "Build the map API" OFF)
mark_as_advanced (HDF5_ENABLE_MAP_API)

option (HDF5_BUILD_DOC "Build documentation" OFF)

option (HDF5_BUILD_PARALLEL_TOOLS "Build Parallel HDF5 Tools" OFF)
mark_as_advanced (HDF5_BUILD_PARALLEL_TOOLS)

option (HDF5_BUILD_TOOLS "Build HDF5 Tools" ON)

option (HDF5_ENABLE_PLUGIN_SUPPORT "Enable PLUGIN Filters" OFF)

option (HDF5_BUILD_HL_LIB "Build HIGH Level HDF5 Library" ON)

option (HDF5_BUILD_FORTRAN "Build FORTRAN support" OFF)

option (HDF5_BUILD_CPP_LIB "Build HDF5 C++ Library" OFF)

option (HDF5_BUILD_JAVA "Build Java HDF5 Library" OFF)
cmake_dependent_option (HDF5_ENABLE_JNI "Force JNI implementation instead of FFM for Java bindings when Java 25+ is available" ON "HDF5_BUILD_JAVA" OFF)
mark_as_advanced (HDF5_ENABLE_JNI)
cmake_dependent_option (HDF5_ENABLE_MAVEN_DEPLOY "Enable Maven repository deployment support" OFF "HDF5_BUILD_JAVA" OFF)
mark_as_advanced (HDF5_ENABLE_MAVEN_DEPLOY)
cmake_dependent_option (HDF5_MAVEN_SNAPSHOT "Build Maven snapshot versions with -SNAPSHOT suffix" OFF "HDF5_BUILD_JAVA" OFF)
mark_as_advanced (HDF5_MAVEN_SNAPSHOT)

option (HDF5_BUILD_EXAMPLES "Build HDF5 Library Examples" ON)

option (BUILD_TESTING "Build HDF5 Unit Testing" ON)

else()
set(HDF5_USE_FOLDERS OFF)
set(HDF5_NO_PACKAGES ON)
set(HDF5_NO_PACKAGES OFF)
set(HDF5_ONLY_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(HDF5_BUILD_STATIC_TOOLS OFF)
set(BUILD_STATIC_EXECS OFF)
set(HDF5_ENABLE_ANALYZER_TOOLS OFF)
set(HDF5_ENABLE_SANITIZERS OFF)
set(HDF5_ENABLE_FORMATTERS OFF)
set(HDF5_ENABLE_COVERAGE OFF)
set(HDF5_ENABLE_USING_MEMCHECKER OFF)
set(HDF5_ENABLE_PREADWRITE OFF)
set(HDF5_ENABLE_DEPRECATED_SYMBOLS ON)
set(HDF5_MINGW_STATIC_GCC_LIBS OFF)
set(HDF5_ENABLE_TRACE OFF)
set(HDF5_ENABLE_EMBEDDED_LIBINFO 1)
set(HDF5_ENABLE_HDFS OFF)
set(HDF5_ENABLE_PARALLEL OFF)
set(HDF5_ENABLE_SZIP_SUPPORT OFF)
set(HDF5_ENABLE_ZLIB_SUPPORT ON)
set(HDF5_PACKAGE_EXTLIBS OFF)
set(HDF5_ENABLE_THREADSAFE ${VTK_HDF5_ENABLE_THREADSAFE})
set(HDF5_ENABLE_CONCURRENCY OFF)
set (HDF5_ENABLE_MAP_API OFF)
set (HDF5_BUILD_DOC OFF)
set (HDF5_BUILD_PARALLEL_TOOLS OFF)
set (HDF5_BUILD_TOOLS OFF)
set (HDF5_ENABLE_PLUGIN_SUPPORT OFF)
set(HDF5_BUILD_HL_LIB ON)
set(HDF5_BUILD_FORTRAN OFF)
set(HDF5_BUILD_CPP_LIB OFF)
set(HDF5_BUILD_JAVA OFF)
set(HDF5_BUILD_EXAMPLES OFF)
set(BUILD_TESTING OFF)

endif()
