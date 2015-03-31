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
set(VTK_SMP_IMPLEMENTATION_TYPE "Sequential" CACHE STRING
  "Which multi-threaded parallelism implementation to use. Options are Sequential, Simple, Kaapi or TBB"
)
set_property(CACHE VTK_SMP_IMPLEMENTATION_TYPE PROPERTY STRINGS Sequential Simple Kaapi TBB)

if( NOT ("${VTK_SMP_IMPLEMENTATION_TYPE}" STREQUAL "Kaapi" OR
         "${VTK_SMP_IMPLEMENTATION_TYPE}" STREQUAL "TBB" OR
         "${VTK_SMP_IMPLEMENTATION_TYPE}" STREQUAL "Simple") )
  set(VTK_SMP_IMPLEMENTATION_TYPE "Sequential")
endif()

if ("${VTK_SMP_IMPLEMENTATION_TYPE}" STREQUAL "TBB")
  find_package(TBB REQUIRED)
  set(VTK_SMP_IMPLEMENTATION_LIBRARIES "")
  if (TBB_LIBRARY)
    list(APPEND VTK_SMP_IMPLEMENTATION_LIBRARIES optimized ${TBB_LIBRARY})
  endif()
  if (TBB_LIBRARY_DEBUG)
    list(APPEND VTK_SMP_IMPLEMENTATION_LIBRARIES debug ${TBB_LIBRARY_DEBUG})
  endif()
  set(VTK_SMP_ATOMIC_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/SMP/TBB")
  include_directories(${TBB_INCLUDE_DIRS})
  # This needs to be here because all modules that include vtkAtomic.h
  # need to include tbb/atomic.h
  list(APPEND vtkCommonCore_SYSTEM_INCLUDE_DIRS ${TBB_INCLUDE_DIRS})
elseif ("${VTK_SMP_IMPLEMENTATION_TYPE}" STREQUAL "Kaapi")
  find_package(XKaapi REQUIRED)
  include_directories(${XKAAPI_INCLUDE_DIRS})
  set(VTK_SMP_IMPLEMENTATION_LIBRARIES ${XKAAPI_LIBRARIES})
  set(VTK_SMP_ATOMIC_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/SMP/Sequential")
  list(APPEND vtkCommonCore_SYSTEM_INCLUDE_DIRS ${XKAAPI_INCLUDE_DIRS})
elseif ("${VTK_SMP_IMPLEMENTATION_TYPE}" STREQUAL "Simple")
  set(VTK_SMP_IMPLEMENTATION_LIBRARIES)
  set(VTK_SMP_ATOMIC_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/SMP/Sequential")
  message(WARNING "The Simple backend for SMP operations is an experimental backend that is mainly used for debugging currently. We recommend that you use either the TBB or the Kaapi backend for production work. Use the Sequential backend if you would like to turn off any SMP parallelism.")
elseif ("${VTK_SMP_IMPLEMENTATION_TYPE}" STREQUAL "Sequential")
  set(VTK_SMP_IMPLEMENTATION_LIBRARIES)
  set(VTK_SMP_ATOMIC_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/SMP/Sequential")
endif()

if(EXISTS ${VTK_SMP_ATOMIC_DIRECTORY}/vtkAtomicInt.cxx)
  set(VTK_ATOMIC_CXX_FILE ${VTK_SMP_ATOMIC_DIRECTORY}/vtkAtomicInt.cxx)
else()
  set(VTK_ATOMIC_CXX_FILE "")
endif()

configure_file(${VTK_SMP_ATOMIC_DIRECTORY}/vtkAtomicInt.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkAtomicInt.h COPYONLY)

configure_file(SMP/${VTK_SMP_IMPLEMENTATION_TYPE}/vtkSMPThreadLocal.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMPThreadLocal.h COPYONLY)

configure_file(SMP/${VTK_SMP_IMPLEMENTATION_TYPE}/vtkSMPToolsInternal.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMPToolsInternal.h COPYONLY)

#-----------------------------------------------------------------------------

include_directories(${VTK_SOURCE_DIR}/ThirdParty/utf8/source)

SET(Module_SRCS
  vtkAbstractArray.cxx
  vtkAnimationCue.cxx
  vtkArrayCoordinates.cxx
  vtkArray.cxx
  vtkArrayExtents.cxx
  vtkArrayExtentsList.cxx
  vtkArrayIterator.cxx
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
  vtkIOStream.cxx
  vtkLargeInteger.cxx
  vtkLongArray.cxx
  vtkLookupTable.cxx
  vtkMappedDataArray.txx
  vtkMath.cxx
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
  vtkSortDataArray.cxx
  vtkStdString.cxx
  vtkStringArray.cxx
  vtkTimePointUtility.cxx
  vtkTimeStamp.cxx
  vtkTypedDataArray.txx
  vtkUnicodeStringArray.cxx
  vtkUnicodeString.cxx
  vtkUnsignedCharArray.cxx
  vtkUnsignedIntArray.cxx
  vtkUnsignedLongArray.cxx
  vtkUnsignedShortArray.cxx
  vtkVariantArray.cxx
  vtkVariant.cxx
  vtkVersion.cxx
  vtkVoidArray.cxx
  vtkWeakPointerBase.cxx
  vtkWindow.cxx
  vtkXMLFileOutputWindow.cxx
  vtkDataArrayTemplate.txx
  vtkDataArrayTemplateHelper.cxx
  vtkDenseArray.txx
  vtkSparseArray.txx
  vtkTypedArray.txx
  vtkTypeTemplate.h
  vtkType.h
  vtkSystemIncludes.h
  ${VTK_ATOMIC_CXX_FILE}
  vtkSMPThreadLocalObject.h
  vtkSMPTools.h
  SMP/${VTK_SMP_IMPLEMENTATION_TYPE}/vtkSMPTools.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMPToolsInternal.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMPThreadLocal.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkAtomicInt.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkMathConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkVersionMacros.h
  )

set(${vtk-module}_HDRS
  vtkABI.h
  vtkArrayInterpolate.h
  vtkArrayInterpolate.txx
  vtkArrayIteratorIncludes.h
  vtkArrayIteratorTemplate.h
  vtkArrayIteratorTemplate.txx
  vtkArrayIteratorTemplateImplicit.txx
  vtkArrayPrint.h
  vtkArrayPrint.txx
  vtkAutoInit.h
  vtkDataArrayIteratorMacro.h
  vtkDataArrayTemplateImplicit.txx
  vtkIOStreamFwd.h
  vtkInformationInternals.h
  vtkMappedDataArray.h
  vtkMathUtilities.h
  vtkNew.h
  vtkSetGet.h
  vtkSmartPointer.h
  vtkTemplateAliasMacro.h
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
  ${CMAKE_CURRENT_BINARY_DIR}/vtkToolkits.h
  )

if(vtkCommonDataModel_ENABLED)
  include_directories(${VTK_BINARY_DIR}/Common/DataModel)
  set_property(SOURCE vtkInformationDataObjectKey.cxx
    PROPERTY COMPILE_DEFINITIONS vtkCommonDataModel_ENABLED)
endif()

# __int64 seems to be platform specific, turning on if
if(HAVE_VTK_SIZEOF__INT64)
  list(APPEND Module_SRCS
    vtkUnsigned__Int64Array.cxx
    vtk__Int64Array.cxx
    )
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
vtk_prepare_cmakedefine("" CMAKE_NO_ANSI_STRING_STREAM VTK_NO_ANSI_STRING_STREAM)
vtk_prepare_cmakedefine(NOT VTK_EXPLICIT_TEMPLATES
                  VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION)
vtk_prepare_cmakedefine(NOT VTK_COMPILER_HAS_FULL_SPECIALIZATION
                        VTK_NO_FULL_TEMPLATE_SPECIALIZATION)

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

if(VTK_TYPE_USE_LONG_LONG)
  list(APPEND Module_SRCS
    vtkLongLongArray.cxx
    vtkUnsignedLongLongArray.cxx
    )
endif()

if(VTK_TYPE_USE___INT64)
  list(APPEND Module_SRCS vtk__Int64Array.cxx)
  if(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    list(APPEND Module_SRCS vtkUnsigned__Int64Array.cxx)
  endif()
endif()

# Configure data arrays for platform-independent fixed-size types.
# Match the type selection here to that in vtkType.h.
if(VTK_TYPE_CHAR_IS_SIGNED)
  set(VTK_TYPE_NATIVE_Int8 Char)
else()
  set(VTK_TYPE_NATIVE_Int8 SignedChar)
endif()
set(VTK_TYPE_NATIVE_Int16 Short)
set(VTK_TYPE_NATIVE_Int32 Int)
set(VTK_TYPE_NATIVE_UInt8 UnsignedChar)
set(VTK_TYPE_NATIVE_UInt16 UnsignedShort)
set(VTK_TYPE_NATIVE_UInt32 UnsignedInt)

if(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
  set(VTK_TYPE_NATIVE_Int64 LongLong)
  set(VTK_TYPE_NATIVE_UInt64 UnsignedLongLong)
else()
  if(VTK_SIZEOF_LONG MATCHES "^8$")
    set(VTK_TYPE_NATIVE_Int64 Long)
    set(VTK_TYPE_NATIVE_UInt64 UnsignedLong)
  else()
    if(VTK_SIZEOF___INT64 MATCHES "^8$")
      set(VTK_TYPE_NATIVE_Int64 __Int64)
      if(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
        set(VTK_TYPE_NATIVE_UInt64 Unsigned__Int64)
      endif()
    endif()
  endif()
endif()
set(VTK_TYPE_NATIVE_Float32 Float)
set(VTK_TYPE_NATIVE_Float64 Double)
foreach(t Int8 Int16 Int32 Int64 UInt8 UInt16 UInt32 UInt64 Float32 Float64)
  set(VTK_TYPE_NAME ${t})
  set(VTK_TYPE_NATIVE ${VTK_TYPE_NATIVE_${t}})
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
  vtkArrayIterator
  vtkArrayIteratorTemplate
  vtkArray
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
  vtkArrayCoordinates.cxx
  vtkArrayExtents.cxx
  vtkArrayExtentsList.cxx
  vtkArrayIteratorTemplate.txx
  vtkArrayRange.cxx
  vtkArraySort.cxx
  vtkArrayWeights.cxx
  vtkBoundingBox.cxx
  vtkBreakPoint.cxx
  vtkCallbackCommand.cxx
  vtkCommonInformationKeyManager.cxx
  vtkDebugLeaksManager.cxx
  vtkEventForwarderCommand.cxx
  vtkFloatingPointExceptions
  vtkGarbageCollectorManager.cxx
  vtkIOStream.cxx
  vtkIndent.cxx
  vtkLargeInteger.cxx
  vtkOStrStreamWrapper.cxx
  vtkOStreamWrapper.cxx
  vtkOldStyleCallbackCommand.cxx
  vtkSimpleCriticalSection.cxx
  vtkSmartPointerBase.cxx
  vtkStdString.cxx
  vtkTimeStamp.cxx
  vtkVariant.cxx
  vtkWeakPointerBase.cxx
  vtkUnicodeString.cxx
  vtkDataArrayTemplate.h
  vtkDataArrayPrivate.txx

  vtkABI.h
  vtkArrayInterpolate.h
  vtkArrayInterpolate.txx
  vtkArrayIteratorIncludes.h
  vtkArrayIteratorTemplate.h
  vtkArrayIteratorTemplate.txx
  vtkArrayIteratorTemplateImplicit.txx
  vtkArrayPrint.h
  vtkArrayPrint.txx
  vtkAutoInit.h
  vtkDataArrayTemplate.txx
  vtkDataArrayTemplateHelper.cxx
  vtkDataArrayTemplateImplicit.txx
  vtkDenseArray.txx
  vtkIOStreamFwd.h
  vtkInformationInternals.h
  vtkMathUtilities.h
  vtkMappedDataArray.txx
  vtkNew.h
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
  vtkSMPThreadLocalObject.h
  vtkSMPTools.h
  SMP/${VTK_SMP_IMPLEMENTATION_TYPE}/vtkSMPTools.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMPToolsInternal.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMPThreadLocal.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkAtomicInt.h
  ${VTK_ATOMIC_CXX_FILE}

  WRAP_EXCLUDE
  )

set_source_files_properties(
  vtkDataArrayTemplate.h
  vtkDenseArray.h
  vtkSparseArray.h
  vtkTypedArray.h
  vtkTypeTemplate.h
  HEADER_FILE_ONLY
  )

set_source_files_properties(
  vtkArrayCoordinates.cxx
  vtkArrayExtents.cxx
  vtkArrayExtentsList.cxx
  vtkArrayRange.cxx
  vtkArraySort.cxx
  vtkArrayWeights.cxx
  vtkTimeStamp.cxx
  vtkVariant.cxx
  vtkDenseArray.txx
  vtkSparseArray.txx
  vtkTypedArray.txx
  vtkTypeTemplate.h
  vtkSystemIncludes.h
  vtkType.h
  vtkConfigure.h
  vtkMathConfigure.h
  vtkVersionMacros.h
  PROPERTIES WRAP_SPECIAL 1
)

# Activate code in "vtkVariant.cxx".  See the comment in the source.
set_source_files_properties(vtkVariant.cxx PROPERTIES
  COMPILE_FLAGS -DVTK_VARIANT_IMPL)

# Need nsl to resolve gethostbyname on SunOS-5.8
# and socket also
if(CMAKE_SYSTEM MATCHES "SunOS.*")
  set(KIT_LIBS ${KIT_LIBS} socket nsl)
endif()

vtk_module_library(vtkCommonCore ${Module_SRCS})

vtk_module_link_libraries(vtkCommonCore LINK_PRIVATE ${CMAKE_THREAD_LIBS} LINK_PUBLIC ${VTK_SMP_IMPLEMENTATION_LIBRARIES})
