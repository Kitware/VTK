from vtk import *
import sys

# all values should be between 0 and 1
def WithinTol(a, b):
    for i in range(3):
        if abs(a[i] - b[i]) > 0.001:
            return False
    return True

def CheckTetra(success, x, pcoords):
    if x[0] >= 0. and x[0] <= 1.0 and x[1] >= 0. and x[1] <= 1.0 and x[2] >= 0. and x[2] <= 1.0 and x[0]+x[1]+x[2] < 1.01:
        if success != 1:
            print("should have found point ", x, " but did not")
            return False
        elif not WithinTol(x, pcoords):
            print("Parametric inversion failed for ", x, " with value of ", pcoords)
            return False
    else:
        if success == 1:
            print("should NOT have found point ", x, " but did. pcoords is ", pcoords)
            return False
    return True

def CheckHex(success, x, pcoords):
    if x[0] >= 0. and x[0] <= 1.0 and x[1] >= 0. and x[1] <= 1.0 and x[2] >= 0. and x[2] <= 1.0:
        if success != 1:
            print("should have found point ", x, " but did not")
            return False
        elif not WithinTol(x, pcoords):
            print("Parametric inversion failed for ", x, " with value of ", pcoords)
            return False
    else:
        if success == 1:
            print("should NOT have found point ", x, " but did. pcoords is ", pcoords)
            return False
    return True

def CheckPyramid(success, x, pcoords):
    # pcoords not in same coordinate space as x to we shrink it back down
    pcoords2 = [pcoords[0]*(1.-pcoords[2]), pcoords[1]*(1.-pcoords[2]), pcoords[2]]
    if x[0] >= 0. and x[0] <= 1.0-x[2] and x[1] >= 0. and x[1] <= 1.0-x[2] and x[2] >= 0. and x[2] <= 1.0:
        if success != 1:
            print("should have found point ", x, " but did not")
            return False
        elif not WithinTol(x, pcoords2):
            print("Parametric inversion failed for ", x, " with value of ", pcoords2)
            return False
    else:
        if success == 1:
            print("should NOT have found point ", x, " but did. pcoords is ", pcoords2)
            return False
    return True

def CheckWedge(success, x, pcoords):
    # wedge is a bit weird in that the normal of the bottom face (points 0, 1 and 2) using the right-hand rule
    # points away from the top face instead of towards it like the tet does.
    pcoords2 = [pcoords[1], pcoords[0], pcoords[2]]
    if x[0] >= 0. and x[0] <= 1.0 and x[1] >= 0. and x[1] <= 1.0 and x[2] >= 0. and x[2] <= 1.0 and x[0]+x[1] < 1.0001:
        if success != 1:
            print("should have found point ", x, " but did not")
            return False
        elif not WithinTol(x, pcoords2):
            print("Parametric inversion failed for ", x, " with value of ", pcoords2)
            return False
    else:
        if success == 1:
            print("should NOT have found point ", x, " but did. pcoords is ", pcoords2)
            return False
    return True

def CheckCell(cell, checkCellFunction):
    closestPoint = [0., 0., 0.]
    subId = reference(0)
    pcoords = [0., 0., 0.]
    dist2 = reference(0.)
    weights = [0.]*30
    x = [.25, .25, .25]
    for i in range(21):
        x[0] = -1.+3.*i/20.
        for j in range(21):
            x[1] = -1.+3.*j/20.
            for k in range(21):
                x[2] = -1.+3.*k/20.
                success = c.EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights)
                if not checkCellFunction(success, x, pcoords):
                    return False
    return True

pts = vtkPoints()
pts.SetNumberOfPoints(20) # quadratic hex has the most points with 20

# check Tets
pts.SetPoint(0, 0, 0, 0)
pts.SetPoint(1, 1, 0, 0)
pts.SetPoint(2, 0, 1, 0)
pts.SetPoint(3, 0, 0, 1)
pts.SetPoint(4, 0.5, 0, 0)
pts.SetPoint(5, 0.5, 0.5, 0)
pts.SetPoint(6, 0, 0.5, 0)
pts.SetPoint(7, 0, 0, 0.5)
pts.SetPoint(8, 0.5, 0, 0.5)
pts.SetPoint(9, 0., 0.5, 0.5)

c = vtkTetra()
c.Initialize(4, pts)
if not CheckCell(c, CheckTetra):
    print("Failure for vtkTetra")
    sys.exit(1)

c = vtkQuadraticTetra()
c.Initialize(10, pts)
if not CheckCell(c, CheckTetra):
    print("Failure for vtkQuadraticTetra")
    sys.exit(1)

# check hexes
pts.SetPoint(0, 0, 0, 0)
pts.SetPoint(1, 1, 0, 0)
pts.SetPoint(2, 1, 1, 0)
pts.SetPoint(3, 0, 1, 0)
pts.SetPoint(4, 0, 0, 1)
pts.SetPoint(5, 1, 0, 1)
pts.SetPoint(6, 1, 1, 1)
pts.SetPoint(7, 0, 1, 1)
pts.SetPoint(8, .5, 0, 0)
pts.SetPoint(9, 1, .5, 0)
pts.SetPoint(10, .5, 1, 0)
pts.SetPoint(11, 0, .5, 0)
pts.SetPoint(12, .5, 0, 1)
pts.SetPoint(13, 1, .5, 1)
pts.SetPoint(14, .5, 1, 1)
pts.SetPoint(15, 0, .5, 1)
pts.SetPoint(16, 0, 0, .5)
pts.SetPoint(17, 1, 0, .5)
pts.SetPoint(18, 1, 1, .5)
pts.SetPoint(19, 0, 1, .5)

c = vtkHexahedron()
c.Initialize(8, pts)
if not CheckCell(c, CheckHex):
    print("Failure for vtkHexahedron")
    sys.exit(1)

c = vtkQuadraticHexahedron()
c.Initialize(20, pts)
if not CheckCell(c, CheckHex):
    print("Failure for vtkQuadraticHexahedron")
    sys.exit(1)

# check pyramids
pts.SetPoint(0, 0, 0, 0)
pts.SetPoint(1, 1, 0, 0)
pts.SetPoint(2, 1, 1, 0)
pts.SetPoint(3, 0, 1, 0)
pts.SetPoint(4, 0, 0, 1)
pts.SetPoint(5, .5, 0, 0)
pts.SetPoint(6, 1, .5, 0)
pts.SetPoint(7, .5, 1, 0)
pts.SetPoint(8, 0, .5, 0)
pts.SetPoint(9, 0, 0, .5)
pts.SetPoint(10, .5, 0, .5)
pts.SetPoint(11, .5, .5, .5)
pts.SetPoint(12, 0, .5, .5)

c = vtkPyramid()
c.Initialize(5, pts)
if not CheckCell(c, CheckPyramid):
    print("Failure for vtkPyramid")
    sys.exit(1)

c = vtkQuadraticPyramid()
c.Initialize(13, pts)
if not CheckCell(c, CheckPyramid):
    print("Failure for vtkQuadraticPyramid")
    sys.exit(1)

# check wedges
pts.SetPoint(0, 0, 0, 0)
pts.SetPoint(1, 0, 1, 0)
pts.SetPoint(2, 1, 0, 0)
pts.SetPoint(3, 0, 0, 1)
pts.SetPoint(4, 0, 1, 1)
pts.SetPoint(5, 1, 0, 1)

pts.SetPoint(6, 0, .5, 0)
pts.SetPoint(7, .5, .5, 0)
pts.SetPoint(8, .5, 0, 0)
pts.SetPoint(9, 0, .5, 1)
pts.SetPoint(10, .5, .5, 1)
pts.SetPoint(11, .5, 0, 1)
pts.SetPoint(12, 0, 0, .5)
pts.SetPoint(13, 0, 1, .5)
pts.SetPoint(14, 1, 0, .5)

c = vtkWedge()
c.Initialize(6, pts)
if not CheckCell(c, CheckWedge):
    print("Failure for vtkWedge")
    sys.exit(1)

c = vtkQuadraticWedge()
c.Initialize(15, pts)
if not CheckCell(c, CheckWedge):
    print("Failure for vtkQuadraticWedge")
    sys.exit(1)


print("Success")
sys.exit(0)
