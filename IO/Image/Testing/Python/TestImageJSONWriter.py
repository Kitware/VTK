#!/usr/bin/env python
import os, json, sys
import vtk

args = sys.argv[1:]
temp_dir = args[args.index("-T") + 1]

# Always use / to prevent windows/python issue with backslash
tmp_file = temp_dir + '/wavelet_slice_3.json'

expected_first_values = [75.9335, 102.695, 91.2387, 115.507, 105.995, 125.724, 118.773, 132.24, 128.255, 134.254, 133.446, 131.431, 133.843, 123.998, 129.505]

# Image pipeline
image1 = vtk.vtkRTAnalyticSource()
image1.Update()
output = image1.GetOutput()
dim_ref = [value for value in output.GetDimensions()]
origin_ref = [value for value in output.GetOrigin()]
spacing_ref = [value for value in output.GetSpacing()]

print(dim_ref)
print(origin_ref)
print(spacing_ref)

writer = vtk.vtkJSONImageWriter()
writer.SetInputData(image1.GetOutput())
writer.SetFileName(tmp_file)
writer.SetArrayName("RTData")
writer.SetSlice(3)
writer.Write()

# Try to load JSON file and compare with dumped data
print("Writing file:", tmp_file)
json_file = open(tmp_file, "r")
json_obj = json.load(json_file)
json_file.close()

slice = json_obj['RTData']
if json_obj["dimensions"] != dim_ref:
    print("Dimension ERROR")
    sys.exit(1)
else:
    print("Dimension OK")
if json_obj["origin"] != origin_ref:
    print("Origin ERROR")
    sys.exit(1)
else:
    print("Origin OK")
if json_obj["spacing"] != spacing_ref:
    print("Spacing ERROR")
    sys.exit(1)
else:
    print("Spacing OK")
if len(slice) == 441:
    print("Slice size OK")
else:
    print("Slice size ERROR - Size of ", str(len(slice)))
    sys.exit(1)

for i in range(len(expected_first_values)):
    if expected_first_values[i] != slice[i]:
        sys.exit(1)

print("All good...")
sys.exit()
