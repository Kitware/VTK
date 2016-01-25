#
# Simple CMake -P script to rasterize a postscript file to a png image.
#
# Usage:
#
# cmake -DPSFILE=<postscript file>
#     [ -DPNGFILE=<png file> ]
#       -DGS_EXECUTABLE=<ghostscript executable>
#       -DREMOVEPS=<bool>
#       -P RasterizePostScript.cmake
#
# PSFILE is the input postscript file, PNGFILE is the output png file path. If
# PNGFILE is not specified, it will have the same basename as PSFILE, but with
# a .png extension.
#
# The script simple calls ghostscript with the relevant parameters to produce
# a 500x500 png. The path to the ghostscript executable for your system must be
# specified in GS_EXECUTABLE.
#
# If REMOVEPS is true, the postscript file will be removed upon successful
# conversion.

if(NOT PSFILE)
  message(FATAL_ERROR "PSFILE is not specified!")
endif()

if(NOT GS_EXECUTABLE)
  message(FATAL_ERROR "GS_EXECUTABLE is not specified!")
endif()

if(NOT PNGFILE)
  get_filename_component(BASENAME "${PSFILE}" NAME_WE)
  get_filename_component(PSPATH "${PSFILE}" PATH)
  set(PNGFILE "${PSPATH}/${BASENAME}.png")
endif()

# Remove any old output (in case conversion fails)
file(REMOVE "${PNGFILE}")

# Get the bounding box from the file metadata (always added by GL2PS)
file(READ "${PSFILE}" BBOXOUT)

if (NOT BBOXOUT)
  message(FATAL_ERROR "The input file is empty: ${PSFILE}")
endif()

string(REGEX MATCH "%%BoundingBox:[ ]+[0-9-]+[ ]+[0-9-]+[ ]+[0-9]+[ ]+[0-9]+"
  BBOX "${BBOXOUT}")

# If the metadata isn't provided, ask ghostscript to find out.
# Beware, GhostScript computes a tight bbox and treats white pixels as
# transparent, so the gs bbox is dependent on the contents of the image.
if(NOT BBOX)
  message("No '%%BoundingBox <x> <y> <w> <h>' header found. Asking ghostscript...")

  execute_process(COMMAND
    "${GS_EXECUTABLE}" -sSAFER -sBATCH -sNOPAUSE -sDEVICE=bbox "${PSFILE}"
    RESULT_VARIABLE EXITCODE
    ERROR_VARIABLE BBOXOUT
  )

  if(NOT ${EXITCODE} EQUAL 0)
    message(FATAL_ERROR "GhostScript exited with status ${EXITCODE}:\n${BBOXOUT}")
  endif()

  string(REGEX MATCH "%%BoundingBox:[ ]+[0-9-]+[ ]+[0-9-]+[ ]+[0-9]+[ ]+[0-9]+"
    BBOX "${BBOXOUT}")

  if(NOT BBOX)
    message("Ghostscript couldn't figure it out either :(\nOutput:\n${BBOXOUT}")
  endif()
endif()

string(REGEX REPLACE
  "^%%BoundingBox:[ ]+[0-9-]+[ ]+[0-9-]+[ ]+([0-9]+)[ ]+([0-9]+)"
  "\\1x\\2" BBOX "${BBOX}")

execute_process(
  COMMAND "${GS_EXECUTABLE}"
    -sSAFER -sBATCH -sNOPAUSE -sDEVICE=png16m "-sOutputFile=${PNGFILE}"
    "-g${BBOX}" "${PSFILE}"
  RESULT_VARIABLE EXITCODE OUTPUT_VARIABLE ERRORSTR)

if(NOT ${EXITCODE} EQUAL 0)
  message(FATAL_ERROR "GhostScript exited with status ${EXITCODE}:\n${ERRORSTR}")
endif()

if(REMOVEPS)
  file(REMOVE "${PSFILE}")
endif()
