# Core configuration options.
if("${CMAKE_SIZEOF_VOID_P}" GREATER 4)
 set(VTK_USE_64BIT_IDS_DEFAULT ON)
else()
 set(VTK_USE_64BIT_IDS_DEFAULT OFF)
endif()

option(VTK_USE_64BIT_IDS "Build VTK with 64 bit ids"
  ${VTK_USE_64BIT_IDS_DEFAULT})

option(VTK_DEBUG_LEAKS "Build leak checking support into VTK." OFF)

# This option determines the behavior of the New methods of vtkObject derived
# classes. If this option is off then only classes specifically using the
# vtkObjectFactoryNewMacro and vtkAbstractObjectFactoryNewMacro will allow for
# overrides. If the method is on then vtkStandardNewMacro will use the
# vtkObjectFactoryNewMacro's implementation body.
option(VTK_ALL_NEW_OBJECT_FACTORY
  "Build all vtkObject derived classes with object factory new methods." OFF)

mark_as_advanced(VTK_DEBUG_LEAKS VTK_USE_64BIT_IDS VTK_ALL_NEW_OBJECT_FACTORY)

set(vtkCommonCore_EXPORT_OPTIONS
  VTK_DEBUG_LEAKS
  VTK_USE_64BIT_IDS
  VTK_ALL_NEW_OBJECT_FACTORY
  )

# Choose which multi-threaded parallelism library to use
set(VTK_SMP_IMPLEMENTATION_TYPE_DOC_STRING "Which multi-threaded parallelism implementation to use. Options are Sequential, OpenMP or TBB")

set(VTK_SMP_IMPLEMENTATION_TYPE "Sequential" CACHE STRING ${VTK_SMP_IMPLEMENTATION_TYPE_DOC_STRING})

set_property(CACHE VTK_SMP_IMPLEMENTATION_TYPE PROPERTY STRINGS Sequential OpenMP TBB)

if( NOT ("${VTK_SMP_IMPLEMENTATION_TYPE}" STREQUAL "OpenMP" OR
         "${VTK_SMP_IMPLEMENTATION_TYPE}" STREQUAL "TBB") )
  set(VTK_SMP_IMPLEMENTATION_TYPE "Sequential" CACHE STRING ${VTK_SMP_IMPLEMENTATION_TYPE_DOC_STRING} FORCE)
endif()

set(VTK_SMP_SOURCES "")
set(VTK_SMP_HEADERS "")
set(VTK_SMP_USE_DEFAULT_ATOMICS ON)

if ("${VTK_SMP_IMPLEMENTATION_TYPE}" STREQUAL "TBB")
  find_package(TBB REQUIRED)
  set(VTK_SMP_IMPLEMENTATION_LIBRARIES ${TBB_LIBRARIES})
  include_directories(${TBB_INCLUDE_DIRS})
  # This needs to be here because all modules that include vtkAtomic.h
  # need to include tbb/atomic.h
  list(APPEND vtkCommonCore_SYSTEM_INCLUDE_DIRS ${TBB_INCLUDE_DIRS})

  set(VTK_SMP_USE_DEFAULT_ATOMICS OFF)
  set(VTK_SMP_IMPLEMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SMP/TBB")
  set(VTK_SMP_SOURCES ${VTK_SMP_IMPLEMENTATION_DIR}/vtkSMPTools.cxx)
  set(VTK_SMP_HEADERS_TO_CONFIG
    vtkAtomic.h vtkSMPToolsInternal.h vtkSMPThreadLocal.h)

elseif ("${VTK_SMP_IMPLEMENTATION_TYPE}" STREQUAL "OpenMP")
  find_package(OpenMP REQUIRED)
  add_definitions(${OpenMP_CXX_FLAGS})
  set(VTK_SMP_IMPLEMENTATION_LIBRARIES "${OpenMP_CXX_LIBRARIES}")

  set(VTK_SMP_IMPLEMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SMP/OpenMP")
  set(VTK_SMP_SOURCES ${VTK_SMP_IMPLEMENTATION_DIR}/vtkSMPTools.cxx
    ${VTK_SMP_IMPLEMENTATION_DIR}/vtkSMPThreadLocalImpl.cxx)
  set(VTK_SMP_HEADERS_TO_CONFIG
    vtkSMPToolsInternal.h vtkSMPThreadLocal.h vtkSMPThreadLocalImpl.h)

  if (OpenMP_CXX_SPEC_DATE AND NOT ${OpenMP_CXX_SPEC_DATE} LESS 201107)
    set(VTK_SMP_USE_DEFAULT_ATOMICS OFF)
    list(APPEND VTK_SMP_SOURCES ${VTK_SMP_IMPLEMENTATION_DIR}/vtkAtomic.cxx)
    list(APPEND VTK_SMP_HEADERS_TO_CONFIG vtkAtomic.h)
  else()
    message(WARNING "Required OpenMP version (3.1) for atomics not detected. Using default atomics implementation.")
  endif()

elseif ("${VTK_SMP_IMPLEMENTATION_TYPE}" STREQUAL "Sequential")
  set(VTK_SMP_IMPLEMENTATION_LIBRARIES)
  set(VTK_SMP_IMPLEMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SMP/Sequential")
  set(VTK_SMP_SOURCES ${VTK_SMP_IMPLEMENTATION_DIR}/vtkSMPTools.cxx)
  set(VTK_SMP_HEADERS_TO_CONFIG vtkSMPToolsInternal.h vtkSMPThreadLocal.h)
endif()

if (${VTK_SMP_USE_DEFAULT_ATOMICS})
  set(VTK_ATOMICS_DEFAULT_IMPL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SMP/Sequential")
  list(APPEND VTK_SMP_SOURCES ${VTK_ATOMICS_DEFAULT_IMPL_DIR}/vtkAtomic.cxx)
  configure_file(${VTK_ATOMICS_DEFAULT_IMPL_DIR}/vtkAtomic.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/vtkAtomic.h COPYONLY)
  list(APPEND VTK_SMP_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/vtkAtomic.h)
endif()

foreach (HDR_FILE ${VTK_SMP_HEADERS_TO_CONFIG})
  configure_file(${VTK_SMP_IMPLEMENTATION_DIR}/${HDR_FILE}.in
    ${CMAKE_CURRENT_BINARY_DIR}/${HDR_FILE} COPYONLY)
  list(APPEND VTK_SMP_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/${HDR_FILE})
endforeach()

list(APPEND VTK_SMP_HEADERS vtkSMPTools.h vtkSMPThreadLocalObject.h)

#-------------------------------------------------------------------------------
# Generate the vtkTypeList_Create macros:
include(vtkCreateTypeListMacros)
CreateTypeListMacros(
  VTK_TYPELISTMACRO_HEADER_CONTENTS
  99
  vtkTypeList_Create
  "vtkTypeList::TypeList"
  "vtkTypeList::NullType"
)
configure_file(vtkTypeListMacros.h.in vtkTypeListMacros.h)

#-------------------------------------------------------------------------------
# And the ArrayDispatch array list header:
option(VTK_DISPATCH_AOS_ARRAYS
  "Include array-of-structs vtkDataArray subclasses in dispatcher."
  ON
)
option(VTK_DISPATCH_SOA_ARRAYS
  "Include struct-of-arrays vtkDataArray subclasses in dispatcher."
  OFF
)
option(VTK_DISPATCH_TYPED_ARRAYS
  "Include vtkTypedDataArray subclasses (e.g. old mapped arrays) in dispatcher."
  OFF
)
include(vtkCreateArrayDispatchArrayList)
vtkArrayDispatch_default_array_setup()
vtkArrayDispatch_generate_array_header(VTK_ARRAYDISPATCH_ARRAY_LIST)
configure_file(vtkArrayDispatchArrayList.h.in vtkArrayDispatchArrayList.h)

option(VTK_WARN_ON_DISPATCH_FAILURE
  "If enabled, vtkArrayDispatch will print a warning when a dispatch fails."
  OFF
)

mark_as_advanced(
  VTK_DISPATCH_AOS_ARRAYS
  VTK_DISPATCH_SOA_ARRAYS
  VTK_DISPATCH_TYPED_ARRAYS
  VTK_WARN_ON_DISPATCH_FAILURE
)

#-----------------------------------------------------------------------------

include_directories(${VTK_SOURCE_DIR}/ThirdParty/utf8/source)

SET(Module_SRCS
  vtkAbstractArray.cxx
  vtkAngularPeriodicDataArray.txx
  vtkAnimationCue.cxx
  vtkAOSDataArrayTemplate.txx
  vtkArrayCoordinates.cxx
  vtkArray.cxx
  vtkArrayExtents.cxx
  vtkArrayExtentsList.cxx
  vtkArrayIterator.cxx
  vtkArrayIteratorTemplate.txx
  vtkArrayIteratorTemplateInstantiate.cxx
  vtkArrayRange.cxx
  vtkArraySort.cxx
  vtkArrayWeights.cxx
  vtkBitArray.cxx
  vtkBitArrayIterator.cxx
  vtkBoxMuellerRandomSequence.cxx
  vtkBreakPoint.cxx
  vtkByteSwap.cxx
  vtkCallbackCommand.cxx
  vtkCharArray.cxx
  vtkCollection.cxx
  vtkCollectionIterator.cxx
  vtkCommand.cxx
  vtkCommonInformationKeyManager.cxx
  vtkConditionVariable.cxx
  vtkCriticalSection.cxx
  vtkDataArrayCollection.cxx
  vtkDataArrayCollectionIterator.cxx
  vtkDataArray.cxx
  vtkDataArraySelection.cxx
  vtkDebugLeaks.cxx
  vtkDebugLeaksManager.cxx
  vtkDoubleArray.cxx
  vtkDynamicLoader.cxx
  vtkEventForwarderCommand.cxx
  vtkFileOutputWindow.cxx
  vtkFloatArray.cxx
  vtkFloatingPointExceptions.cxx
  vtkGarbageCollector.cxx
  vtkGarbageCollectorManager.cxx
  vtkGaussianRandomSequence.cxx
  vtkIdListCollection.cxx
  vtkIdList.cxx
  vtkIdTypeArray.cxx
  vtkIndent.cxx
  vtkInformation.cxx
  vtkInformationDataObjectKey.cxx
  vtkInformationDoubleKey.cxx
  vtkInformationDoubleVectorKey.cxx
  vtkInformationIdTypeKey.cxx
  vtkInformationInformationKey.cxx
  vtkInformationInformationVectorKey.cxx
  vtkInformationIntegerKey.cxx
  vtkInformationIntegerPointerKey.cxx
  vtkInformationIntegerVectorKey.cxx
  vtkInformationIterator.cxx
  vtkInformationKey.cxx
  vtkInformationKeyLookup.cxx
  vtkInformationKeyVectorKey.cxx
  vtkInformationObjectBaseKey.cxx
  vtkInformationObjectBaseVectorKey.cxx
  vtkInformationRequestKey.cxx
  vtkInformationStringKey.cxx
  vtkInformationStringVectorKey.cxx
  vtkInformationUnsignedLongKey.cxx
  vtkInformationVariantKey.cxx
  vtkInformationVariantVectorKey.cxx
  vtkInformationVector.cxx
  vtkInstantiator.cxx
  vtkIntArray.cxx
  vtkLargeInteger.cxx
  vtkLongArray.cxx
  vtkLongLongArray.cxx
  vtkLookupTable.cxx
  vtkMappedDataArray.txx
  vtkMath.cxx
  vtkMersenneTwister.cxx
  vtkMinimalStandardRandomSequence.cxx
  vtkMultiThreader.cxx
  vtkMutexLock.cxx
  vtkObjectBase.cxx
  vtkObject.cxx
  vtkObjectFactoryCollection.cxx
  vtkObjectFactory.cxx
  vtkOldStyleCallbackCommand.cxx
  vtkOStreamWrapper.cxx
  vtkOStrStreamWrapper.cxx
  vtkOutputWindow.cxx
  vtkOverrideInformationCollection.cxx
  vtkOverrideInformation.cxx
  vtkPeriodicDataArray.txx
  vtkPoints2D.cxx
  vtkPoints.cxx
  vtkPriorityQueue.cxx
  vtkRandomSequence.cxx
  vtkReferenceCount.cxx
  vtkScalarsToColors.cxx
  vtkShortArray.cxx
  vtkSignedCharArray.cxx
  vtkSimpleCriticalSection.cxx
  vtkSmartPointerBase.cxx
  vtkSOADataArrayTemplate.txx
  vtkSOADataArrayTemplateInstantiate.cxx
  vtkSortDataArray.cxx
  vtkStdString.cxx
  vtkStringArray.cxx
  vtkStringOutputWindow.cxx
  vtkTimePointUtility.cxx
  vtkTimeStamp.cxx
  vtkTypedDataArray.txx
  vtkUnicodeStringArray.cxx
  vtkUnicodeString.cxx
  vtkUnsignedCharArray.cxx
  vtkUnsignedIntArray.cxx
  vtkUnsignedLongArray.cxx
  vtkUnsignedLongLongArray.cxx
  vtkUnsignedShortArray.cxx
  vtkVariantArray.cxx
  vtkVariant.cxx
  vtkVersion.cxx
  vtkVoidArray.cxx
  vtkWeakPointerBase.cxx
  vtkWindow.cxx
  vtkXMLFileOutputWindow.cxx
  vtkDenseArray.txx
  vtkSparseArray.txx
  vtkTypedArray.txx
  vtkTypeTemplate.h
  vtkType.h
  vtkSystemIncludes.h
  vtkWrappingHints.h
  ${VTK_SMP_SOURCES}
  ${VTK_SMP_HEADERS}
  ${CMAKE_CURRENT_BINARY_DIR}/vtkConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkMathConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkVersionMacros.h
  )

set(${vtk-module}_HDRS
  vtkABI.h
  vtkAngularPeriodicDataArray.h
  vtkArrayDispatch.h
  vtkArrayDispatch.txx
  vtkArrayInterpolate.h
  vtkArrayInterpolate.txx
  vtkArrayIteratorIncludes.h
  vtkArrayIteratorTemplateImplicit.txx
  vtkArrayPrint.h
  vtkArrayPrint.txx
  vtkAssume.h
  vtkAtomicTypeConcepts.h
  vtkAtomicTypes.h
  vtkAutoInit.h
  vtkBuffer.h
  vtkDataArrayAccessor.h
  vtkDataArrayIteratorMacro.h
  vtkDataArrayTemplate.h
  vtkGenericDataArray.h
  vtkGenericDataArrayLookupHelper.h
  vtkGenericDataArray.txx
  vtkIOStream.h
  vtkIOStreamFwd.h
  vtkInformationInternals.h
  vtkMappedDataArray.h
  vtkMathUtilities.h
  vtkMersenneTwister.h
  vtkNew.h
  vtkPeriodicDataArray.h
  vtkSetGet.h
  vtkSmartPointer.h
  vtkSOADataArrayTemplate.h
  vtkSOADataArrayTemplate.txx
  vtkTemplateAliasMacro.h
  vtkTestDataArray.h
  vtkTypeList.h
  vtkTypeList.txx
  vtkTypeTraits.h
  vtkTypedDataArray.h
  vtkTypedDataArrayIterator.h
  vtkVariantCast.h
  vtkVariantCreate.h
  vtkVariantExtract.h
  vtkVariantInlineOperators.h
  vtkWeakPointer.h
  vtkWin32Header.h
  vtkWindows.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkArrayDispatchArrayList.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkToolkits.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTypeListMacros.h
  )

if(vtkCommonDataModel_ENABLED)
  include_directories(${VTK_BINARY_DIR}/Common/DataModel)
  set_property(SOURCE vtkInformationDataObjectKey.cxx
    PROPERTY COMPILE_DEFINITIONS vtkCommonDataModel_ENABLED)
endif()

if(WIN32)
  # TODO: VTK_USE_X was once tested here but is now in another module
  list(APPEND Module_SRCS
    vtkWin32OutputWindow.cxx
    vtkWin32OutputWindowProcessEncoded.c
    vtkWin32ProcessOutputWindow.cxx
    )
  set_property(SOURCE vtkWin32OutputWindowProcessEncoded.c PROPERTY WRAP_EXCLUDE 1)
endif()

if(ANDROID)
  list(APPEND Module_SRCS
    vtkAndroidOutputWindow.cxx
    )
endif()

# Generate data for folding Unicode strings
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkUnicodeString.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkUnicodeString.cmake @ONLY)

# Setup clean configuration of vtkConfigure.h and vtkToolkits.h.
macro(VTK_PREPARE_CMAKEDEFINE not invar outvar)
  if(${not} ${invar})
    set(${outvar} 1)
  else()
    set(${outvar})
  endif()
endmacro()

vtk_prepare_cmakedefine("" CMAKE_WORDS_BIGENDIAN VTK_WORDS_BIGENDIAN)

# Provide version macros through vtkConfigure.h for compatibility.
if(VTK_LEGACY_REMOVE)
  set(VTK_CONFIG_LEGACY_VERSION "")
else()
  set(VTK_CONFIG_LEGACY_VERSION "#include \"vtkVersionMacros.h\" // removed by VTK_LEGACY_REMOVE")
endif()

# Check for atomic functions
include(CheckSymbolExists)
if (WIN32)
  check_symbol_exists(InterlockedAdd "windows.h" VTK_HAS_INTERLOCKEDADD)
endif()

configure_file(vtkVersionMacros.h.in vtkVersionMacros.h @ONLY)
configure_file(vtkConfigure.h.in vtkConfigure.h @ONLY)
configure_file(vtkToolkits.h.in vtkToolkits.h @ONLY)

add_custom_command(
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/CaseFolding.txt
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vtkUnicodeString.cmake
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/vtkUnicodeString.cmake
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkUnicodeCaseFoldData.h
  )

set_source_files_properties(
  vtkUnicodeString.cxx
  OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vtkUnicodeCaseFoldData.h
  )

# Configure data arrays for platform-independent fixed-size types.
# Match the type selection here to that in vtkType.h.
set(VTK_TYPE_NATIVE_Int8 "
#if VTK_TYPE_INT8 == VTK_CHAR
# include \"vtkCharArray.h\"
# define vtkTypeArrayBase vtkCharArray
#else
# include \"vtkSignedCharArray.h\"
# define vtkTypeArrayBase vtkSignedCharArray
#endif
")
set(VTK_TYPE_NATIVE_UInt8 "
#if VTK_TYPE_UINT8 == VTK_UNSIGNED_CHAR
# include \"vtkUnsignedCharArray.h\"
# define vtkTypeArrayBase vtkUnsignedCharArray
#endif
")
set(VTK_TYPE_NATIVE_Int16 "
#if VTK_TYPE_INT16 == VTK_SHORT
# include \"vtkShortArray.h\"
# define vtkTypeArrayBase vtkShortArray
#endif
")
set(VTK_TYPE_NATIVE_UInt16 "
#if VTK_TYPE_UINT16 == VTK_UNSIGNED_SHORT
# include \"vtkUnsignedShortArray.h\"
# define vtkTypeArrayBase vtkUnsignedShortArray
#endif
")
set(VTK_TYPE_NATIVE_Int32 "
#if VTK_TYPE_INT32 == VTK_INT
# include \"vtkIntArray.h\"
# define vtkTypeArrayBase vtkIntArray
#endif
")
set(VTK_TYPE_NATIVE_UInt32 "
#if VTK_TYPE_UINT32 == VTK_UNSIGNED_INT
# include \"vtkUnsignedIntArray.h\"
# define vtkTypeArrayBase vtkUnsignedIntArray
#endif
")
set(VTK_TYPE_NATIVE_Int64 "
#if VTK_TYPE_INT64 == VTK_LONG
# include \"vtkLongArray.h\"
# define vtkTypeArrayBase vtkLongArray
#elif VTK_TYPE_INT64 == VTK_LONG_LONG
# include \"vtkLongLongArray.h\"
# define vtkTypeArrayBase vtkLongLongArray
#endif
")
set(VTK_TYPE_NATIVE_UInt64 "
#if VTK_TYPE_UINT64 == VTK_UNSIGNED_LONG
# include \"vtkUnsignedLongArray.h\"
# define vtkTypeArrayBase vtkUnsignedLongArray
#elif VTK_TYPE_UINT64 == VTK_UNSIGNED_LONG_LONG
# include \"vtkUnsignedLongLongArray.h\"
# define vtkTypeArrayBase vtkUnsignedLongLongArray
#endif
")
set(VTK_TYPE_NATIVE_Float32 "
#if VTK_TYPE_FLOAT32 == VTK_FLOAT
# include \"vtkFloatArray.h\"
# define vtkTypeArrayBase vtkFloatArray
#endif
")
set(VTK_TYPE_NATIVE_Float64 "
#if VTK_TYPE_FLOAT64 == VTK_DOUBLE
# include \"vtkDoubleArray.h\"
# define vtkTypeArrayBase vtkDoubleArray
#endif
")

foreach(t Int8 Int16 Int32 Int64 UInt8 UInt16 UInt32 UInt64 Float32 Float64)
  set(VTK_TYPE_NAME ${t})
  set(VTK_TYPE_NATIVE "${VTK_TYPE_NATIVE_${t}}")
  if(VTK_TYPE_NATIVE)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkTypedArray.h.in
                   ${CMAKE_CURRENT_BINARY_DIR}/vtkType${t}Array.h
                   @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkTypedArray.cxx.in
                   ${CMAKE_CURRENT_BINARY_DIR}/vtkType${t}Array.cxx
                   @ONLY)
    set(Module_SRCS ${Module_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/vtkType${t}Array.cxx)
  endif()
endforeach()

# look for various headers and functions
include(CheckCXXExpressionCompiles)
include(CheckIncludeFile)

# Check C++ <cmath> first, where the C++11 standard says these must be.
check_cxx_expression_compiles("std::isnan(0.0)" "cmath" VTK_HAS_STD_ISNAN)
check_cxx_expression_compiles("std::isinf(0.0)" "cmath" VTK_HAS_STD_ISINF)
check_cxx_expression_compiles("std::isfinite(0.0)" "cmath" VTK_HAS_STD_ISFINITE)

# Check C99 <math.h> next, where the C99 standard says these must be.
# (they will be found even if they are defined as macros)
check_symbol_exists(isnan "math.h" VTK_HAS_ISNAN)
check_symbol_exists(isinf "math.h" VTK_HAS_ISINF)
check_symbol_exists(isfinite "math.h" VTK_HAS_ISFINITE)

# Check for finite() which is widespread, though non-standard.
check_symbol_exists(finite "math.h" VTK_HAS_FINITE)

# Check float.h last, where Windows keeps its underscore versions.
if(NOT VTK_HAS_ISNAN AND NOT VTK_HAS_STD_ISNAN)
  check_symbol_exists(_isnan "float.h" VTK_HAS__ISNAN)
endif()
if(NOT VTK_HAS_FINITE AND NOT VTK_HAS_ISFINITE AND NOT VTK_HAS_STD_ISFINITE)
  check_symbol_exists(_finite "float.h" VTK_HAS__FINITE)
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkMathConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkMathConfigure.h
  )

# Figure out if feenableexcept and fedisableexcept functions are present
check_include_file("fenv.h" HAVE_FENV_H)
if(HAVE_FENV_H)
  set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
  if(UNIX)
    set(CMAKE_REQUIRED_LIBRARIES m)
  endif()
  check_symbol_exists(feenableexcept "fenv.h" VTK_HAS_FEENABLEEXCEPT)
  set(CMAKE_REQUIRED_LIBRARIES)
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkFloatingPointExceptionsConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkFloatingPointExceptionsConfigure.h
  )

set_source_files_properties(
  # vtkObjectBase is marked ABSTRACT so that instantiators are happy.
  vtkAbstractArray
  vtkArray
  vtkArrayIterator
  vtkCallbackCommand
  vtkCommand
  vtkCommonInformationKeyManager
  vtkDataArray
  vtkEventForwarderCommand
  vtkFloatingPointExceptions
  vtkFunctionSet
  vtkGaussianRandomSequence
  vtkInformationDataObjectKey
  vtkInformationDoubleKey
  vtkInformationDoubleVectorKey
  vtkInformationIdTypeKey
  vtkInformationInformationKey
  vtkInformationInformationVectorKey
  vtkInformationIntegerKey
  vtkInformationIntegerPointerKey
  vtkInformationIntegerVectorKey
  vtkInformationKey
  vtkInformationKeyVectorKey
  vtkInformationObjectBaseKey
  vtkInformationObjectBaseVectorKey
  vtkInformationQuadratureSchemeDefinitionVectorKey
  vtkInformationRequestKey
  vtkInformationStringKey
  vtkInformationStringVectorKey
  vtkInformationUnsignedLongKey
  vtkInformationVariantKey
  vtkInformationVariantVectorKey
  vtkObjectBase
  vtkObjectFactory
  vtkOldStyleCallbackCommand
  vtkRandomSequence
  vtkWindow
  ABSTRACT
  )

set_source_files_properties(
  vtkAOSDataArrayTemplate.h
  vtkAOSDataArrayTemplate.txx
  vtkArrayCoordinates.cxx
  vtkArrayExtents.cxx
  vtkArrayExtentsList.cxx
  vtkArrayIteratorTemplate.txx
  vtkArrayIteratorTemplateInstantiate.cxx
  vtkArrayRange.cxx
  vtkArraySort.cxx
  vtkArrayWeights.cxx
  vtkBuffer.h
  vtkBoundingBox.cxx
  vtkBreakPoint.cxx
  vtkCallbackCommand.cxx
  vtkCommonInformationKeyManager.cxx
  vtkDebugLeaksManager.cxx
  vtkEventForwarderCommand.cxx
  vtkFloatingPointExceptions
  vtkGarbageCollectorManager.cxx
  vtkIndent.cxx
  vtkLargeInteger.cxx
  vtkMersenneTwister_Private.cxx
  vtkOStrStreamWrapper.cxx
  vtkOStreamWrapper.cxx
  vtkOldStyleCallbackCommand.cxx
  vtkSimpleCriticalSection.cxx
  vtkSmartPointerBase.cxx
  vtkSOADataArrayTemplate.h
  vtkSOADataArrayTemplate.txx
  vtkSOADataArrayTemplateInstantiate.cxx # Has no API
  vtkStdString.cxx
  vtkTimeStamp.cxx
  vtkVariant.cxx
  vtkWeakPointerBase.cxx
  vtkUnicodeString.cxx
  vtkDataArrayPrivate.txx

  vtkABI.h
  vtkAngularPeriodicDataArray.txx
  vtkArrayInterpolate.h
  vtkArrayInterpolate.txx
  vtkArrayIteratorIncludes.h
  vtkArrayIteratorTemplate.h
  vtkArrayIteratorTemplateImplicit.txx
  vtkArrayPrint.h
  vtkArrayPrint.txx
  vtkAtomicTypeConcepts.h
  vtkAtomicTypes.h
  vtkAutoInit.h
  vtkDenseArray.txx
  vtkGenericDataArrayHelpers.h
  vtkInformationInternals.h
  vtkIOStream.h
  vtkIOStreamFwd.h
  vtkIOStreamFwd.h
  vtkMathUtilities.h
  vtkMappedDataArray.txx
  vtkNew.h
  vtkPeriodicDataArray.txx
  vtkSetGet.h
  vtkSmartPointer.h
  vtkSparseArray.txx
  vtkTemplateAliasMacro.h
  vtkTypeTraits.h
  vtkTypedArray.txx
  vtkTypedDataArray.txx
  vtkVariantCast.h
  vtkVariantCreate.h
  vtkVariantExtract.h
  vtkVariantInlineOperators.h
  vtkWeakPointer.h
  vtkWin32Header.h
  vtkWindows.h
  vtkToolkits.h
  vtkSystemIncludes.h
  vtkType.h
  vtkConfigure.h
  vtkMathConfigure.h
  vtkVersionMacros.h
  vtkTypeTemplate.h
  vtkWrappingHints.h
  ${VTK_SMP_SOURCES}
  ${VTK_SMP_HEADERS}
  "${VTK_TYPELISTMACROS_HEADER}"

  WRAP_EXCLUDE
  )

# python uses WRAP_EXCLUDE_PYTHON and ignores WRAP_EXCLUDE
set_source_files_properties(
  vtkABI.h
  vtkAOSDataArrayTemplate.txx
  vtkArrayIteratorTemplateInstantiate.cxx
  vtkAtomic.h
  vtkAutoInit.h
  vtkIOStream.cxx
  vtkIOStreamFwd.h
  vtkLargeInteger.cxx
  vtkOldStyleCallbackCommand.cxx
  vtkOStreamWrapper.cxx
  vtkOStrStreamWrapper.cxx
  vtkNew.h
  vtkSetGet.h
  vtkSmartPointer.h
  vtkSmartPointerBase.h
  vtkSOADataArrayTemplate.txx
  vtkSOADataArrayTemplateInstantiate.cxx # Has no API
  vtkStdString.cxx
  vtkUnicodeString.cxx
  vtkWeakPointer.h
  vtkWeakPointerBase.cxx
  vtkWin32Header.h
  vtkWrappingHints.h
  PROPERTIES WRAP_EXCLUDE_PYTHON 1
)

set_source_files_properties(
  vtkDataArrayTemplate.h
  vtkDenseArray.h
  vtkSparseArray.h
  vtkTypedArray.h
  vtkTypeTemplate.h
  "${VTK_TYPELISTMACROS_HEADER}"
  HEADER_FILE_ONLY
  )

set_source_files_properties(
  vtkArrayIteratorTemplateInstantiate.cxx # Has no header
  vtkSOADataArrayTemplateInstantiate.cxx # Has no header
  PROPERTIES SKIP_HEADER_INSTALL 1
)

# Need nsl to resolve gethostbyname on SunOS-5.8
# and socket also
if(CMAKE_SYSTEM MATCHES "SunOS.*")
  set(KIT_LIBS ${KIT_LIBS} socket nsl)
endif()

vtk_module_library(vtkCommonCore ${Module_SRCS})

vtk_module_link_libraries(vtkCommonCore LINK_PRIVATE ${CMAKE_THREAD_LIBS} LINK_PUBLIC ${VTK_SMP_IMPLEMENTATION_LIBRARIES})

if(ANDROID)
  target_link_libraries(vtkCommonCore LINK_PRIVATE log)
endif()
