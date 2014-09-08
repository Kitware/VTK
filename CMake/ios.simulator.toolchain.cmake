set(CMAKE_SYSTEM_NAME Darwin)

exec_program(/usr/bin/xcode-select ARGS -print-path OUTPUT_VARIABLE XCODE_DEVELOPER_DIR)

find_program(CMAKE_C_COMPILER NAME gcc
  PATHS
  "${XCODE_DEVELOPER_DIR}/Platforms/iPhoneSimulator.platform/Developer/usr/bin/"
  /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/
  /Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/
  NO_DEFAULT_PATH)

find_program(CMAKE_CXX_COMPILER NAME g++
  PATHS
  "{XCODE_DEVELOPER_DIR}/Platforms/iPhoneSimulator.platform/Developer/usr/bin/"
  /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/
  /Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/
  NO_DEFAULT_PATH)

set(CMAKE_OSX_ARCHITECTURES i386)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")

# Set the CMAKE_OSX_SYSROOT to the latest SDK found
set(CMAKE_OSX_SYSROOT)
set(possible_sdk_roots
  "${XCODE_DEVELOPER_DIR}/Platforms/iPhoneSimulator.platform/Developer/SDKs"
  /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs
  /Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs
  )
foreach(sdk_root ${possible_sdk_roots})
  foreach(sdk iPhoneSimulator4.3.sdk iPhoneSimulator5.0.sdk iPhoneSimulator5.1.sdk iPhoneSimulator6.0.sdk iPhoneSimulator6.1.sdk iPhoneSimulator7.0.sdk iPhoneSimulator7.1.sdk)
    if (EXISTS ${sdk_root}/${sdk} AND IS_DIRECTORY ${sdk_root}/${sdk})
      set(CMAKE_OSX_SYSROOT ${sdk_root}/${sdk})
    endif()
  endforeach()
endforeach()
if (NOT CMAKE_OSX_SYSROOT)
  message(FATAL_ERROR "Could not find a usable iOS SDK in ${sdk_root}")
endif()
message(STATUS "-- Using iOS SDK: ${CMAKE_OSX_SYSROOT}")

# If the we are on 7.0 or higher SDK we should add simulator version min to get
# things to compile.  This is probably more properly handled with a compiler version
# check but this works for now.
if(CMAKE_OSX_SYSROOT MATCHES iPhoneSimulator7.[0-9].sdk)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mios-simulator-version-min=5.0")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mios-simulator-version-min=5.0")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__IPHONE_OS_VERSION_MIN_REQUIRED=50000")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__PHONE_OS_VERSION_MIN_REQIORED=50000")
endif()

set(CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}" CACHE STRING "osx architectures")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "c++ flags")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "c flags")
set(CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}" CACHE PATH "osx sysroot")


set(CMAKE_FIND_ROOT_PATH ${CMAKE_OSX_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(APPLE_IOS ON)
set(TARGET_IPHONE_SIMULATOR ON)