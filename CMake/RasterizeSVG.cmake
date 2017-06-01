#
# Simple CMake -P script to rasterize an SVG file to a png image.
#
# Usage:
#
# cmake -DSVGFILE=<SVG file>
#     [ -DPNGFILE=<PNG file> ]
#       -DCONVERTER=<wkhtmltoimage executable>
#       -DREMOVESVG=<bool>
#       -P RasterizeSVG.cmake
#
# SVGFILE is the input SVG file, PNGFILE is the output png file path. If
# PNGFILE is not specified, it will have the same basename as SVGFILE, but with
# a .png extension.
#
# The script calls wkhtmltoimage with the relevant parameters to produce
# a png with the same dimensions as the input file's bounding box. The path to
# the wkhtmltoimage executable for your system must be specified in CONVERTER.
#
# If REMOVESVG is true, the input SVG file will be removed upon successful
# conversion.

function(get_dims_from_svg filename width_var height_var)
  set(${width_var} "" PARENT_SCOPE)
  set(${height_var} "" PARENT_SCOPE)

  # wkhtmltoimage doesn't do a great job of restricting the output image to the
  # input dimensions, so scan the header of the file for the dimensions, which
  # appear as '<svg ... width="XXX" ... height="XXX" ... >' in the root <svg>
  # element. This should appear in the first 1KB, so don't read the whole file:
  file(READ "${filename}" HEADER LIMIT 1024)

  if (NOT HEADER)
    message("The input file is empty: ${filename}")
    return()
  endif()

  string(REGEX REPLACE
    "^.*<svg[^>]+width=\"([^\"]+)\".*$" "\\1"
    WIDTH "${HEADER}")
  string(REGEX REPLACE
    "^.*<svg[^>]+height=\"([^\"]+)\".*$" "\\1"
    HEIGHT "${HEADER}")

  if (NOT WIDTH OR NOT HEIGHT)
    message("Width or height information missing from first KB of input. "
            "Header:\n{HEADER}")
    return()
  endif()

  set(${width_var} "${WIDTH}" PARENT_SCOPE)
  set(${height_var} "${HEIGHT}" PARENT_SCOPE)
endfunction()

if(NOT CONVERTER)
  message(FATAL_ERROR "CONVERTER is not specified!")
endif()

if (NOT SVGFILE)
  message(FATAL_ERROR "SVGFILE not set!")
endif()

get_filename_component(BASENAME "${SVGFILE}" NAME_WE)
get_filename_component(WORKPATH "${SVGFILE}" PATH)

set(WIDTH)
set(HEIGHT)
get_dims_from_svg("${SVGFILE}" WIDTH HEIGHT)

if (NOT WIDTH OR NOT HEIGHT)
  message(FATAL_ERROR "Could not determine input image dimensions.")
endif()

if(NOT PNGFILE)
  set(PNGFILE "${WORKPATH}/${BASENAME}.png")
endif()

# Remove any old output (in case conversion fails)
file(REMOVE "${PNGFILE}")

# Rasterize SVGFILE --> png
execute_process(
  COMMAND "${CONVERTER}"
    --crop-w ${WIDTH} --crop-h ${HEIGHT} "${SVGFILE}" "${PNGFILE}"
  RESULT_VARIABLE EXITCODE
  OUTPUT_VARIABLE ERRORSTR)

  if(NOT ${EXITCODE} EQUAL 0)
    message(FATAL_ERROR "SVG->PNG conversion exited with status ${EXITCODE}:\n"
                        "${ERRORSTR}")
  endif()
endif()

if(REMOVESVG)
  file(REMOVE "${SVGFILE}")
endif()
