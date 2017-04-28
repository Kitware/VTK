from vtkWebCorePython import vtkWebApplication
import vtk
from vtk.test import Testing


class TestObjectId(Testing.vtkTest):
    def testObjId(self):
        # Just make sure if we call it twice with None, the results match
        objId1 = vtkWebApplication.GetObjectId(None)
        objId1b = vtkWebApplication.GetObjectId(None)
        print('Object ids for None: objId1 => ',objId1,', objId1b => ',objId1b)
        self.assertTrue(objId1 == objId1b)

        polyData = vtk.vtkPolyData()
        objId2 = vtkWebApplication.GetObjectId(polyData)
        objId2b = vtkWebApplication.GetObjectId(polyData)
        print('Object ids for polydata: objId2 => ',objId2,', objId2b => ',objId2b)
        self.assertTrue(objId2 == objId2b)

        points = polyData.GetPoints()
        objId3 = vtkWebApplication.GetObjectId(points)
        objId3b = vtkWebApplication.GetObjectId(points)
        print('Object ids for polydata points: objId3 => ',objId3,', objId3b => ',objId3b)
        self.assertTrue(objId3 == objId3b)

if __name__ == "__main__":
    Testing.main([(TestObjectId, 'test')])
