# Tests with test images in Baseline/Graphics
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    EnSight6Elements
    EnSight6OfficeBin
    EnSightBlow1ASCII
    EnSightBlow1Bin
    EnSightBlow2ASCII
    EnSightBlow2Bin
    EnSightBlow3Bin
    EnSightBlow4Bin
    EnSightBlow5ASCII
    EnSightCompound
    EnSightGoldElements
    EnSightIronProtASCII
    EnSightIronProtBin
    EnSightMandelbrot
    EnSightNfacedASCII
    EnSightNfacedBin
    EnSightOfficeASCII
    EnSightOfficeBin
    EnSightRectGridASCII
    EnSightRectGridBin
    EnSightSelectArrays
    nacaBinary
  )
endif()

foreach( tfile ${tests})
  vtk_add_test_tcl(${tfile}.tcl)
endforeach()
