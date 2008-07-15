CMAKE_MINIMUM_REQUIRED (VERSION 2.0)
PROJECT(PAPER NONE)

SET(CMAKE_C_FLAGS "")
SET(CMAKE_C_FLAGS_RELEASE "")
SET(CMAKE_C_FLAGS_MINSIZEREL "")
SET(CMAKE_C_FLAGS_DEBUG "")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO "")

# Document
SET(DOCUMENT Verdict-Manual-2007)
SET(LITERATURE Verdict-Manual-2007)
SET(SECTIONS
  VerdictMathMacros.tex
  00-Introduction.tex
  10-Instructions.tex
  API.tex
  Hex.tex
  HexDiagonal.tex
  HexDimension.tex
  HexDistortion.tex
  HexEdgeRatio.tex
  HexJacobian.tex
  HexMaximumEdgeRatio.tex
  HexMaxAspectFrobenius.tex
  HexMedAspectFrobenius.tex
  HexRelativeSizeSquared.tex
  HexScaledJacobian.tex
  HexShape.tex
  HexShapeAndSize.tex
  HexShear.tex
  HexShearAndSize.tex
  HexSkew.tex
  HexStretch.tex
  HexTaper.tex
  HexVolume.tex
  HexOddy.tex
  Quad.tex
  QuadArea.tex
  QuadAspectRatio.tex
  QuadEdgeRatio.tex
  QuadRadiusRatio.tex
  QuadMedAspectFrobenius.tex
  QuadMaxAspectFrobenius.tex
  QuadCondition.tex
  QuadDistortion.tex
  QuadEdgeRatio.tex
  QuadJacobian.tex
  QuadMaxAspectFrobenius.tex
  QuadMaximumAngle.tex
  QuadMaximumEdgeRatio.tex
  QuadMedAspectFrobenius.tex
  QuadMinimumAngle.tex
  QuadOddy.tex
  QuadRadiusRatio.tex
  QuadRelativeSizeSquared.tex
  QuadScaledJacobian.tex
  QuadShapeAndSize.tex
  QuadShape.tex
  QuadShearAndSize.tex
  QuadShear.tex
  QuadSkew.tex
  QuadStretch.tex
  QuadTaper.tex
  QuadWarpage.tex
  Tet.tex
  TetEdgeRatio.tex
  TetAspectBeta.tex
  TetAspectDelta.tex
  TetAspectFrobenius.tex
  TetAspectGamma.tex
  TetAspectRatio.tex
  TetCollapseRatio.tex
  TetCondition.tex
  TetDistortion.tex
  TetJacobian.tex
  TetMinimumAngle.tex
  TetRadiusRatio.tex
  TetRelativeSizeSquared.tex
  TetScaledJacobian.tex
  TetShapeAndSize.tex
  TetShape.tex
  TetVolume.tex
  Tri.tex
  TriArea.tex
  TriAspectFrobenius.tex
  TriAspectRatio.tex
  TriCondition.tex
  TriDistortion.tex
  TriEdgeRatio.tex
  TriMaximumAngle.tex
  TriMinimumAngle.tex
  TriRadiusRatio.tex
  TriRelativeSizeSquared.tex
  TriScaledJacobian.tex
  TriShapeAndSize.tex
  TriShape.tex
  OtherEl.tex
  PyrVolume.tex
  WedVolume.tex
  KniVolume.tex
  Verdict-Manual-2007.tex
  )

SET(SVG_FILES
  svg/hex.svg
  svg/quad.svg
  svg/quad-axes.svg
  svg/quad-areas.svg
  svg/quad-vertex-areas.svg
  svg/tet.svg
  svg/tet-height.svg
  svg/tri.svg
  svg/pyramid.svg
  svg/wedge.svg
  svg/knife.svg
)

SET(PNG_FILES
  png/quality-streamingTess.png
  png/tri4qualVR-bq2.png
)

FIND_PACKAGE(LATEX REQUIRED)

set( CMAKE_FIND_APPBUNDLE NEVER )
FIND_PROGRAM(INKSCAPE
  NAMES inkscape
  PATHS
    /usr/local/bin
    /usr/bin
    /Applications/Inkscape.app/Contents/Resources/bin
)

SET(SOURCETEX ${CMAKE_CURRENT_SOURCE_DIR}/${DOCUMENT}.tex)
SET(DESTTEX ${CMAKE_CURRENT_BINARY_DIR}/${DOCUMENT}.tex)

CONFIGURE_FILE("${SOURCETEX}" "${DESTTEX}" @ONLY IMMEDIATE)

SET(depends_files)
FOREACH(file "${LITERATURE}.bib" ${PNG_FILES}  ${SVG_FILES} ${SECTIONS})
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${file}"
    "${CMAKE_CURRENT_BINARY_DIR}/${file}" COPYONLY IMMEDIATE)
  SET(depends_files ${depends_files} "${CMAKE_CURRENT_BINARY_DIR}/${file}")
ENDFOREACH(file)
FOREACH ( SVG_FILE ${SVG_FILES} )
  STRING( REGEX REPLACE "svg" "png" PNG_FILE "${SVG_FILE}" )
  ADD_CUSTOM_COMMAND(
    OUTPUT ${PAPER_BINARY_DIR}/${PNG_FILE}
    COMMAND ${INKSCAPE}
    ARGS -D -d 300 --export-png="${PAPER_BINARY_DIR}/${PNG_FILE}" "${PAPER_SOURCE_DIR}/${SVG_FILE}"
    DEPENDS ${PAPER_SOURCE_DIR}/${SVG_FILE}
    COMMENT "Generating ${PNG_FILE}"
  )
  SET( depends_files ${depends_files} "${PAPER_BINARY_DIR}/${PNG_FILE}" )
ENDFOREACH ( SVG_FILE )

SET(BIBDEP ${PAPER_BINARY_DIR}/${DOCUMENT}.bbl)

ADD_CUSTOM_TARGET(LaTeXDocument ALL echo
  DEPENDS   ${PAPER_BINARY_DIR}/${DOCUMENT}.pdf
  )

IF(WIN32)
  SET(E "#")
  SET(COMMAND_SEP "&")
ELSE(WIN32)
  SET(E "=")
  SET(COMMAND_SEP "\;")
ENDIF(WIN32)

SET(PS2PDF_OPTIONSe
  -sPAPERSIZE${E}letter -dPDFSETTINGS${E}/prepress -dCompatibilityLevel${E}1.3 -dAutoFilterColorImages${E}false -dAutoFilterGrayImages${E}false -dColorImageFilter${E}/FlateEncode -dGrayImageFilter${E}/FlateEncode -dMonoImageFilter${E}/FlateEncode -dDownsampleGrayImages${E}false -dDownsampleColorImages${E}false
  )

ADD_CUSTOM_COMMAND(
  OUTPUT ${PAPER_BINARY_DIR}/${DOCUMENT}.pdf
  COMMAND ${PDFLATEX_COMPILER} ${DESTTEX} ${COMMAND_SEP} ${PDFLATEX_COMPILER} ${DESTTEX}
  DEPENDS ${DESTTEX} ${depends_files} ${PAPER_BINARY_DIR}/${DOCUMENT}.bbl
  COMMENT "Final PDF")

ADD_CUSTOM_COMMAND(
  OUTPUT ${PAPER_BINARY_DIR}/${DOCUMENT}.aux
  COMMAND ${PDFLATEX_COMPILER}
  ARGS ${DESTTEX}
  DEPENDS ${DESTTEX} ${depends_files}
  COMMENT "Generate auxiliary data")

ADD_CUSTOM_COMMAND(
  OUTPUT ${PAPER_BINARY_DIR}/${DOCUMENT}.bbl
  COMMAND ${BIBTEX_COMPILER}
  ARGS ${PAPER_BINARY_DIR}/${DOCUMENT}
  DEPENDS ${PAPER_BINARY_DIR}/${DOCUMENT}.aux
  COMMENT "Generate bibliography")

SET(clean_files 
  ${PAPER_BINARY_DIR}/${DOCUMENT}.log 
  ${PAPER_BINARY_DIR}/${DOCUMENT}.aux 
  ${PAPER_BINARY_DIR}/${DOCUMENT}.bbl 
  ${PAPER_BINARY_DIR}/${DOCUMENT}.blg 
  ${PAPER_BINARY_DIR}/${DOCUMENT}.dvi 
  ${PAPER_BINARY_DIR}/${DOCUMENT}.idx 
  ${PAPER_BINARY_DIR}/${DOCUMENT}.lof 
  ${PAPER_BINARY_DIR}/${DOCUMENT}.lot 
  ${PAPER_BINARY_DIR}/${DOCUMENT}.toc 
  ${PAPER_BINARY_DIR}/${DOCUMENT}.ind 
  ${PAPER_BINARY_DIR}/${DOCUMENT}.ilg )

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${clean_files}")

# .aux:
# .dvi
# .idx: index
# .lof: list of figures
# .log: latex log file
# .lot: list of tables
# .toc: table of contents

# .ind: makeindex output
# .ilg: makeindex log file

# .bib:
# .bbl: bibtex output file
# .blg: bibtex log file

# tetex env var for cls path: TEXINPUTS
