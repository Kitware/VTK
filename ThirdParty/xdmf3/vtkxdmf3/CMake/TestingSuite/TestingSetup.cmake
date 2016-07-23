# List of global variables needed by tests
SET(defines 
    JAVA_TEST_DEPENDENCIES
    JAVA_TEST_FILE_DEPENDENCIES
    JAVA_TEST_CLASSPATH
    JAVA_TEST_LDPATH
    JAVA_TEST_PATH
    JAVA_TEST_TARGETS
    PYTHON_TEST_DEPENDENCIES
    PYTHON_TEST_FILE_DEPENDENCIES
    PYTHON_TEST_PYTHONPATH
    PYTHON_TEST_LDPATH
    PYTHON_TEST_PATH
    CXX_TEST_DEPENDENCIES
    CXX_TEST_LDPATH
    CXX_TEST_PATH
)

# Take care of Windows Path Seperators
IF(WIN32)
    SET(sep ";")
ELSE()
    SET(sep ":")
ENDIF()

# Make sure they are defined
FOREACH(def IN LISTS defines)
    GET_PROPERTY(is_defined GLOBAL PROPERTY ${def} DEFINED)
    IF(NOT is_defined)
        DEFINE_PROPERTY(GLOBAL PROPERTY ${def}
			BRIEF_DOCS "${def}"
			FULL_DOCS "${def} No Documentation"
        )
    ENDIF()
ENDFOREACH()

# Set the testing suite dir
SET(TESTING_SUITE_DIR "${CMAKE_SOURCE_DIR}/CMake" CACHE PATH 
    "Testing Suite CMake Dir")
MARK_AS_ADVANCED(TESTING_SUITE_DIR)

# Argument Parsing Macro
# Author: Brian Panneton
# Description: This macro parses the provided argument string and sets the vars
# Parameters:         
#        	${test_args}    = the argument string for the test
# Output:	
#		${arguments}	= whatever is left after stripping the arguments
#		${dup}		= number or string to append to a duplicate test
#		${tdep}		= test dependencies (comma seperated list)
MACRO(PARSE_TEST_ARGS test_args)
    STRING(COMPARE NOTEQUAL "${test_args}" "" check)
    IF(${check})
        SET(arguments "${test_args}")
	# Here we strip out any arguments for the testing suite
        
	# ${dup}
        STRING(REGEX MATCH "dup=([^ ;])+" dup "${test_args}")
        STRING(REGEX REPLACE "dup=" "" dup "${dup}")
        STRING(REGEX REPLACE ";" "" dup "${dup}")
        STRING(REGEX REPLACE "dup=([^ ;])+" "" arguments "${arguments}")
        
	# ${tdep}
        STRING(REGEX MATCH "tdep=([^ ;])+" tdep "${test_args}")
        STRING(REGEX REPLACE "tdep=" "" tdep "${tdep}")
        STRING(REGEX REPLACE ";" "" tdep "${tdep}")
	STRING(REGEX REPLACE "tdep=([^ ;])+" "" arguments "${arguments}")
	STRING(REGEX REPLACE "," ";" tdep "${tdep}")
    ELSE()
        SET(arguments "") # Sanity Check	
    ENDIF()
ENDMACRO()

# Set Core Macro
# Author: Brian Panneton
# Description: This macro checks the directory provided to see if it is a core
# Parameters:         
#               dir    	= the directory to be checked
# Output:     
#		is_core	= variable is set to 'Core' is core was found in dir
MACRO(SET_CORE dir)
    STRING(REGEX MATCH "core" is_core "${dir}")
    IF(EXISTS ${is_core})
        SET(is_core "Core")
    ELSE(EXISTS ${is_core})
      STRING(REGEX MATCH "utils" is_core "${dir}")
      IF(EXISTS ${is_core})
          SET(is_core "Util")
      ENDIF(EXISTS ${is_core})
    ENDIF(EXISTS ${is_core})
ENDMACRO(SET_CORE dir)
