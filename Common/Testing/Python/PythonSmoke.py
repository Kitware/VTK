import sys

try:
  import vtk

except:
  print "Cannot import vtk"
  sys.exit(1)

try:
  print dir(vtk)
except:
  print "Cannot print dir(vtk)"
  sys.exit(1)

try:
  o = vtk.vtkObject()
except:
  print "Cannot create vtkObject"
  sys.exit(1)

try:
  print o
except:
  print "Cannot print object"
  sys.exit(1)

sys.exit(0)

