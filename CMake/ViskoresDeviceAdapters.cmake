##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#
function(viskores_extract_real_library library real_library)
  if(NOT UNIX)
    set(${real_library} "${library}" PARENT_SCOPE)
    return()
  endif()

  #Read in the first 4 bytes and see if they are the ELF magic number
  set(_elf_magic "7f454c46")
  file(READ ${library} _hex_data OFFSET 0 LIMIT 4 HEX)
  if(_hex_data STREQUAL _elf_magic)
    #we have opened a elf binary so this is what
    #we should link too
    set(${real_library} "${library}" PARENT_SCOPE)
    return()
  endif()

  file(READ ${library} _data OFFSET 0 LIMIT 1024)
  if("${_data}" MATCHES "INPUT \\(([^(]+)\\)")
    #extract out the so name from REGEX MATCh command
    set(_proper_so_name "${CMAKE_MATCH_1}")

    #construct path to the real .so which is presumed to be in the same directory
    #as the input file
    get_filename_component(_so_dir "${library}" DIRECTORY)
    set(${real_library} "${_so_dir}/${_proper_so_name}" PARENT_SCOPE)
  else()
    #unable to determine what this library is so just hope everything works
    #add pass it unmodified.
    set(${real_library} "${library}" PARENT_SCOPE)
  endif()
endfunction()

if(Viskores_ENABLE_TBB AND NOT (TARGET viskores_tbb OR TARGET viskores::tbb))
  # Skip find_package(TBB) if we already have it
  if (NOT TARGET TBB::tbb)
    find_package(TBB REQUIRED)
  endif()

  add_library(viskores_tbb INTERFACE)
  target_link_libraries(viskores_tbb INTERFACE TBB::tbb)
  target_compile_definitions(viskores_tbb INTERFACE "TBB_VERSION_MAJOR=${TBB_VERSION_MAJOR}")
  set_target_properties(viskores_tbb PROPERTIES EXPORT_NAME tbb)
  install(TARGETS viskores_tbb EXPORT ${Viskores_EXPORT_NAME})
endif()

if(Viskores_ENABLE_OPENMP AND NOT (TARGET viskores_openmp OR TARGET viskores::openmp))
  find_package(OpenMP 4.0 REQUIRED COMPONENTS CXX QUIET)

  add_library(viskores_openmp INTERFACE)
  target_link_libraries(viskores_openmp INTERFACE OpenMP::OpenMP_CXX)
  target_compile_options(viskores_openmp INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${OpenMP_CXX_FLAGS}>)
  if(Viskores_ENABLE_CUDA)
    string(REPLACE ";" "," openmp_cuda_flags "-Xcompiler=${OpenMP_CXX_FLAGS}")
    target_compile_options(viskores_openmp INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:${openmp_cuda_flags}>)
  endif()
  set_target_properties(viskores_openmp PROPERTIES EXPORT_NAME openmp)
  install(TARGETS viskores_openmp EXPORT ${Viskores_EXPORT_NAME})
endif()

if(Viskores_ENABLE_CUDA)
  cmake_minimum_required(VERSION 3.13...3.15 FATAL_ERROR)
  enable_language(CUDA)

  if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA" AND
    CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 9.2)
    message(FATAL_ERROR "Viskores CUDA support requires version 9.2+")
  endif()

  if(NOT (TARGET viskores_cuda OR TARGET viskores::cuda))
    add_library(viskores_cuda INTERFACE)
    set_target_properties(viskores_cuda PROPERTIES EXPORT_NAME cuda)

    install(TARGETS viskores_cuda EXPORT ${Viskores_EXPORT_NAME})
    # Reserve `requires_static_builds` to potential work around issues
    # where Viskores doesn't work when building shared as virtual functions fail
    # inside device code. We don't want to force BUILD_SHARED_LIBS to a specific
    # value as that could impact other projects that embed Viskores. Instead what
    # we do is make sure that libraries built by viskores_library() are static
    # if they use CUDA
    #
    # This needs to be lower-case for the property to be properly exported
    # CMake 3.15 we can add `requires_static_builds` to the EXPORT_PROPERTIES
    # target property to have this automatically exported for us
    set_target_properties(viskores_cuda PROPERTIES
      requires_static_builds TRUE
    )

    target_compile_options(viskores_cuda INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>)

    if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA" AND
      CMAKE_CUDA_COMPILER_VERSION VERSION_GREATER_EQUAL 11.0)
      # CUDA 11+ deprecated C++11 support
      target_compile_features(viskores_cuda INTERFACE cxx_std_14)
    endif()

    # If we have specified CMAKE_CUDA_ARCHITECTURES and CMake >= 3.18 we are
    # done setting up viskores_cuda.
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES OR CMAKE_VERSION VERSION_LESS 3.18)

      # Recommend user to use CMAKE_CUDA_ARCHITECTURES instead
      if(DEFINED Viskores_CUDA_Architecture AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
        message(DEPRECATION "Viskores_CUDA_Architecture used, use CMAKE_CUDA_ARCHITECTURES instead in CMake >= 3.18")
      endif()

      # We disable CMAKE_CUDA_ARCHITECTURES since we add the arch manually
      set(CMAKE_CUDA_ARCHITECTURES OFF)

      # add the -gencode flags so that all cuda code
      # way compiled properly

      #---------------------------------------------------------------------------
      # When using CMAKE >= 3.18 use instead CMAKE_CUDA_ARCHITECTURES since it
      # is the canonical way to specify archs in modern CMAKE.
      #
      # Populates CMAKE_CUDA_FLAGS with the best set of flags to compile for a
      # given GPU architecture. The majority of developers should leave the
      # option at the default of 'native' which uses system introspection to
      # determine the smallest numerous of virtual and real architectures it
      # should target.
      #
      # The option of 'all' is provided for people generating libraries that
      # will deployed to any number of machines, it will compile all CUDA code
      # for all major virtual architectures, guaranteeing that the code will run
      # anywhere.
      #
      # The option 'none' is provided so that when being built as part of another
      # project, its own custom flags can be used.
      #
      # 1 - native
      #   - Uses system introspection to determine compile flags
      # 2 - fermi
      #   - Uses: --generate-code=arch=compute_20,code=sm_20
      # 3 - kepler
      #   - Uses: --generate-code=arch=compute_30,code=sm_30
      #   - Uses: --generate-code=arch=compute_35,code=sm_35
      # 4 - maxwell
      #   - Uses: --generate-code=arch=compute_50,code=sm_50
      # 5 - pascal
      #   - Uses: --generate-code=arch=compute_60,code=sm_60
      # 6 - volta
      #   - Uses: --generate-code=arch=compute_70,code=sm_70
      # 7 - turing
      #   - Uses: --generate-code=arch=compute_75,code=sm_75
      # 8 - ampere
      #   - Uses: --generate-code=arch=compute_80,code=sm_80
      #   - Uses: --generate-code=arch=compute_86,code=sm_86
      # 8 - all
      #   - Uses: --generate-code=arch=compute_30,code=sm_30
      #   - Uses: --generate-code=arch=compute_35,code=sm_35
      #   - Uses: --generate-code=arch=compute_50,code=sm_50
      #   - Uses: --generate-code=arch=compute_60,code=sm_60
      #   - Uses: --generate-code=arch=compute_70,code=sm_70
      #   - Uses: --generate-code=arch=compute_75,code=sm_75
      #   - Uses: --generate-code=arch=compute_80,code=sm_80
      #   - Uses: --generate-code=arch=compute_86,code=sm_86
      # 8 - none
      #

      #specify the property
      set(Viskores_CUDA_Architecture "native" CACHE STRING "Which GPU Architecture(s) to compile for")
      set_property(CACHE Viskores_CUDA_Architecture PROPERTY STRINGS native fermi kepler maxwell pascal volta turing ampere all none)

      #detect what the property is set too
      if(Viskores_CUDA_Architecture STREQUAL "native")

        if(VISKORES_CUDA_NATIVE_EXE_PROCESS_RAN_OUTPUT)
          #Use the cached value
          set(arch_flags ${VISKORES_CUDA_NATIVE_EXE_PROCESS_RAN_OUTPUT})
        else()

          #run execute_process to do auto_detection
          if(CMAKE_GENERATOR MATCHES "Visual Studio")
            set(args "-ccbin" "${CMAKE_CXX_COMPILER}" "--run" "${Viskores_CMAKE_MODULE_PATH}/ViskoresDetectCUDAVersion.cu")
          elseif(CUDA_HOST_COMPILER)
            set(args "-ccbin" "${CUDA_HOST_COMPILER}" "--run" "${Viskores_CMAKE_MODULE_PATH}/ViskoresDetectCUDAVersion.cu")
          else()
            set(args "--run" "${Viskores_CMAKE_MODULE_PATH}/ViskoresDetectCUDAVersion.cu")
          endif()

          execute_process(
                  COMMAND ${CMAKE_CUDA_COMPILER} ${args}
                  RESULT_VARIABLE ran_properly
                  OUTPUT_VARIABLE run_output
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

          if(ran_properly EQUAL 0)
            #find the position of the "--generate-code" output. With some compilers such as
            #msvc we get compile output plus run output. So we need to strip out just the
            #run output
            string(FIND "${run_output}" "--generate-code" position)
            string(SUBSTRING "${run_output}" ${position} -1 run_output)

            set(arch_flags ${run_output})
            set(VISKORES_CUDA_NATIVE_EXE_PROCESS_RAN_OUTPUT ${run_output} CACHE INTERNAL
                    "device type(s) for cuda[native]")
          else()
            message(FATAL_ERROR "Error detecting architecture flags for CUDA. Please set Viskores_CUDA_Architecture manually.")
          endif()
        endif()
      endif()

      if(Viskores_CUDA_Architecture STREQUAL "fermi")
        set(arch_flags --generate-code=arch=compute_20,code=sm_20)
      elseif(Viskores_CUDA_Architecture STREQUAL "kepler")
        set(arch_flags --generate-code=arch=compute_30,code=sm_30
                       --generate-code=arch=compute_35,code=sm_35)
      elseif(Viskores_CUDA_Architecture STREQUAL "maxwell")
        set(arch_flags --generate-code=arch=compute_50,code=sm_50)
      elseif(Viskores_CUDA_Architecture STREQUAL "pascal")
        set(arch_flags --generate-code=arch=compute_60,code=sm_60
                       --generate-code=arch=compute_61,code=sm_61)
      elseif(Viskores_CUDA_Architecture STREQUAL "volta")
        set(arch_flags --generate-code=arch=compute_70,code=sm_70)
      elseif(Viskores_CUDA_Architecture STREQUAL "turing")
        set(arch_flags --generate-code=arch=compute_75,code=sm_75)
      elseif(Viskores_CUDA_Architecture STREQUAL "ampere")
        set(arch_flags --generate-code=arch=compute_80,code=sm_80
                       --generate-code=arch=compute_86,code=sm_86)
      elseif(Viskores_CUDA_Architecture STREQUAL "all")
        set(arch_flags --generate-code=arch=compute_30,code=sm_30
                       --generate-code=arch=compute_35,code=sm_35
                       --generate-code=arch=compute_50,code=sm_50
                       --generate-code=arch=compute_60,code=sm_60
                       --generate-code=arch=compute_70,code=sm_70
                       --generate-code=arch=compute_75,code=sm_75
                       --generate-code=arch=compute_80,code=sm_80
                       --generate-code=arch=compute_86,code=sm_86)
      endif()

      string(REPLACE ";" " " arch_flags "${arch_flags}")

      if(POLICY CMP0105)
        cmake_policy(GET CMP0105 policy_105_enabled)
      endif()

      if(policy_105_enabled STREQUAL "NEW")
        target_compile_options(viskores_cuda INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:${arch_flags}>)
        target_link_options(viskores_cuda INTERFACE $<DEVICE_LINK:${arch_flags}>)
      else()
        # Before 3.18 we had to use CMAKE_CUDA_FLAGS as we had no way
        # to propagate flags to the device link step
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} ${arch_flags}")
      endif()

      # This needs to be lower-case for the property to be properly exported
      # CMake 3.15 we can add `cuda_architecture_flags` to the EXPORT_PROPERTIES
      # target property to have this automatically exported for us
      set(Viskores_CUDA_Architecture_Flags "${arch_flags}")
      set_target_properties(viskores_cuda PROPERTIES cuda_architecture_flags "${arch_flags}")
      unset(arch_flags)
    endif()
  endif()
endif()

#-----------------------------------------------------------------------------
# Kokkos with its Cuda backend enabled, expects everything to be compiled using its
# `nvcc-wrapper` as the CXX compiler. As the name suggests, nvcc-wrapper is a wrapper around
# Cuda's nvcc compiler. Kokkos targets have all of the flags meant for the nvcc compiler set as the
# CXX compiler flags. This function changes all such flags to be CUDA flags so that we can use
# CMake and viskores's existing infrastructure to compile for Cuda and Host separately. Without this
# all of the files will be compiled using nvcc which can be very time consuming. It can also have
# issues with calling host functions from device functions when compiling code for other backends.
function(kokkos_fix_compile_options)
  set(targets Kokkos::kokkos)
  set(seen_targets)
  set(cuda_arch)

  while(targets)
    list(GET targets 0 target_name)
    list(REMOVE_AT targets 0)

    get_target_property(link_libraries ${target_name} INTERFACE_LINK_LIBRARIES)
    foreach(lib_target IN LISTS link_libraries)
      if (TARGET ${lib_target})
        if (lib_target IN_LIST seen_targets)
          continue()
        endif()

        list(APPEND seen_targets ${lib_target})
        list(APPEND targets ${lib_target})
        get_target_property(compile_options ${lib_target} INTERFACE_COMPILE_OPTIONS)
        if (compile_options)
          string(REGEX MATCH "[$]<[$]<COMPILE_LANGUAGE:CXX>:-Xcompiler;.*>" cxx_compile_options "${compile_options}")
          string(REGEX MATCH "-arch=sm_[0-9][0-9]" cuda_arch "${compile_options}")
          string(REPLACE "-Xcompiler;" "" cxx_compile_options "${cxx_compile_options}")
          list(TRANSFORM compile_options REPLACE "--relocatable-device-code=true" "") #We use CMake for this flag
          list(TRANSFORM compile_options REPLACE "COMPILE_LANGUAGE:CXX" "COMPILE_LANGUAGE:CUDA")
          list(APPEND compile_options "${cxx_compile_options}")
          set_property(TARGET ${lib_target} PROPERTY INTERFACE_COMPILE_OPTIONS ${compile_options})
        endif()

        set_property(TARGET ${lib_target} PROPERTY INTERFACE_LINK_OPTIONS "")
      endif()
    endforeach()
  endwhile()

  set_property(TARGET viskores_kokkos PROPERTY INTERFACE_LINK_OPTIONS "$<DEVICE_LINK:${cuda_arch}>")
  if (OPENMP IN_LIST Kokkos_DEVICES)
    set_property(TARGET viskores_kokkos PROPERTY INTERFACE_LINK_OPTIONS "$<HOST_LINK:-fopenmp>")
  endif()
endfunction()

if(Viskores_ENABLE_KOKKOS AND NOT TARGET viskores_kokkos)
  cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

  find_package(Kokkos 3.7 REQUIRED)

  # We must empty this property for every kokkos backend device since it
  # contains a generator expresion which breaks some of our users builds.
  set_property(TARGET Kokkos::kokkoscore PROPERTY INTERFACE_COMPILE_DEFINITIONS "")

  if (CUDA IN_LIST Kokkos_DEVICES)
    cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
    enable_language(CUDA)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND
       CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA" AND CMAKE_CUDA_COMPILER_VERSION VERSION_GREATER_EQUAL "10.0" AND CMAKE_CUDA_COMPILER_VERSION VERSION_LESS "11.0" AND
       CMAKE_BUILD_TYPE STREQUAL "Release")
      message(WARNING "There is a known issue with Cuda 10 and -O3 optimization. Switching to -O2. Please refer to issue #555.")
      string(REPLACE "-O3" "-O2" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
      string(REPLACE "-O3" "-O2" CMAKE_CUDA_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    endif()

    string(REGEX MATCH "[0-9][0-9]$" cuda_arch ${Kokkos_ARCH})
    set(CMAKE_CUDA_ARCHITECTURES ${cuda_arch})
    message(STATUS "Detected Cuda arch from Kokkos: ${cuda_arch}")

    add_library(viskores_kokkos_cuda INTERFACE)
    set_property(TARGET viskores_kokkos_cuda PROPERTY EXPORT_NAME kokkos_cuda)
    install(TARGETS viskores_kokkos_cuda EXPORT ${Viskores_EXPORT_NAME})
  elseif(HIP IN_LIST Kokkos_DEVICES)
    cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
    enable_language(HIP)

    set_target_properties(Kokkos::kokkoscore PROPERTIES
      INTERFACE_COMPILE_OPTIONS ""
      INTERFACE_LINK_OPTIONS ""
      )
    add_library(viskores_kokkos_hip INTERFACE)
    set_property(TARGET viskores_kokkos_hip PROPERTY EXPORT_NAME kokkos_hip)
    install(TARGETS viskores_kokkos_hip EXPORT ${Viskores_EXPORT_NAME})
  endif()

  add_library(viskores_kokkos INTERFACE IMPORTED GLOBAL)
  set_target_properties(viskores_kokkos PROPERTIES INTERFACE_LINK_LIBRARIES "Kokkos::kokkos")

  if (TARGET viskores_kokkos_cuda)
    kokkos_fix_compile_options()
  endif()
endif()

if(NOT TARGET Threads::Threads)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
endif()
