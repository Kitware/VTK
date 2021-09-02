vtk_add_test_python(
  EnSight6Elements.py
  EnSight6ElementsBin.py
  EnSight6OfficeBin.py
  EnSight6TemporalGeometryChange.py,NO_VALID
  EnSightBlow1ASCII.py
  EnSightBlow1Bin.py
  EnSightBlow2ASCII.py
  EnSightBlow2Bin.py
  EnSightBlow3Bin.py
  EnSightBlow4Bin.py
  EnSightBlow5ASCII.py
  EnSightCompound.py
  EnSightGoldAsymTensor.py
  EnSightGoldAsymTensorBin.py
  EnSightGoldElements.py
  EnSightGoldElementsBin.py
  EnSightGoldEmptyParts.py,NO_VALID,NO_RT
  EnSightGoldFortran.py
  EnSightGoldRigidBody.py
  EnSightGoldUndefAndPartialAscii.py,NO_VALID,NO_RT
  EnSightGoldUndefAndPartialBin.py,NO_VALID,NO_RT
  EnSightIronProtASCII.py
  EnSightIronProtBin.py
  EnSightMandelbrot.py
  EnSightNfacedASCII.py
  EnSightNfacedBin.py
  EnSightOfficeASCII.py
  EnSightOfficeBin.py
  EnSightRectGridASCII.py
  EnSightRectGridBin.py
  EnSightSelectArrays.py
  EnSightTensorsInversion.py
  EnSightTensorsInversionBin.py
  EnSightTimeChange.py,NO_VALID,NO_RT
  nacaBinary.py
  TestEnSightGoldCombinedReader.py,NO_RT
  )

# This test can take much longer on Windows than other platforms, so extend the timeout
if (NOT VTK_TEST_TIMEOUT_TestEnSightGoldCombinedReader)
  set(VTK_TEST_TIMEOUT_TestEnSightGoldCombinedReader 150)
endif()
set_tests_properties(VTK::IOEnSightPython-TestEnSightGoldCombinedReader
  PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestEnSightGoldCombinedReader})
