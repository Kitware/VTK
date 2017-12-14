include(CheckSymbolExists)

# Core configuration options.
if("${CMAKE_SIZEOF_VOID_P}" GREATER 4)
 set(VTK_USE_64BIT_IDS_DEFAULT ON)
else()
 set(VTK_USE_64BIT_IDS_DEFAULT OFF)
 set(VTK_USE_64BIT_TIMESTAMPS_DEFAULT OFF)
 option(VTK_USE_64BIT_TIMESTAMPS "Build VTK with 64 bit timestamps"
  ${VTK_USE_64BIT_TIMESTAMPS_DEFAULT})
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

mark_as_advanced(VTK_DEBUG_LEAKS VTK_USE_64BIT_IDS VTK_ALL_NEW_OBJECT_FACTORY VTK_USE_64BIT_TIMESTAMPS)

set(vtkCommonCore_EXPORT_OPTIONS
  VTK_DEBUG_LEAKS
  VTK_USE_64BIT_IDS
  VTK_ALL_NEW_OBJECT_FACTORY
  VTK_USE_64BIT_TIMESTAMPS
  )
set(vtk_smp_sources)
set(vtk_smp_headers)
set(vtk_smp_defines)
set(vtk_smp_includes)
set(vtk_smp_libraries)
include("${CMAKE_CURRENT_SOURCE_DIR}/vtkSMPSelection.cmake")

list(APPEND vtkCommonCore_SYSTEM_INCLUDE_DIRS ${vtk_smp_includes})

# Generate the vtkTypeList_Create macros:
include("${CMAKE_CURRENT_SOURCE_DIR}/vtkCreateTypeListMacros.cmake")
CreateTypeListMacros(
  VTK_TYPELISTMACRO_HEADER_CONTENTS
  99
  vtkTypeList_Create
  "vtkTypeList::TypeList"
  "vtkTypeList::NullType")
configure_file(vtkTypeListMacros.h.in vtkTypeListMacros.h)

# And the ArrayDispatch array list header:
option(VTK_DISPATCH_AOS_ARRAYS "Include array-of-structs vtkDataArray subclasses in dispatcher." ON)
option(VTK_DISPATCH_SOA_ARRAYS "Include struct-of-arrays vtkDataArray subclasses in dispatcher." OFF)
option(VTK_DISPATCH_TYPED_ARRAYS "Include vtkTypedDataArray subclasses (e.g. old mapped arrays) in dispatcher." OFF)
option(VTK_WARN_ON_DISPATCH_FAILURE "If enabled, vtkArrayDispatch will print a warning when a dispatch fails." OFF)
mark_as_advanced(
  VTK_DISPATCH_AOS_ARRAYS
  VTK_DISPATCH_SOA_ARRAYS
  VTK_DISPATCH_TYPED_ARRAYS
  VTK_WARN_ON_DISPATCH_FAILURE)

include("${CMAKE_CURRENT_SOURCE_DIR}/vtkCreateArrayDispatchArrayList.cmake")
vtkArrayDispatch_default_array_setup()
vtkArrayDispatch_generate_array_header(VTK_ARRAYDISPATCH_ARRAY_LIST)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkArrayDispatchArrayList.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkArrayDispatchArrayList.h"
  @ONLY)

SET(Module_SRCS
  vtkAOSDataArrayTemplate.txx
  vtkAbstractArray.cxx
  vtkAnimationCue.cxx
  vtkArray.cxx
  vtkArrayCoordinates.cxx
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
  vtkDataArray.cxx
  vtkDataArrayCollection.cxx
  vtkDataArrayCollectionIterator.cxx
  vtkDataArrayPrivate.txx
  vtkDataArraySelection.cxx
  vtkDebugLeaks.cxx
  vtkDebugLeaksManager.cxx
  vtkDenseArray.txx
  vtkDoubleArray.cxx
  vtkDynamicLoader.cxx
  vtkEventForwarderCommand.cxx
  vtkFileOutputWindow.cxx
  vtkFloatArray.cxx
  vtkFloatingPointExceptions.cxx
  vtkGarbageCollector.cxx
  vtkGarbageCollectorManager.cxx
  vtkGaussianRandomSequence.cxx
  vtkGenericDataArray.txx
  vtkIdList.cxx
  vtkIdListCollection.cxx
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
  vtkOStrStreamWrapper.cxx
  vtkOStreamWrapper.cxx
  vtkObject.cxx
  vtkObjectBase.cxx
  vtkObjectFactory.cxx
  vtkObjectFactoryCollection.cxx
  vtkOldStyleCallbackCommand.cxx
  vtkOutputWindow.cxx
  vtkOverrideInformation.cxx
  vtkOverrideInformationCollection.cxx
  vtkPoints.cxx
  vtkPoints2D.cxx
  vtkPriorityQueue.cxx
  vtkRandomSequence.cxx
  vtkReferenceCount.cxx
  vtkSOADataArrayTemplate.txx
  vtkSOADataArrayTemplateInstantiate.cxx
  vtkScalarsToColors.cxx
  vtkShortArray.cxx
  vtkSignedCharArray.cxx
  vtkSimpleCriticalSection.cxx
  vtkSmartPointerBase.cxx
  vtkSortDataArray.cxx
  vtkSparseArray.txx
  vtkStdString.cxx
  vtkStringArray.cxx
  vtkStringOutputWindow.cxx
  vtkSystemIncludes.h
  vtkTimePointUtility.cxx
  vtkTimeStamp.cxx
  vtkType.h
  vtkTypedArray.txx
  vtkTypedDataArray.txx
  vtkUnicodeString.cxx
  vtkUnicodeStringArray.cxx
  vtkUnsignedCharArray.cxx
  vtkUnsignedIntArray.cxx
  vtkUnsignedLongArray.cxx
  vtkUnsignedLongLongArray.cxx
  vtkUnsignedShortArray.cxx
  vtkVariant.cxx
  vtkVariantArray.cxx
  vtkVersion.cxx
  vtkVoidArray.cxx
  vtkWeakPointerBase.cxx
  vtkWeakReference.cxx
  vtkWindow.cxx
  vtkWrappingHints.h
  vtkXMLFileOutputWindow.cxx
  ${vtk_smp_sources}
  ${CMAKE_CURRENT_BINARY_DIR}/vtkConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkMathConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkVersionMacros.h
  )

set(${vtk-module}_HDRS
  vtkABI.h
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
  vtkGenericDataArrayLookupHelper.h
  vtkIOStream.h
  vtkIOStreamFwd.h
  vtkInformationInternals.h
  vtkMappedDataArray.h
  vtkMathUtilities.h
  vtkMersenneTwister.h
  vtkNew.h
  vtkSOADataArrayTemplate.txx
  vtkSetGet.h
  vtkSmartPointer.h
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
  vtkWeakReference.h
  vtkWin32Header.h
  vtkWindows.h
  ${vtk_smp_headers}
  ${CMAKE_CURRENT_BINARY_DIR}/vtkArrayDispatchArrayList.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkToolkits.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTypeListMacros.h
  )

add_definitions(${vtk_smp_defines})

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
endif()

if(ANDROID)
  list(APPEND Module_SRCS
    vtkAndroidOutputWindow.cxx
    )
endif()

# Platform configuration tests.
include(TestBigEndian)
test_big_endian(VTK_WORDS_BIGENDIAN)

# Provide version macros through vtkConfigure.h for compatibility.
if(VTK_LEGACY_REMOVE)
  set(VTK_CONFIG_LEGACY_VERSION "")
else()
  set(VTK_CONFIG_LEGACY_VERSION "#include \"vtkVersionMacros.h\" // removed by VTK_LEGACY_REMOVE")
endif()

configure_file(vtkVersionMacros.h.in vtkVersionMacros.h @ONLY)
configure_file(vtkConfigure.h.in vtkConfigure.h @ONLY)
configure_file(vtkToolkits.h.in vtkToolkits.h @ONLY)

add_custom_command(
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/CaseFolding.txt"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/vtkUnicodeString.cmake"
  COMMAND "${CMAKE_COMMAND}"
          "-Dsource_dir=${CMAKE_CURRENT_SOURCE_DIR}"
          "-Dbinary_dir=${CMAKE_CURRENT_BINARY_DIR}"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/vtkUnicodeString.cmake"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/vtkUnicodeCaseFoldData.h")

set_source_files_properties(vtkUnicodeString.cxx
  PROPERTIES
    OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/vtkUnicodeCaseFoldData.h")

include("${CMAKE_CURRENT_SOURCE_DIR}/vtkTypeArrays.cmake")

include(CheckCXXSourceCompiles)

macro(CHECK_CXX_EXPRESSION_COMPILES EXPRESSION FILES VARIABLE)
  set(SOURCE "/* CHECK_CXX_EXPRESSION_COMPILES */\n")
  foreach(FILE ${FILES})
    set(SOURCE "${SOURCE}#include <${FILE}>\n")
  endforeach()
  set(SOURCE "${SOURCE}\nint main()\n{\n")
  set(SOURCE "${SOURCE}  static_cast<void>(${EXPRESSION});\n\n")
  set(SOURCE "${SOURCE}  return 0;\n}\n")
  check_cxx_source_compiles("${SOURCE}" "${VARIABLE}")
endmacro()

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
include(CheckIncludeFile)
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
  vtkDataArrayTemplate.h
  vtkDenseArray.h
  vtkSparseArray.h
  vtkTypedArray.h
  "${VTK_TYPELISTMACROS_HEADER}"
  HEADER_FILE_ONLY
  )

set_source_files_properties(
  vtkArrayIteratorTemplateInstantiate.cxx # Has no header
  vtkSOADataArrayTemplateInstantiate.cxx # Has no header
  PROPERTIES SKIP_HEADER_INSTALL 1
)

if(MSVC)
  set_source_files_properties(
    vtkDataArray.cxx
    PROPERTIES COMPILE_FLAGS /bigobj
    )
endif()

# Need nsl to resolve gethostbyname on SunOS-5.8
# and socket also
if(CMAKE_SYSTEM MATCHES "SunOS.*")
  set(KIT_LIBS ${KIT_LIBS} socket nsl)
endif()

vtk_module_library(vtkCommonCore ${Module_SRCS})

vtk_module_link_libraries(vtkCommonCore LINK_PRIVATE ${CMAKE_THREAD_LIBS} LINK_PUBLIC ${vtk_smp_libraries})

if(ANDROID)
  target_link_libraries(vtkCommonCore LINK_PRIVATE log)
endif()
