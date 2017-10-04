if (CMAKE_MAJOR_VERSION GREATER 2)
    cmake_policy(SET CMP0042 NEW) # osx rpath
    cmake_policy(SET CMP0011 NEW) # policy setting
endif()
