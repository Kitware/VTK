set(tests
  NetCDFCF2DBounds.py
  NetCDFCFSetOutputType.py
  NetCDFCFSphericalCoords.py
  NetCDFCFUnstructured.py
  NetCDFReader.py
)

# The NetCDFCF reader tests are reused to ensure that the
# XArray interface (which operates through the NetCDFCF reader) also
# works. For these duplicated tests, add an argument for the test to
# load data through xarray and a second argument to have the test use the
# same baseline image as the original test.
foreach (name IN LISTS tests)
  string(REGEX REPLACE "(.*)\.py" "\\1" name_no_ext "${name}")
  # pass xarray to test the XArray NetCDFCFReader connection
  set("${name_no_ext}XArray_ARGS" xarray)
  # use the same baseline as the original NetCDFCFReader tests
  set("${name_no_ext}XArray_BASELINE_NAME" ${name_no_ext})
endforeach()

vtk_add_test_python(
  # Rest NetCDFCFReader
  ${tests}

  # Test XArray interface Reuse the same `.py` script but provide a
  # different name. Also note that the for loop above provides extra
  # parameters to run the XArray interface and point to the correct
  # baseline image.
  NetCDFCF2DBoundsXArray,NetCDFCF2DBounds.py
  NetCDFCFSetOutputTypeXArray,NetCDFCFSetOutputType.py
  NetCDFCFSphericalCoordsXArray,NetCDFCFSphericalCoords.py
  NetCDFReaderXArray,NetCDFReader.py
  NetCDFCFUnstructuredXArray,NetCDFCFUnstructured.py
  )
