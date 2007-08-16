# Macro to provide an option only if a set of other variables are ON.
# Example invocation:
#
#  VTK_DEPENDENT_OPTION(USE_FOO "Use Foo" ON "USE_BAR;USE_ZOT" OFF)
#
# If both USE_BAR and USE_ZOT are true, this provides an option called
# USE_FOO that defaults to ON.  Otherwise, it sets USE_FOO to OFF.  If
# the status of USE_BAR or USE_ZOT ever changes, any value for the
# USE_FOO option is saved so that when the option is re-enabled it
# retains its old value.
#
MACRO(VTK_DEPENDENT_OPTION option doc default depends force)
  IF(${option}_ISSET MATCHES "^${option}_ISSET$")
    SET(${option}_AVAILABLE 1)
    FOREACH(d ${depends})
      IF(NOT ${d})
        SET(${option}_AVAILABLE 0)
      ENDIF(NOT ${d})
    ENDFOREACH(d)

    IF(${option}_AVAILABLE)
      OPTION(${option} "${doc}" "${default}")
      SET(${option} "${${option}}" CACHE BOOL "${doc}" FORCE)
    ELSE(${option}_AVAILABLE)
      IF(NOT ${option} MATCHES "^${option}$")
        SET(${option} "${${option}}" CACHE INTERNAL "${doc}")
      ENDIF(NOT ${option} MATCHES "^${option}$")
      SET(${option} ${force})
    ENDIF(${option}_AVAILABLE)

  ELSE(${option}_ISSET MATCHES "^${option}_ISSET$")
    SET(${option} "${${option}_ISSET}")
  ENDIF(${option}_ISSET MATCHES "^${option}_ISSET$")
ENDMACRO(VTK_DEPENDENT_OPTION)
