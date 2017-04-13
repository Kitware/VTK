from vtkWebCorePython import vtkWebApplication
import vtk
from vtk.test import Testing

class TestObjectId(Testing.vtkTest):
    def testObjId(self):
        # Just make sure if we call it twice with None, the results match
        objId1 = vtkWebApplication.GetObjectId(None)
        objId1b = vtkWebApplication.GetObjectId(None)
        self.assertTrue(objId1 == objId1b)

        polyData = vtk.vtkPolyData()
        objId2 = vtkWebApplication.GetObjectId(polyData)
        self.assertEqual(objId2.index('0x'), 0)

        points = polyData.GetPoints()
        objId3 = vtkWebApplication.GetObjectId(points)
        self.assertTrue(objId3 == '0' or objId3 == '0x0')

if __name__ == "__main__":
    Testing.main([(TestObjectId, 'test')])
