# Version Suite
# Author: Brian Panneton
# Descrition:   This small suite allows you to add support
#               for versioning in your projects

# This allows you to turn on and off the auto
# update of the (project name)Version.hpp file
SET(VERSION_CONTROL_AUTOUPDATE OFF CACHE BOOL "Automaticaly Update The Version")
MARK_AS_ADVANCED(VERSION_CONTROL_AUTOUPDATE)

# We need to make sure we have the header file
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/CMake/VersionSuite)

# Default incase CalculateVerison is not called
SET(vMajor "0")
SET(vMinor "0")

# This Macro allows you to set up the Version in a one liner
MACRO(VersionCreate versionName versionMajor export_name)
    VersionMajorSet(${versionMajor})
    VersionCalculate()
    VersionWrite(${versionName} ${export_name} "${ARGN}")
ENDMACRO()

# This Macro allows you to set the rewrite number
MACRO(VersionMajorSet versionMajor)
        SET(vMajor ${versionMajor})
ENDMACRO()

# This Macro calculates the number of tags from your git repo
MACRO(VersionCalculate)
    FIND_PACKAGE(Git)
    IF(GIT_FOUND)
        EXEC_PROGRAM(${GIT_EXECUTABLE} ${CMAKE_SOURCE_DIR} ARGS tag OUTPUT_VARIABLE return)
        STRING(REGEX REPLACE "\n" ";" return "${return}")
        SET(count 0)
        FOREACH(r ${return})
            MATH(EXPR count "${count} + 1")
        ENDFOREACH()
        SET(vMinor ${count})
    ELSE()
        SET(vMinor "X")
    ENDIF()
ENDMACRO()

# This Macro writes your hpp/cpp files
MACRO(VersionWrite vProjectName export_name)
    SET(include_list "${ARGN}")
    FOREACH(il ${include_list})
        SET(includes "${includes}\n\#include \"${il}\"")
    ENDFOREACH()
    FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${vProjectName}Version.hpp
"/* Current Version of ${vProjectName}
 * Major is: ${vMajor}
 * Minor is: ${vMinor}
 */
${includes}
\#include \"ProjectVersion.hpp\"
extern ${export_name} ProjectVersion ${vProjectName}Version;\n"
    )

        FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${vProjectName}Version.cpp
"/* Current Version of ${vProjectName}
 * Make sure to include this file in your built sources
 */
\#include \"${vProjectName}Version.hpp\"
ProjectVersion ${vProjectName}Version = ProjectVersion(\"${vProjectName}\", \"${vMajor}\", \"${vMinor}\");\n"
        )
ENDMACRO()
