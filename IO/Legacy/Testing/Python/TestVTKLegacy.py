#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkIntArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkImageData,
    vtkRectilinearGrid,
    vtkSphere,
    vtkStructuredGrid,
)
from vtkmodules.vtkFiltersExtraction import vtkExtractGeometry
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkIOLegacy import (
    vtkPolyDataReader,
    vtkPolyDataWriter,
    vtkRectilinearGridReader,
    vtkRectilinearGridWriter,
    vtkStructuredGridReader,
    vtkStructuredGridWriter,
    vtkStructuredPointsReader,
    vtkStructuredPointsWriter,
    vtkUnstructuredGridReader,
    vtkUnstructuredGridWriter,
)
import sys

# Test writing out / reading of legacy VTK files. Currently
# tests: vtkPolyData, vtkUnstructuredGrid, vtkStructuredPoints,
# vtkStructuredGrid, vtkRectilinearGrid.

# Test vtkPolyData
sphere = vtkSphereSource()
sphere.SetThetaResolution(8)
sphere.SetPhiResolution(4)

# Write / read polygonal data - legacy version
print("I/O vtkPolyData")
wpd = vtkPolyDataWriter()
wpd.SetFileVersion(42)
wpd.WriteToOutputStringOn()
wpd.SetInputConnection(sphere.GetOutputPort())
wpd.Write()

legacyOutStr = wpd.GetOutputString()
#print(legacyOutStr)

rpd = vtkPolyDataReader()
rpd.ReadFromInputStringOn()
rpd.SetInputString(legacyOutStr)
rpd.Update()

print("Writing / reading version 4.2")
print("Major Version: ",rpd.GetFileMajorVersion())
print("Minor Version: ",rpd.GetFileMinorVersion())
print("File Version: ",rpd.GetFileVersion())

# Write / read polygonal data - latest version
wpd.SetFileVersion(51)
wpd.Write()

outStr = wpd.GetOutputString()
#print(outStr)

rpd.SetInputString(outStr)
rpd.Update()

print("Writing / reading version 5.1")
print("Major Version: ",rpd.GetFileMajorVersion())
print("Minor Version: ",rpd.GetFileMinorVersion())
print("File Version: ",rpd.GetFileVersion())

# Compare the strings and make sure a version difference is published.
if not "4.2" in legacyOutStr:
    print("Bad legacy writer output")
    sys.exit(1)

if not "5.1" in outStr:
    print("Bad writer output")
    sys.exit(1)

# Write / read unstructured data - legacy version
# Convert polydata to unstructured grid
print("\nI/O vtkUnstructuredGrid")
sph = vtkSphere()
sph.SetRadius(10000000)
extract = vtkExtractGeometry()
extract.SetInputConnection(sphere.GetOutputPort())
extract.SetImplicitFunction(sph)

wug = vtkUnstructuredGridWriter()
wug.SetFileVersion(42)
wug.WriteToOutputStringOn()
wug.SetInputConnection(extract.GetOutputPort())
wug.Write()

legacyOutStr = wug.GetOutputString()
#print(legacyOutStr)

rug = vtkUnstructuredGridReader()
rug.ReadFromInputStringOn()
rug.SetInputString(legacyOutStr)
rug.Update()

print("Writing / reading version 4.2")
print("Major Version: ",rug.GetFileMajorVersion())
print("Minor Version: ",rug.GetFileMinorVersion())
print("File Version: ",rug.GetFileVersion())

# Write / read ustructured grid - latest version
wug.SetFileVersion(51)
wug.Write()

outStr = wug.GetOutputString()
#print(outStr)

rug.SetInputString(outStr)
rug.Update()

print("Writing / reading version 5.1")
print("Major Version: ",rug.GetFileMajorVersion())
print("Minor Version: ",rug.GetFileMinorVersion())
print("File Version: ",rug.GetFileVersion())

# Compare the strings and make sure a version difference is published.
if not "4.2" in legacyOutStr:
    print("Bad legacy writer output")
    sys.exit(1)

if not "5.1" in outStr:
    print("Bad writer output")
    sys.exit(1)

# vtkImageData
print("\nI/O vtkStructuredPoints (aka vtkImageData)")
img = vtkImageData()
img.SetDimensions(3,4,5)
img.AllocateScalars(4,1) #array of shorts
num = 3*4*5
s = img.GetPointData().GetScalars()
for i in range(0,num):
    s.SetValue(i,i)

iw = vtkStructuredPointsWriter()
iw.SetInputData(img)
iw.SetFileVersion(42)
iw.WriteToOutputStringOn()
iw.Write()

legacyOutStr = iw.GetOutputString()
#print(legacyOutStr)

ir = vtkStructuredPointsReader()
ir.ReadFromInputStringOn()
ir.SetInputString(legacyOutStr)
ir.Update()

print("Writing / reading version 4.2")
print("Major Version: ",ir.GetFileMajorVersion())
print("Minor Version: ",ir.GetFileMinorVersion())
print("File Version: ",ir.GetFileVersion())

iw.SetFileVersion(51)
iw.Write()

outStr = iw.GetOutputString()
#print(outStr)

ir.SetInputString(outStr)
ir.Update()

print("Writing / reading version 5.1")
print("Major Version: ",ir.GetFileMajorVersion())
print("Minor Version: ",ir.GetFileMinorVersion())
print("File Version: ",ir.GetFileVersion())

# Compare the strings and make sure a version difference is published.
if not "4.2" in legacyOutStr:
    print("Bad legacy writer output")
    sys.exit(1)

if not "5.1" in outStr:
    print("Bad writer output")
    sys.exit(1)

# vtkStructuredGrid
print("\nI/O vtkStructuredGrid")
sg = vtkStructuredGrid()
dims = [3,4,5]
sg.SetDimensions(dims)
num = dims[0]*dims[1]*dims[2]

pts = vtkPoints()
pts.SetNumberOfPoints(num)
for k in range(0,dims[2]):
    for j in range(0,dims[1]):
        for i in range(0,dims[0]):
            pId = i + j*dims[0] + k*dims[0]*dims[1]
            pts.SetPoint(pId,i,j,k)
sg.SetPoints(pts)

sgs = vtkIntArray()
sgs.SetNumberOfTuples(num)
sg.GetPointData().SetScalars(sgs)
for i in range(0,num):
    sgs.SetValue(i,i)

sgw = vtkStructuredGridWriter()
sgw.SetInputData(sg)
sgw.SetFileVersion(42)
sgw.WriteToOutputStringOn()
sgw.Write()

legacyOutStr = sgw.GetOutputString()
#print(legacyOutStr)

ir = vtkStructuredGridReader()
ir.ReadFromInputStringOn()
ir.SetInputString(legacyOutStr)
ir.Update()

print("Writing / reading version 4.2")
print("Major Version: ",ir.GetFileMajorVersion())
print("Minor Version: ",ir.GetFileMinorVersion())
print("File Version: ",ir.GetFileVersion())

sgw.SetFileVersion(51)
sgw.Write()

outStr = sgw.GetOutputString()
#print(outStr)

ir.SetInputString(outStr)
ir.Update()

print("Writing / reading version 5.1")
print("Major Version: ",ir.GetFileMajorVersion())
print("Minor Version: ",ir.GetFileMinorVersion())
print("File Version: ",ir.GetFileVersion())

# Compare the strings and make sure a version difference is published.
if not "4.2" in legacyOutStr:
    print("Bad legacy writer output")
    sys.exit(1)

if not "5.1" in outStr:
    print("Bad writer output")
    sys.exit(1)

# vtkRectilinearGrid
print("\nI/O vtkRectilinearGrid")
rg = vtkRectilinearGrid()
dims = [3,4,5]
rg.SetDimensions(dims)
num = dims[0]*dims[1]*dims[2]

xPts = vtkFloatArray()
xPts.SetNumberOfTuples(dims[0])
yPts = vtkFloatArray()
yPts.SetNumberOfTuples(dims[1])
zPts = vtkFloatArray()
zPts.SetNumberOfTuples(dims[2])

for i in range(0,dims[0]):
    xPts.SetTuple1(i,i)
for j in range(0,dims[1]):
    yPts.SetTuple1(j,j)
for k in range(0,dims[2]):
    zPts.SetTuple1(k,k)

rg.SetXCoordinates(xPts)
rg.SetYCoordinates(yPts)
rg.SetZCoordinates(zPts)

rgs = vtkIntArray()
rgs.SetNumberOfTuples(num)
rg.GetPointData().SetScalars(rgs)
for i in range(0,num):
    rgs.SetValue(i,i)

rgw = vtkRectilinearGridWriter()
rgw.SetInputData(rg)
rgw.SetFileVersion(42)
rgw.WriteToOutputStringOn()
rgw.Write()

legacyOutStr = rgw.GetOutputString()
#print(legacyOutStr)

ir = vtkRectilinearGridReader()
ir.ReadFromInputStringOn()
ir.SetInputString(legacyOutStr)
ir.Update()

print("Writing / reading version 4.2")
print("Major Version: ",ir.GetFileMajorVersion())
print("Minor Version: ",ir.GetFileMinorVersion())
print("File Version: ",ir.GetFileVersion())

rgw.SetFileVersion(51)
rgw.Write()

outStr = rgw.GetOutputString()
#print(outStr)

ir.SetInputString(outStr)
ir.Update()

print("Writing / reading version 5.1")
print("Major Version: ",ir.GetFileMajorVersion())
print("Minor Version: ",ir.GetFileMinorVersion())
print("File Version: ",ir.GetFileVersion())

# Compare the strings and make sure a version difference is published.
if not "4.2" in legacyOutStr:
    print("Bad legacy writer output")
    sys.exit(1)

if not "5.1" in outStr:
    print("Bad writer output")
    sys.exit(1)

print("\nSuccessful read / write")
sys.exit(0)
