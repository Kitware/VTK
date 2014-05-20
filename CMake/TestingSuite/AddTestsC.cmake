INCLUDE(TestingSetup)

# We should have one place that points to the c source directory and the c
# binary directory
SET(c_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
SET(c_binary_dir ${CMAKE_CURRENT_BINARY_DIR})

# C Add Dependencies Macro
# Author: Brian Panneton
# Description: This macro adds the c test dependencies.
# 	  Note: The tests already depend on their own file
# Parameters:         
#              dependencies         = any dependencies needed for c tests
MACRO(ADD_TEST_C_DEPENDENCIES dependencies)
    IF(NOT ("${dependencies}" STREQUAL ""))
        SET_PROPERTY(GLOBAL APPEND PROPERTY C_TEST_DEPENDENCIES
            "${dependencies}"
 	)
    ENDIF(NOT ("${dependencies}" STREQUAL ""))
ENDMACRO(ADD_TEST_C_DEPENDENCIES dependencies)

# C Add LDPath  Macro
# Author: Brian Panneton
# Description: This macro adds the c test ldpaths.
# Parameters:         
#               ld  = any ldpaths needed for c tests
MACRO(ADD_TEST_C_LDPATH ld)
    GET_PROPERTY(ldpath GLOBAL PROPERTY C_TEST_LDPATH)
    IF("${ld}" STRGREATER "")
        SET_PROPERTY(GLOBAL PROPERTY C_TEST_LDPATH 
                "${ldpath}${sep}${ld}" 
        )
    ENDIF("${ld}" STRGREATER "")
ENDMACRO(ADD_TEST_C_LDPATH ld)

# C Add Path  Macro
# Author: Brian Panneton
# Description: This macro adds the c test paths.
# Parameters:         
#               p  = any paths needed for c tests
MACRO(ADD_TEST_C_PATH p)
    GET_PROPERTY(path GLOBAL PROPERTY C_TEST_PATH)
    IF("${p}" STRGREATER "")
        SET_PROPERTY(GLOBAL PROPERTY C_TEST_PATH 
                "${path}${sep}${p}" 
        )
    ENDIF("${p}" STRGREATER "")
ENDMACRO(ADD_TEST_C_PATH p)

# C Test Macro
# Author: Brian Panneton
# Description: This macro builds and add the c test in one shot.
# Parameters:         
#		executable      = executable name
#		dup		= duplicate number
#		tdep		= test dependency (Full Test Name with Prefix)
#             	${ARGN}         = any arguments for the executable
MACRO(ADD_TEST_C executable)
    PARSE_TEST_ARGS("${ARGN}")
    
    IF(EXISTS ${c_source_dir}/${executable}.c)
        ADD_EXECUTABLE(${executable}${dup} ${c_source_dir}/${executable}.c)
    ENDIF(EXISTS ${c_source_dir}/${executable}.c)

    GET_PROPERTY(c_dependencies GLOBAL PROPERTY C_TEST_DEPENDENCIES)
    GET_PROPERTY(c_ldpath GLOBAL PROPERTY C_TEST_LDPATH)
    GET_PROPERTY(c_path GLOBAL PROPERTY C_TEST_PATH)
    TARGET_LINK_LIBRARIES(${executable}${dup} ${c_dependencies})
 
    # Take care of windowisims
    IF(WIN32)
        SET_TARGET_PROPERTIES(${executable}${dup} PROPERTIES 
            PREFIX ../
            RUNTIME_OUTPUT_DIRECTORY ${c_binary_dir}
            LIBRARY_OUTPUT_DIRECTORY ${c_binary_dir}
            ARCHIVE_OUTPUT_DIRECTORY ${c_binary_dir})

        IF("${c_path}" STREQUAL "")
            SET(c_path ${c_ldpath})
        ENDIF("${c_path}" STREQUAL "")
    ENDIF(WIN32)

    SET_CORE("${c_binary_dir}")
    ADD_TEST(C${is_core}_${executable}${dup} ${CMAKE_COMMAND}
            -D "EXECUTABLE=${executable}${dup}"
            -D "ARGUMENTS=${arguments}"
            -D "LDPATH=${c_ldpath}"
            -D "PATH=${c_path}"
            -D "SEPARATOR=${sep}"
            -P "${c_binary_dir}/TestDriverC.cmake"
    )
    IF(NOT "${tdep}" STREQUAL "")
	    SET_TESTS_PROPERTIES(C${is_core}_${executable}${dup}
            PROPERTIES DEPENDS ${tdep})
    ENDIF(NOT "${tdep}" STREQUAL "")
ENDMACRO(ADD_TEST_C executable)

# C MPI Test Macro
# Author: Andrew Burns
# Description: This macro builds and adds a script to execute MPI tests.
# Parameters:
#               executable      = script name
#               files           = code to be compiled and executed by the script
#               tdep            = test dependency (Full Test Name with Prefix)
#               ${ARGN}         = any arguments for the executable
MACRO(ADD_MPI_TEST_C script files)
    PARSE_TEST_ARGS("${ARGN}")

    set(tempfiles ${files})

    WHILE(NOT "${tempfiles}" STREQUAL "")
        # ${executable}
        STRING(REGEX MATCH "([^ ,])+,|([^ ,])+" executable "${tempfiles}")
        STRING(REGEX REPLACE "," "" executable "${executable}")
        STRING(REGEX REPLACE "${executable},|${executable}" "" trimmed "${tempfiles}")

        set(tempfiles ${trimmed})

        IF(EXISTS ${c_source_dir}/${executable}.c)
            ADD_EXECUTABLE(${executable} ${c_source_dir}/${executable}.c)
        ENDIF(EXISTS ${c_source_dir}/${executable}.c)

        GET_PROPERTY(c_dependencies GLOBAL PROPERTY C_TEST_DEPENDENCIES)
        GET_PROPERTY(c_ldpath GLOBAL PROPERTY C_TEST_LDPATH)
        GET_PROPERTY(c_path GLOBAL PROPERTY C_TEST_PATH)
        TARGET_LINK_LIBRARIES(${executable} ${c_dependencies})

        # Take care of windowisims
        IF(WIN32)
            SET_TARGET_PROPERTIES(${executable} PROPERTIES
                PREFIX ../
                RUNTIME_OUTPUT_DIRECTORY ${c_binary_dir}
                LIBRARY_OUTPUT_DIRECTORY ${c_binary_dir}
                ARCHIVE_OUTPUT_DIRECTORY ${c_binary_dir})

            IF("${c_path}" STREQUAL "")
                SET(c_path ${c_ldpath})
            ENDIF("${c_path}" STREQUAL "")
        ENDIF(WIN32)
    ENDWHILE(NOT "${tempfiles}" STREQUAL "")

    SET_CORE("${c_binary_dir}")
    ADD_TEST(C${is_core}_${script} ${CMAKE_COMMAND}
            -D "EXECUTABLE='./${script}'"
            -D "ARGUMENTS=${arguments}"
            -D "LDPATH=${c_ldpath}"
            -D "PATH=${c_path}"
            -D "SEPARATOR=${sep}"
            -P "${c_binary_dir}/TestDriverC.cmake"
    )
    IF(NOT "${tdep}" STREQUAL "")
            SET_TESTS_PROPERTIES(C${is_core}_${script}
            PROPERTIES DEPENDS ${tdep} ${script})
    ENDIF(NOT "${tdep}" STREQUAL "")
    file(COPY
        ${c_source_dir}/${script}
        DESTINATION ${c_binary_dir}/
    )
ENDMACRO(ADD_MPI_TEST_C script files)

# C Clean Macro
# Author: Brian Panneton
# Description: This macro sets up the c test for a make clean.
# Parameters:         
#		executable      = executable name
#             	${ARGN}         = files that the executable created
MACRO(CLEAN_TEST_C executable)
    set_property(DIRECTORY APPEND PROPERTY 
        ADDITIONAL_MAKE_CLEAN_FILES ${ARGN} 
    )
ENDMACRO(CLEAN_TEST_C executable)

 # Configure the c 'driver' file
CONFIGURE_FILE(${TESTING_SUITE_DIR}/TestingSuite/TestDriverC.cmake.in ${c_binary_dir}/TestDriverC.cmake @ONLY)

