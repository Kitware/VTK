#ifndef DIY_MPI_EXPORT_H
#define DIY_MPI_EXPORT_H

#if defined(_MSC_VER)
#  ifdef DIY_MPI_STATIC_BUILD
     /* This is a static component and has no need for exports
        elf based static libraries are able to have hidden/default visibility
        controls on symbols so we should propagate this information in that
        use case
     */
#    define DIY_MPI_EXPORT_DEFINE
#    define DIY_MPI_IMPORT_DEFINE
#    define DIY_MPI_NO_EXPORT_DEFINE
#  else
#    define DIY_MPI_EXPORT_DEFINE __declspec(dllexport)
#    define DIY_MPI_IMPORT_DEFINE __declspec(dllimport)
#    define DIY_MPI_NO_EXPORT_DEFINE
#  endif
#else
#  define DIY_MPI_EXPORT_DEFINE __attribute__((visibility("default")))
#  define DIY_MPI_IMPORT_DEFINE __attribute__((visibility("default")))
#  define DIY_MPI_NO_EXPORT_DEFINE __attribute__((visibility("hidden")))
#endif

#ifndef DIY_MPI_EXPORT
#  if !defined(DIY_MPI_AS_LIB)
#    define DIY_MPI_EXPORT
#    define DIY_MPI_EXPORT_FUNCTION inline
#  else
#    if defined(DIY_HAS_MPI)
       /* We are building this library */
#      define DIY_MPI_EXPORT DIY_MPI_EXPORT_DEFINE
#    else
       /* We are using this library */
#      define DIY_MPI_EXPORT DIY_MPI_IMPORT_DEFINE
#    endif
#    define DIY_MPI_EXPORT_FUNCTION DIY_MPI_EXPORT
#  endif
#endif

#ifndef DIY_MPI_EXPORT_FUNCTION
#error "DIY_MPI_EXPORT_FUNCTION not defined"
#endif

#ifndef DIY_MPI_NO_EXPORT
#  define DIY_MPI_NO_EXPORT DIY_MPI_NO_EXPORT_DEFINE
#endif

#endif // DIY_MPI_EXPORT_H
