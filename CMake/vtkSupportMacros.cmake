macro (vtk_set_with_default var value)
  if (NOT ${var})
    set(${var} "${value}")
  endif ()
endmacro ()

# Bridge an old, deprecated, setting to a new replacement setting.
#
# Use this function when a user-visible flag is being renamed or otherwise
# replaced. If the old value is set, it will be given as the default value,
# otherwise the given default value will be used. This returned value should
# then be used in the ``set(CACHE)`` or ``option()`` call for the new value.
#
# If the old value is set, it will warn that it is deprecated for the new name.
#
# If replacing the setting ``OLD_SETTING`` with ``NEW_SETTING``, its usage
# would look like:
#
#   vtk_deprecated_setting(default_setting NEW_SETTING OLD_SETTING "default value")
#   set(NEW_SETTING "${default_setting}"
#     CACHE STRING "Documentation for the setting.")
function (vtk_deprecated_setting output_default new old intended_default)
  set(default "${intended_default}")
  if (DEFINED "${old}")
    message(WARNING "The '${old}' variable is deprecated for '${new}'.")
    set(default "${${old}}")
  endif ()

  set("${output_default}" "${default}" PARENT_SCOPE)
endfunction ()

# Remove an old / obsolete setting
#
# Use this function when a user-visible flag is being removed entirely. If the
# old value is set, it will be cause a warning message letting the user know
# that the setting has no effect.
function (vtk_obsolete_setting old)
  if (DEFINED "${old}")
    message(WARNING "The '${old}' variable is obsolete and no longer has any effect.")
  endif ()
endfunction ()
