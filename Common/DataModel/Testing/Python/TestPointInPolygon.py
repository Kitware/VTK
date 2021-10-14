#!/usr/bin/env python
import vtk

# Test the vtkPolygon::PointInPolygon() method
#

# Compute point in polygon for a series of points
polygon = vtk.vtkPolygon()
numPts = 4
pts = [0,0,0, 1,0,0, 1,1,0, 0,1,0]
bds = [0,1,0,1,0,1]

# Create and test an invalid polygon
n = [0,0,0]
x = [0.5,0.5,0.0]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  -1

# The next four points should be inside
n = [0,0,1]
x = [0.5,0.5,0.0]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  1

x = [0.1,0.5,0.0]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  1

x = [0.1,0.1,0.0]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  1

x = [0.01,0.01,0.0]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  1

# The next four should be outside
x = [-0.5,-0.5,0.0]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

x = [-0.1,-0.5,0.0]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

x = [-0.1,-0.1,0.0]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

x = [-0.01,-0.01,0.0]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

# Now orient the plane in the x-z plane
pts = [0,0,0, 1,0,0, 1,0,1, 0,0,1]
n = [0,1,0]

# The next four should be inside
x = [0.5,0.0,0.5]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  1

x = [0.1,0.0,0.5]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  1

x = [0.1,0.0,0.1]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  1

x = [0.01,0.0,0.01]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  1

# The next four should be outside
x = [-0.5,0.0,-0.5]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

x = [-0.1,0.0,-0.5]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

x = [-0.1,0.0,-0.1]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

x = [-0.01,0.0,-0.01]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

# Now orient the plane in the y-z plane
pts = [0,0,0, 0,1,0, 0,1,1, 0,0,1]
bds = [0,1,0,1,-1,1]
n = [1,0,0]

# The next four should be inside
x = [0.0,0.5,0.5]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  1

x = [0.0,0.1,0.5]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  1

x = [0.0,0.1,0.1]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  1

x = [0.0,0.01,0.01]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout ==  1

# The next four should be outside
x = [0.0,-0.5,-0.5]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

x = [0.0,-0.1,-0.5]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

x = [0.0,-0.1,-0.1]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

x = [0.0,-0.01,-0.01]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

# The next four should be outside - culled by bbox test
x = [0.0,-1.5,-0.5]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

x = [1.5,0.1,-0.5]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

x = [0.5,0.1,-1.5]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0

x = [0.5,0.1,1.5]
inout = polygon.PointInPolygon(x,numPts,pts,bds,n)
assert inout == 0
