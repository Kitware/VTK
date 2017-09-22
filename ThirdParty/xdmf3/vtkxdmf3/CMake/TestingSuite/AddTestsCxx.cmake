INCLUDE(TestingSetup)

# We should have one place that points to the cxx source directory and the cxx
# binary directory
SET(cxx_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
SET(cxx_binary_dir ${CMAKE_CURRENT_BINARY_DIR})

# CXX Add Dependencies Macro
# Author: Brian Panneton
# Description: This macro adds the cxx test dependencies.
# 	  Note: The tests already depend on their own file
# Parameters:         
#              dependencies         = any dependencies needed for cxx tests
MACRO(ADD_TEST_CXX_DEPENDENCIES dependencies)
    IF(NOT ("${dependencies}" STREQUAL ""))
        SET_PROPERTY(GLOBAL APPEND PROPERTY CXX_TEST_DEPENDENCIES
            "${dependencies}"
 	)
    ENDIF()
ENDMACRO()

# Cxx Add LDPath  Macro
# Author: Brian Panneton
# Description: This macro adds the cxx test ldpaths.
# Parameters:         
#               ld  = any ldpaths needed for cxx tests
MACRO(ADD_TEST_CXX_LDPATH ld)
    GET_PROPERTY(ldpath GLOBAL PROPERTY CXX_TEST_LDPATH)
    IF("${ld}" STRGREATER "")
        SET_PROPERTY(GLOBAL PROPERTY CXX_TEST_LDPATH 
                "${ldpath}${sep}${ld}" 
        )
    ENDIF()
ENDMACRO()

# Cxx Add Path  Macro
# Author: Brian Panneton
# Description: This macro adds the cxx test paths.
# Parameters:         
#               p  = any paths needed for cxx tests
MACRO(ADD_TEST_CXX_PATH p)
    GET_PROPERTY(path GLOBAL PROPERTY CXX_TEST_PATH)
    IF("${p}" STRGREATER "")
        SET_PROPERTY(GLOBAL PROPERTY CXX_TEST_PATH 
                "${path}${sep}${p}" 
        )
    ENDIF()
ENDMACRO()

# CXX Test Macro
# Author: Brian Panneton
# Description: This macro builds and add the cxx test in one shot.
# Parameters:         
#		executable      = executable name
#		dup		= duplicate number
#		tdep		= test dependency (Full Test Name with Prefix)
#             	${ARGN}         = any arguments for the executable
MACRO(ADD_TEST_CXX executable)
    PARSE_TEST_ARGS("${ARGN}")
    
    IF(EXISTS ${cxx_source_dir}/${executable}.cpp)
        ADD_EXECUTABLE(${executable}${dup} ${cxx_source_dir}/${executable}.cpp)
    ENDIF()

    IF(EXISTS ${cxx_source_dir}/${executable}.cxx)
        ADD_EXECUTABLE(${executable}${dup} ${cxx_source_dir}/${executable}.cxx)
    ENDIF()
	
    GET_PROPERTY(cxx_dependencies GLOBAL PROPERTY CXX_TEST_DEPENDENCIES)
    GET_PROPERTY(cxx_ldpath GLOBAL PROPERTY CXX_TEST_LDPATH)
    GET_PROPERTY(cxx_path GLOBAL PROPERTY CXX_TEST_PATH)
    TARGET_LINK_LIBRARIES(${executable}${dup} ${cxx_dependencies})
 
    # Take care of windowisims
    IF(WIN32)
        SET_TARGET_PROPERTIES(${executable}${dup} PROPERTIES 
            PREFIX ../
            RUNTIME_OUTPUT_DIRECTORY ${cxx_binary_dir}
            LIBRARY_OUTPUT_DIRECTORY ${cxx_binary_dir}
            ARCHIVE_OUTPUT_DIRECTORY ${cxx_binary_dir})

        IF("${cxx_path}" STREQUAL "")
            SET(cxx_path ${cxx_ldpath})
        ENDIF()
    ENDIF()

    SET_CORE("${cxx_binary_dir}")
    ADD_TEST(Cxx${is_core}_${executable}${dup} ${CMAKE_COMMAND}
            -D "EXECUTABLE=${executable}${dup}"
            -D "ARGUMENTS=${arguments}"           
            -D "PATH=${cxx_path}"
            -D "SEPARATOR=${sep}"
            -P "${cxx_binary_dir}/TestDriverCxx.cmake"
    )
    IF(NOT "${tdep}" STREQUAL "")
	    SET_TESTS_PROPERTIES(Cxx${is_core}_${executable}${dup}
            PROPERTIES DEPENDS ${tdep})
    ENDIF()
ENDMACRO()

# CXX MPI Test Macro
# Author: Andrew Burns
# Description: This macro builds and adds a script to execute MPI tests.
# Parameters:
#               executable      = script name
#               files           = code to be compiled and executed by the script
#               tdep            = test dependency (Full Test Name with Prefix)
#               ${ARGN}         = any arguments for the executable
MACRO(ADD_MPI_TEST_CXX script files)
    PARSE_TEST_ARGS("${ARGN}")

    set(tempfiles ${files})

    WHILE(NOT "${tempfiles}" STREQUAL "")
        # ${executable}
        STRING(REGEX MATCH "([^ ,])+,|([^ ,])+" executable "${tempfiles}")
        STRING(REGEX REPLACE "," "" executable "${executable}")
        STRING(REGEX REPLACE "${executable},|${executable}" "" trimmed "${tempfiles}")

        set(tempfiles ${trimmed})

        IF(EXISTS ${cxx_source_dir}/${executable}.cpp)
            ADD_EXECUTABLE(${executable} ${cxx_source_dir}/${executable}.cpp)
        ENDIF()

        IF(EXISTS ${cxx_source_dir}/${executable}.cxx)
            ADD_EXECUTABLE(${executable} ${cxx_source_dir}/${executable}.cxx)
        ENDIF()

        GET_PROPERTY(cxx_dependencies GLOBAL PROPERTY CXX_TEST_DEPENDENCIES)
        GET_PROPERTY(cxx_ldpath GLOBAL PROPERTY CXX_TEST_LDPATH)
        GET_PROPERTY(cxx_path GLOBAL PROPERTY CXX_TEST_PATH)
        TARGET_LINK_LIBRARIES(${executable} ${cxx_dependencies})

        # Take care of windowisims
        IF(WIN32)
            SET_TARGET_PROPERTIES(${executable} PROPERTIES
                PREFIX ../
                RUNTIME_OUTPUT_DIRECTORY ${cxx_binary_dir}
                LIBRARY_OUTPUT_DIRECTORY ${cxx_binary_dir}
                ARCHIVE_OUTPUT_DIRECTORY ${cxx_binary_dir})

            IF("${cxx_path}" STREQUAL "")
                SET(cxx_path ${cxx_ldpath})
            ENDIF()
        ENDIF()
    ENDWHILE()

    SET_CORE("${cxx_binary_dir}")
    ADD_TEST(Cxx${is_core}_${script} ${CMAKE_COMMAND}
            -D "EXECUTABLE='./${script}'"
            -D "ARGUMENTS=${arguments}"
            -D "LDPATH=${cxx_ldpath}"
            -D "PATH=${cxx_path}"
            -D "SEPARATOR=${sep}"
            -P "${cxx_binary_dir}/TestDriverCxx.cmake"
    )
    IF(NOT "${tdep}" STREQUAL "")
            SET_TESTS_PROPERTIES(Cxx${is_core}_${script}
            PROPERTIES DEPENDS ${tdep} ${script})
    ENDIF()
    file(COPY
        ${cxx_source_dir}/${script}
        DESTINATION ${cxx_binary_dir}/
    )
ENDMACRO()

# CXX Clean Macro
# Author: Brian Panneton
# Description: This macro sets up the cxx test for a make clean.
# Parameters:         
#		executable      = executable name
#             	${ARGN}         = files that the executable created
MACRO(CLEAN_TEST_CXX executable)
    set_property(DIRECTORY APPEND PROPERTY 
        ADDITIONAL_MAKE_CLEAN_FILES ${ARGN} 
    )
ENDMACRO()

 # Configure the cxx 'driver' file
CONFIGURE_FILE(${TESTING_SUITE_DIR}/TestingSuite/TestDriverCxx.cmake.in ${cxx_binary_dir}/TestDriverCxx.cmake @ONLY)

