from vtk.vtkCommonCore import (
    vtkMath,
    vtkPoints
)
from vtk.vtkCommonDataModel import vtkPolygon;
from vtkmodules.test import Testing

import math

class TestStarPolygon(Testing.vtkTest):
    def test(self):
        n = 12
        t = 2.0*math.pi/n
        s = (1.0 + math.sin(t))/math.cos(t) - 1.0

        points = vtkPoints()
        for z,e in enumerate([-1.0e-6,0.0,1.0e-6]):
            for i in range(n):
                r = 25.0*(1.0 + (i%2)*s + e)
                x, y = r*math.cos(i*t), r*math.sin(i*t)
                points.InsertNextPoint([x, y, 10.0*z])

        nn = [-1,-1,-1]
        for ci in range(3):
            pts = [ci*n + i for i in range(n)]
            vtkPolygon.ComputeNormal(points, n, pts, nn)
            area = vtkPolygon.ComputeArea(points, n, pts, nn)
            print("Cell: %2d Normal: [%5.2f, %5.2f, %5.2f], Area: %g" % (ci, nn[0], nn[1], nn[2], area))
            self.assertEqual(abs(vtkMath.Norm(nn) - 1.0) < 0.0001, True)

if __name__ == "__main__":
    Testing.main([(TestStarPolygon, 'test')])
