 
SET(VPIC_SOURCES
  VPICDataSet.cxx
  VPICDefinition.cxx
  VPICGlobal.cxx
  VPICHeader.cxx
  VPICPart.cxx
  VPICView.cxx
)

ADD_LIBRARY(VPIC ${VPIC_SOURCES})
TARGET_LINK_LIBRARIES(VPIC KWCommon)
