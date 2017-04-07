from vtkWebCorePython import vtkWebApplication
import vtk
from vtk.test import Testing

class TestObjectId(Testing.vtkTest):
    def testObjId(self):
        objId1 = vtkWebApplication.GetObjectId(None)
        self.assertEqual(objId1, '0')

        polyData = vtk.vtkPolyData()
        objId2 = vtkWebApplication.GetObjectId(polyData)
        self.assertEqual(objId2.index('0x'), 0)

        points = polyData.GetPoints()
        objId3 = vtkWebApplication.GetObjectId(points)
        self.assertEqual(objId3, '0')

if __name__ == "__main__":
    Testing.main([(TestObjectId, 'test')])
