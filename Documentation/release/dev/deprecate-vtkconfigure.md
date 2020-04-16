# deprecate-vtkConfigure.h

The `vtkConfigure.h` header has been deprecated. Please use the following headers instead:

  - `vtkBuild.h` - `VTK_BUILD_SHARED_LIBS`
  - `vtkCompiler.h` - Compiler detection and compatibility macros.
  - `vtkDebug.h` - `VTK_DEBUG_LEAKS` and `VTK_WARN_ON_DISPATCH_FAILURE`
  - `vtkDebugRangeIterators.h` - `VTK_DEBUG_RANGE_ITERATORS` and `VTK_ALWAYS_OPTIMIZE_ARRAY_ITERATORS`
  - `vtkEndian.h` - `VTK_WORDS_BIGENDIAN`
  - `vtkFeatures.h` - `VTK_ALL_NEW_OBJECT_FACTORY` and `VTK_USE_MEMKIND`
  - `vtkLegacy.h` - `VTK_LEGACY_REMOVE`, `VTK_LEGACY_SILENT`, and `VTK_LEGACY`
  - `vtkOptions.h` - `VTK_USE_64BIT_IDS` and `VTK_USE_64BIT_TIMESTAMPS`
  - `vtkPlatform.h` - `VTK_REQUIRE_LARGE_FILE_SUPPORT` and `VTK_MAXPATH`
  - `vtkSMP.h` - `VTK_SMP_${backend}` and `VTK_SMP_BACKEND`
  - `vtkThreads.h` - `VTK_USE_PTHREADS`, `VTK_USE_WIN32_THREADS`, `VTK_MAX_THREADS`
    - Also includes `VTK_THREAD_RETURN_VALUE` and `VTK_THREAD_RETURN_TYPE`, but
      `vtkMultiThreader.h` is guaranteed to provide this.
