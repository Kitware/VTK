INCLUDE(TestingSetup)

# We should have one place that points to the fortran source directory and the fortran
# binary directory
SET(fortran_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
SET(fortran_binary_dir ${CMAKE_CURRENT_BINARY_DIR})

# Fortran Add Dependencies Macro
# Author: Brian Panneton
# Description: This macro adds the fortran test dependencies.
# 	  Note: The tests already depend on their own file
# Parameters:         
#              dependencies         = any dependencies needed for fortran tests
MACRO(ADD_TEST_FORTRAN_DEPENDENCIES dependencies)
    IF(NOT ("${dependencies}" STREQUAL ""))
        SET_PROPERTY(GLOBAL APPEND PROPERTY FORTRAN_TEST_DEPENDENCIES
            "${dependencies}"
 	)
    ENDIF()
ENDMACRO()

# Fortran Add LDPath  Macro
# Author: Brian Panneton
# Description: This macro adds the fortran test ldpaths.
# Parameters:         
#               ld  = any ldpaths needed for fortran tests
MACRO(ADD_TEST_FORTRAN_LDPATH ld)
    GET_PROPERTY(ldpath GLOBAL PROPERTY FORTRAN_TEST_LDPATH)
    IF("${ld}" STRGREATER "")
        SET_PROPERTY(GLOBAL PROPERTY FORTRAN_TEST_LDPATH 
                "${ldpath}${sep}${ld}" 
        )
    ENDIF()
ENDMACRO()

# Fortran Add Path  Macro
# Author: Brian Panneton
# Description: This macro adds the fortran test paths.
# Parameters:         
#               p  = any paths needed for fortran tests
MACRO(ADD_TEST_FORTRAN_PATH p)
    GET_PROPERTY(path GLOBAL PROPERTY FORTRAN_TEST_PATH)
    IF("${p}" STRGREATER "")
        SET_PROPERTY(GLOBAL PROPERTY FORTRAN_TEST_PATH 
                "${path}${sep}${p}" 
        )
    ENDIF()
ENDMACRO()

# Fortran Test Macro
# Author: Brian Panneton
# Description: This macro builds and add the fortran test in one shot.
# Parameters:         
#		executable      = executable name
#		dup		= duplicate number
#		tdep		= test dependency (Full Test Name with Prefix)
#             	${ARGN}         = any arguments for the executable
MACRO(ADD_TEST_FORTRAN executable)
    PARSE_TEST_ARGS("${ARGN}")

    IF(EXISTS ${fortran_source_dir}/${executable}.f)
        ADD_EXECUTABLE(${executable} ${fortran_source_dir}/${executable}.f)
    ENDIF()

    IF(EXISTS ${fortran_source_dir}/${executable}.f90)
        ADD_EXECUTABLE(${executable} ${fortran_source_dir}/${executable}.f90)
    ENDIF()

    IF(EXISTS ${fortran_source_dir}/${executable}.F90)
        ADD_EXECUTABLE(${executable} ${fortran_source_dir}/${executable}.F90)
    ENDIF()
    
    GET_PROPERTY(fortran_dependencies GLOBAL PROPERTY FORTRAN_TEST_DEPENDENCIES)
    GET_PROPERTY(fortran_ldpath GLOBAL PROPERTY FORTRAN_TEST_LDPATH)
    GET_PROPERTY(fortran_path GLOBAL PROPERTY FORTRAN_TEST_PATH)
    TARGET_LINK_LIBRARIES(${executable}${dup} ${fortran_dependencies})
 
    # Take care of windowisims
    IF(WIN32)
        SET_TARGET_PROPERTIES(${executable}${dup} PROPERTIES 
            PREFIX ../
            RUNTIME_OUTPUT_DIRECTORY ${fortran_binary_dir}
            LIBRARY_OUTPUT_DIRECTORY ${fortran_binary_dir}
            ARCHIVE_OUTPUT_DIRECTORY ${fortran_binary_dir})

        IF("${fortran_path}" STREQUAL "")
            SET(fortran_path ${fortran_ldpath})
        ENDIF()
    ENDIF()

    SET_CORE("${fortran_binary_dir}")
    ADD_TEST(FORTRAN${is_core}_${executable}${dup} ${CMAKE_COMMAND}
            -D "EXECUTABLE=${executable}${dup}"
            -D "ARGUMENTS=${arguments}"
            -D "LDPATH=${fortran_ldpath}"
            -D "PATH=${fortran_path}"
            -D "SEPARATOR=${sep}"
            -P "${fortran_binary_dir}/TestDriverFortran.cmake"
    )
    IF(NOT "${tdep}" STREQUAL "")
	    SET_TESTS_PROPERTIES(FORTRAN${is_core}_${executable}${dup}
            PROPERTIES DEPENDS ${tdep})
    ENDIF()
ENDMACRO()

# Fortran MPI Test Macro
# Author: Andrew Burns
# Description: This macro builds and adds a script to execute MPI tests.
# Parameters:
#               executable      = script name
#               files           = code to be compiled and executed by the script
#               tdep            = test dependency (Full Test Name with Prefix)
#               ${ARGN}         = any arguments for the executable
MACRO(ADD_MPI_TEST_FORTRAN script files)
    PARSE_TEST_ARGS("${ARGN}")

    set(tempfiles ${files})

    WHILE(NOT "${tempfiles}" STREQUAL "")
        # ${executable}
        STRING(REGEX MATCH "([^ ,])+,|([^ ,])+" executable "${tempfiles}")
        STRING(REGEX REPLACE "," "" executable "${executable}")
        STRING(REGEX REPLACE "${executable},|${executable}" "" trimmed "${tempfiles}")

        set(tempfiles ${trimmed})

        IF(EXISTS ${fortran_source_dir}/${executable}.f)
            ADD_EXECUTABLE(${executable} ${fortran_source_dir}/${executable}.f)
        ENDIF()

        IF(EXISTS ${fortran_source_dir}/${executable}.f90)
            ADD_EXECUTABLE(${executable} ${fortran_source_dir}/${executable}.f90)
        ENDIF()

        IF(EXISTS ${fortran_source_dir}/${executable}.F90)
            ADD_EXECUTABLE(${executable} ${fortran_source_dir}/${executable}.F90)
        ENDIF()

        GET_PROPERTY(fortran_dependencies GLOBAL PROPERTY FORTRAN_TEST_DEPENDENCIES)
        GET_PROPERTY(fortran_ldpath GLOBAL PROPERTY FORTRAN_TEST_LDPATH)
        GET_PROPERTY(fortran_path GLOBAL PROPERTY FORTRAN_TEST_PATH)
        TARGET_LINK_LIBRARIES(${executable} ${fortran_dependencies})

        # Take care of windowisims
        IF(WIN32)
            SET_TARGET_PROPERTIES(${executable} PROPERTIES
                PREFIX ../
                RUNTIME_OUTPUT_DIRECTORY ${fortran_binary_dir}
                LIBRARY_OUTPUT_DIRECTORY ${fortran_binary_dir}
                ARCHIVE_OUTPUT_DIRECTORY ${fortran_binary_dir})

            IF("${fortran_path}" STREQUAL "")
                SET(fortran_path ${fortran_ldpath})
            ENDIF()
        ENDIF()
    ENDWHILE()

    SET_CORE("${fortran_binary_dir}")
    ADD_TEST(FORTRAN${is_core}_${script} ${CMAKE_COMMAND}
            -D "EXECUTABLE='./${script}'"
            -D "ARGUMENTS=${arguments}"
            -D "LDPATH=${fortran_ldpath}"
            -D "PATH=${fortran_path}"
            -D "SEPARATOR=${sep}"
            -P "${fortran_binary_dir}/TestDriverFortran.cmake"
    )
    IF(NOT "${tdep}" STREQUAL "")
            SET_TESTS_PROPERTIES(FORTRAN${is_core}_${script}
            PROPERTIES DEPENDS ${tdep} ${script})
    ENDIF()
    file(COPY
        ${fortran_source_dir}/${script}
        DESTINATION ${fortran_binary_dir}/
    )
ENDMACRO()

# Fortran Clean Macro
# Author: Brian Panneton
# Description: This macro sets up the fortran test for a make clean.
# Parameters:         
#		executable      = executable name
#             	${ARGN}         = files that the executable created
MACRO(CLEAN_TEST_FORTRAN executable)
    set_property(DIRECTORY APPEND PROPERTY 
        ADDITIONAL_MAKE_CLEAN_FILES ${ARGN} 
    )
ENDMACRO()

 # Configure the Fortran 'driver' file
CONFIGURE_FILE(${TESTING_SUITE_DIR}/TestingSuite/TestDriverFortran.cmake.in ${fortran_binary_dir}/TestDriverFortran.cmake @ONLY)

