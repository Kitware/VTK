SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR k1om)
SET(CMAKE_SYSTEM_VERSION 1)

SET(_CMAKE_TOOLCHAIN_PREFIX  "x86_64-k1om-linux-")

# specify the cross compiler
SET(CMAKE_C_COMPILER   "icc")
SET(CMAKE_CXX_COMPILER "icpc")
SET(CMAKE_C_FLAGS "-mmic" CACHE STRING "" FORCE)
SET(CMAKE_CXX_FLAGS "-mmic" CACHE STRING "" FORCE)

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH
  "/opt/mpss/3.3/sysroots/k1om-mpss-linux"
  "/usr/linux-k1om-4.7/"
)

# Following cannot be used for stampede where libraries can be under
# non root path directories

# search for programs in the build host directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
