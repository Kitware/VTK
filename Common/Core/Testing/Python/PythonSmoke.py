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
  try:
    try:
      o = vtk.vtkLineWidget()
      print "Using Hybrid"
    except:
      o = vtk.vtkActor()
      print "Using Rendering"
  except:
    o = vtk.vtkObject()
    print "Using Common"
except:
  print "Cannot create vtkObject"
  sys.exit(1)

try:
  print o
  print "Reference count: %d" % o.GetReferenceCount()
  print "Class name: %s" % o.GetClassName()
except:
  print "Cannot print object"
  sys.exit(1)

try:
  b = vtk.vtkObject()
  d = b.SafeDownCast(o)
  print b, d
except:
  print "Cannot downcast"
  sys.exit(1)

sys.exit(0)

