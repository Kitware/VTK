#ifndef VISKORESDIY_MPI_EXPORT_H
#define VISKORESDIY_MPI_EXPORT_H

#if defined(_MSC_VER)
#  ifdef VISKORESDIY_MPI_STATIC_BUILD
     /* This is a static component and has no need for exports
        elf based static libraries are able to have hidden/default visibility
        controls on symbols so we should propagate this information in that
        use case
     */
#    define VISKORESDIY_MPI_EXPORT_DEFINE
#    define VISKORESDIY_MPI_IMPORT_DEFINE
#    define VISKORESDIY_MPI_NO_EXPORT_DEFINE
#  else
#    define VISKORESDIY_MPI_EXPORT_DEFINE __declspec(dllexport)
#    define VISKORESDIY_MPI_IMPORT_DEFINE __declspec(dllimport)
#    define VISKORESDIY_MPI_NO_EXPORT_DEFINE
#  endif
#else
#  define VISKORESDIY_MPI_EXPORT_DEFINE __attribute__((visibility("default")))
#  define VISKORESDIY_MPI_IMPORT_DEFINE __attribute__((visibility("default")))
#  define VISKORESDIY_MPI_NO_EXPORT_DEFINE __attribute__((visibility("hidden")))
#endif

#ifndef VISKORESDIY_MPI_EXPORT
#  if !defined(VISKORESDIY_MPI_AS_LIB)
#    define VISKORESDIY_MPI_EXPORT
#    define VISKORESDIY_MPI_EXPORT_FUNCTION inline
#  else
#    if defined(VISKORESDIY_HAS_MPI)
       /* We are building this library */
#      define VISKORESDIY_MPI_EXPORT VISKORESDIY_MPI_EXPORT_DEFINE
#    else
       /* We are using this library */
#      define VISKORESDIY_MPI_EXPORT VISKORESDIY_MPI_IMPORT_DEFINE
#    endif
#    define VISKORESDIY_MPI_EXPORT_FUNCTION VISKORESDIY_MPI_EXPORT
#  endif
#endif

#ifndef VISKORESDIY_MPI_EXPORT_FUNCTION
#error "VISKORESDIY_MPI_EXPORT_FUNCTION not defined"
#endif

#ifndef VISKORESDIY_MPI_NO_EXPORT
#  define VISKORESDIY_MPI_NO_EXPORT VISKORESDIY_MPI_NO_EXPORT_DEFINE
#endif

#endif // VISKORESDIY_MPI_EXPORT_H
