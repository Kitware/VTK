cmake_policy(SET CMP0057 NEW)

if (NOT EXISTS "${CMAKE_BINARY_DIR}/install_manifest.txt")
  message(FATAL_ERROR
    "Could not find the install manifest.")
endif ()

set(directories)
file(STRINGS "${CMAKE_BINARY_DIR}/install_manifest.txt" installed_files)
foreach (installed_file IN LISTS installed_files)
  set(full_file "$ENV{DESTDIR}${installed_file}")
  message(STATUS "Uninstalling: ${full_file}")
  if (IS_SYMLINK "${full_file}" OR EXISTS "${full_file}")
    file(REMOVE "${full_file}")
  else ()
    message(STATUS "File ${full_file} not found.")
  endif ()
  get_filename_component(directory "${full_file}" DIRECTORY)
  list(APPEND directories "${directory}")
endforeach ()

if (directories)
  list(REMOVE_DUPLICATES directories)
  # Iterate from the bottom up to not have to restart all the time.
  list(SORT directories)
  list(REVERSE directories)
endif ()

while (directories)
  list(GET directories 0 directory)
  list(REMOVE_AT directories 0)

  file(GLOB dirents "${directory}/*" LIST_DIRECTORIES 1)
  if (NOT dirents)
    message(STATUS "Uninstalling: ${directory}")
    file(REMOVE_RECURSE "${directory}")

    get_filename_component(parent "${directory}" DIRECTORY)
    if (NOT parent IN_LIST directories)
      list(INSERT directories 0 "${parent}")
    endif ()
  endif ()
endwhile ()
