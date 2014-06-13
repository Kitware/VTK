""" Tests if vtkEnsembleSource works properly. """
import vtk
from vtk.test import Testing

class TestEnsemble(Testing.vtkTest):
    resolutions = [4, 8, 16, 32]
    npolys = [5, 9, 17, 33]

    def createSource(self):
        r = vtk.vtkEnsembleSource()

        aColumn = vtk.vtkIntArray()
        aColumn.SetName("Resolution")
        nrows = len(TestEnsemble.resolutions)
        for res in TestEnsemble.resolutions:
            aColumn.InsertNextValue(res)
        table = vtk.vtkTable()
        table.SetNumberOfRows(nrows)
        table.GetRowData().AddArray(aColumn)
        r.SetMetaData(table)

        for res in TestEnsemble.resolutions:
            c = vtk.vtkConeSource()
            c.SetResolution(res)
            r.AddMember(c)

        return r

    def test1(self):
        r = self.createSource()

        for i in range(len(TestEnsemble.resolutions)):
            r.SetCurrentMember(i)
            r.Update()
            self.assertEqual(r.GetOutputDataObject(0).GetNumberOfCells(), TestEnsemble.npolys[i])

    def test2(self):
        global nExecutions

        r = self.createSource()

        nExecutions = 0
        def addToCounter(obj, event):
            global nExecutions
            nExecutions += 1
        r.AddObserver('StartEvent', addToCounter)

        shrink = vtk.vtkShrinkPolyData()
        shrink.SetInputConnection(r.GetOutputPort())
        shrink.UpdateInformation()

        self.assertTrue(shrink.GetOutputInformation(0).Has(vtk.vtkEnsembleSource.META_DATA()))

        metaData = shrink.GetOutputInformation(0).Has(vtk.vtkEnsembleSource.META_DATA())

        shrink.Update()

        oInfo = shrink.GetOutputInformation(0)
        oInfo.Set(vtk.vtkEnsembleSource.UPDATE_MEMBER(), 2)
        shrink.Update()

        output = shrink.GetOutputDataObject(0)

        self.assertEquals(output.GetNumberOfCells(), TestEnsemble.npolys[2])

        shrink.Update()

        oInfo = shrink.GetOutputInformation(0)
        oInfo.Set(vtk.vtkEnsembleSource.UPDATE_MEMBER(), 1)
        shrink.Update()

        self.assertEquals(output.GetNumberOfCells(), TestEnsemble.npolys[1])

        shrink2 = vtk.vtkShrinkPolyData()
        shrink2.SetInputConnection(r.GetOutputPort())

        shrink2.Update()

        output2 = shrink2.GetOutputDataObject(0)

        self.assertEquals(output.GetNumberOfCells(), TestEnsemble.npolys[1])

        self.assertEquals(nExecutions, 3)

if __name__ == "__main__":
    Testing.main([(TestEnsemble, 'test')])
