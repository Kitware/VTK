#
# Simple CMake -P script to rasterize a postscript file and optionally
# a PDF file to a png image.
#
# Usage:
#
# cmake -DPSFILE=<postscript file>
#     [ -DPNGFILE=<png file> ]
#       -DGS_EXECUTABLE=<ghostscript executable>
#       -DREMOVEPS=<bool>
#     [ -DRASTERIZE_PDF=<bool> ]
#     [ -DPDFFILE=<pdf file> ]
#     [ -DPDFPNGFILE=<png_pdf_file> ]
#     [ -DREMOVEPDF=<bool> ]
#       -P RasterizePostScript.cmake
#
# PSFILE is the input postscript file, PNGFILE is the output png file path. If
# PNGFILE is not specified, it will have the same basename as PSFILE, but with
# a .png extension.
#
# if RASTERIZE_PDF is not specified is assumed false.  If the PDFFILE
# is not specified, it will have the same basename as PSFILE, but with
# a .pdf extension. The same, PDFPNGFILE, if not specified, is created
# from basename of PNGFILE like this: <pngbasename>-pdf.png. If
# REMOVEPDF is not specified is the same as REMOVEPS.
#
# If PDFFILE is specified but PSFILE is not, only the PDF file will be tested.
# RASTERIZE_PDF is implied in this situation.
#
# The script simple calls ghostscript with the relevant parameters to produce a
# png with the same dimensions as the input file's bounding box. The path to
# the ghostscript executable for your system must be specified in
# GS_EXECUTABLE.
#
# If REMOVEPS is true, the postscript file will be removed upon successful
# conversion.

function(get_bbox_from_gs filename bbox_var)
  set(${bbox_var} "" PARENT_SCOPE)

  # First just try scanning the file for postscript bbox metadata:
  file(READ "${filename}" BBOXOUT)

  if (NOT BBOXOUT)
    message("The input file is empty: ${filename}")
    return()
  endif()

  string(REGEX MATCH
    "%%BoundingBox:[ ]+[0-9-]+[ ]+[0-9-]+[ ]+[0-9]+[ ]+[0-9]+"
    BBOX "${BBOXOUT}")

  # If the metadata isn't provided, ask ghostscript to find out.
  # Beware, GhostScript computes a tight bbox and treats white pixels as
  # transparent, so the gs bbox is dependent on the contents of the image.
  if(NOT BBOX)
    message("No '%%BoundingBox <x> <y> <w> <h>' header found. Asking ghostscript...")

    execute_process(COMMAND
      "${GS_EXECUTABLE}" -sSAFER -sBATCH -sNOPAUSE -sDEVICE=bbox "${filename}"
      RESULT_VARIABLE EXITCODE
      ERROR_VARIABLE BBOXOUT
    )

    if(NOT ${EXITCODE} EQUAL 0)
      message(FATAL_ERROR "GhostScript exited with status ${EXITCODE}:\n${BBOXOUT}")
    endif()

    string(REGEX MATCH "%%BoundingBox:[ ]+[0-9-]+[ ]+[0-9-]+[ ]+[0-9]+[ ]+[0-9]+"
      BBOX "${BBOXOUT}")

    if(NOT BBOX)
      message(FATAL_ERROR
        "Ghostscript could not determine bounding box\nOutput:\n${BBOXOUT}")
    endif()
  endif()

  string(REGEX REPLACE
    "^%%BoundingBox:[ ]+[0-9-]+[ ]+[0-9-]+[ ]+([0-9]+)[ ]+([0-9]+)"
    "\\1x\\2" BBOX "${BBOX}")

  set(${bbox_var} "${BBOX}" PARENT_SCOPE)
endfunction()

if(NOT GS_EXECUTABLE)
  message(FATAL_ERROR "GS_EXECUTABLE is not specified!")
endif()

set(BBOX)
if (PSFILE)
  get_filename_component(BASENAME "${PSFILE}" NAME_WE)
  get_filename_component(WORKPATH "${PSFILE}" PATH)
  get_bbox_from_gs("${PSFILE}" BBOX)
elseif(PDFFILE)
  set(RASTERIZE_PDF TRUE)
  get_filename_component(BASENAME "${PDFFILE}" NAME_WE)
  get_filename_component(WORKPATH "${PDFFILE}" PATH)
  get_bbox_from_gs("${PDFFILE}" BBOX)
else()
  message(FATAL_ERROR "Neither PSFILE nor PDFFILE is specified!")
endif()

if (NOT BBOX)
  message(FATAL_ERROR "Could not determine bounding box of input.")
endif()

if(NOT PNGFILE)
  set(PNGFILE "${WORKPATH}/${BASENAME}.png")
endif()

if(RASTERIZE_PDF AND NOT PDFFILE)
  set(PDFFILE "${WORKPATH}/${BASENAME}.pdf")
endif()

if(RASTERIZE_PDF AND NOT PDFPNGFILE)
  set(PDFPNGFILE "${WORKPATH}/${BASENAME}-pdf.png")
endif()

# Remove any old output (in case conversion fails)
file(REMOVE "${PNGFILE}")
if (RASTERIZE_PDF)
  file(REMOVE "${PDFPNGFILE}")
endif()

# Rasterize PSFILE --> png
if (PSFILE)
  execute_process(
    COMMAND "${GS_EXECUTABLE}"
      -sSAFER -sBATCH -sNOPAUSE -sDEVICE=png16m "-sOutputFile=${PNGFILE}"
      "-g${BBOX}" "${PSFILE}"
    RESULT_VARIABLE EXITCODE OUTPUT_VARIABLE ERRORSTR)

  if(NOT ${EXITCODE} EQUAL 0)
    message(FATAL_ERROR "GhostScript exited with status ${EXITCODE}:\n${ERRORSTR}")
  endif()
endif()

if (RASTERIZE_PDF AND PDFFILE)
  execute_process(
    COMMAND "${GS_EXECUTABLE}"
    -sSAFER -sBATCH -sNOPAUSE -sDEVICE=png16m "-sOutputFile=${PDFPNGFILE}"
    "-g${BBOX}" "${PDFFILE}"
    RESULT_VARIABLE EXITCODE OUTPUT_VARIABLE ERRORSTR)

  if(NOT ${EXITCODE} EQUAL 0)
    message(FATAL_ERROR "GhostScript exited with status ${EXITCODE}:\n${ERRORSTR}")
  endif()
endif()

if(REMOVEPS)
  file(REMOVE "${PSFILE}")
endif()

if (NOT REMOVEPDF)
  set(REMOVEPDF REMOVEPS)
endif()

if(REMOVEPDF)
  file(REMOVE "${PDFFILE}")
endif()
