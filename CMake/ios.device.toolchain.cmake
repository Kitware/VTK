set(CMAKE_SYSTEM_NAME Darwin)

# Set developer directory
execute_process(COMMAND /usr/bin/xcode-select -print-path
                OUTPUT_VARIABLE XCODE_DEVELOPER_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE)

# Locate gcc
execute_process(COMMAND /usr/bin/xcrun -sdk iphoneos -find gcc
                OUTPUT_VARIABLE CMAKE_C_COMPILER
                OUTPUT_STRIP_TRAILING_WHITESPACE)

# Locate g++
execute_process(COMMAND /usr/bin/xcrun -sdk iphoneos -find g++
                OUTPUT_VARIABLE CMAKE_CXX_COMPILER
                OUTPUT_STRIP_TRAILING_WHITESPACE)

# Set the CMAKE_OSX_SYSROOT to the latest SDK found
execute_process(COMMAND /usr/bin/xcrun -sdk iphoneos --show-sdk-path
                OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
                OUTPUT_STRIP_TRAILING_WHITESPACE)

# Set compilation flags
set(CMAKE_OSX_ARCHITECTURES "armv7;armv7s")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -miphoneos-version-min=5.0 -fvisibility=hidden -fvisibility-inlines-hidden")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -miphoneos-version-min=5.0 -fvisibility=hidden -fvisibility-inlines-hidden")

set(CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}" CACHE STRING "osx architectures")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "c++ flags")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "c flags")
set(CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}" CACHE PATH "osx sysroot")
set(MACOSX_BUNDLE_GUI_IDENTIFIER CACHE STRING "com.kitware.\${PRODUCT_NAME:identifier}")

set(CMAKE_FIND_ROOT_PATH ${CMAKE_OSX_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(APPLE_IOS ON)
set(TARGET_OS_IPHONE ON)


#
# Legacy searches (xcrun is not available)
#

# gcc
if (NOT CMAKE_C_COMPILER)
  find_program(CMAKE_C_COMPILER NAME gcc
    PATHS
    "${XCODE_DEVELOPER_DIR}/Platforms/iPhoneOS.platform/Developer/usr/bin/"
    /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/
    /Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/
    NO_DEFAULT_PATH)
endif()

# g++
if (NOT CMAKE_CXX_COMPILER)
  find_program(CMAKE_CXX_COMPILER NAME g++
    PATHS
    "${XCODE_DEVELOPER_DIR}/Platforms/iPhoneOS.platform/Developer/usr/bin/"
    /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/
    /Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/
    NO_DEFAULT_PATH)
endif()

# sysroot
if (NOT CMAKE_OSX_SYSROOT)
  set(possible_sdk_roots
    "${XCODE_DEVELOPER_DIR}/Platforms/iPhoneOS.platform/Developer/SDKs"
    /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs
    /Developer/Platforms/iPhoneOS.platform/Developer/SDKs
    )
  foreach(sdk_root ${possible_sdk_roots})
    foreach(sdk iPhoneOS4.3.sdk iPhoneOS5.0.sdk iPhoneOS5.1.sdk iPhoneOS6.0.sdk
                iPhoneOS6.1.sdk iPhoneOS7.0.sdk iPhoneOS7.1.sdk iPhoneOS8.0.sdk)
      if (EXISTS ${sdk_root}/${sdk} AND IS_DIRECTORY ${sdk_root}/${sdk})
        set(CMAKE_OSX_SYSROOT ${sdk_root}/${sdk})
      endif()
    endforeach()
  endforeach()
  if (NOT CMAKE_OSX_SYSROOT)
    message(FATAL_ERROR "Could not find a usable iOS SDK in ${sdk_root}")
  endif()
endif()

message(STATUS "-- gcc found at: ${CMAKE_C_COMPILER}")
message(STATUS "-- g++ found at: ${CMAKE_CXX_COMPILER}")
message(STATUS "-- Using iOS SDK: ${CMAKE_OSX_SYSROOT}")
