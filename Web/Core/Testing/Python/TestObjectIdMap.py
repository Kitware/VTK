import vtk
from vtk.test import Testing
from vtk.vtkWebCore import vtkWebApplication

class TestObjectId(Testing.vtkTest):
    def testObjId(self):
        map = vtk.vtkObjectIdMap()
        # Just make sure if we call it twice with None, the results match
        objId1 = map.GetGlobalId(None)
        objId1b = map.GetGlobalId(None)
        print('Object ids for None: objId1 => ',objId1,', objId1b => ',objId1b)
        self.assertTrue(objId1 == objId1b)

        object2 = vtk.vtkObject()
        addr2 = object2.__this__
        addr2 = addr2[1:addr2.find('_', 1)]
        addr2 = int(addr2, 16)

        object3 = vtk.vtkObject()
        addr3 = object3.__this__
        addr3 = addr3[1:addr3.find('_', 1)]
        addr3 = int(addr3, 16)

        # insert the bigger address first
        if (addr2 < addr3):
            object2, object3 = object3, object2

        objId2 = map.GetGlobalId(object2)
        objId2b = map.GetGlobalId(object2)
        print('Object ids for object2: objId2 => ',objId2,', objId2b => ',objId2b)
        self.assertTrue(objId2 == objId2b)

        objId3 = map.GetGlobalId(object3)
        objId3b = map.GetGlobalId(object3)
        print('Object ids for object3: objId3 => ',objId3,', objId3b => ',objId3b)
        self.assertTrue(objId3 == objId3b)

if __name__ == "__main__":
    Testing.main([(TestObjectId, 'test')])
