""" Tests if vtkEnsembleSource works properly. """
from vtkmodules.vtkCommonCore import vtkIntArray
from vtkmodules.vtkCommonDataModel import vtkTable
from vtkmodules.vtkCommonExecutionModel import vtkEnsembleSource
from vtkmodules.vtkFiltersGeneral import vtkShrinkPolyData
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.test import Testing

class TestEnsemble(Testing.vtkTest):
    resolutions = [4, 8, 16, 32]
    npolys = [5, 9, 17, 33]

    def createSource(self):
        r = vtkEnsembleSource()

        aColumn = vtkIntArray()
        aColumn.SetName("Resolution")
        nrows = len(TestEnsemble.resolutions)
        for res in TestEnsemble.resolutions:
            aColumn.InsertNextValue(res)
        table = vtkTable()
        table.SetNumberOfRows(nrows)
        table.GetRowData().AddArray(aColumn)
        r.SetMetaData(table)

        for res in TestEnsemble.resolutions:
            c = vtkConeSource()
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

        shrink = vtkShrinkPolyData()
        shrink.SetInputConnection(r.GetOutputPort())
        shrink.UpdateInformation()

        self.assertTrue(shrink.GetOutputInformation(0).Has(vtkEnsembleSource.META_DATA()))

        metaData = shrink.GetOutputInformation(0).Has(vtkEnsembleSource.META_DATA())

        shrink.Update()

        oInfo = shrink.GetOutputInformation(0)
        oInfo.Set(vtkEnsembleSource.UPDATE_MEMBER(), 2)
        shrink.Update()

        output = shrink.GetOutputDataObject(0)

        self.assertEqual(output.GetNumberOfCells(), TestEnsemble.npolys[2])

        shrink.Update()

        oInfo = shrink.GetOutputInformation(0)
        oInfo.Set(vtkEnsembleSource.UPDATE_MEMBER(), 1)
        shrink.Update()

        self.assertEqual(output.GetNumberOfCells(), TestEnsemble.npolys[1])

        shrink2 = vtkShrinkPolyData()
        shrink2.SetInputConnection(r.GetOutputPort())

        shrink2.Update()

        output2 = shrink2.GetOutputDataObject(0)

        self.assertEqual(output.GetNumberOfCells(), TestEnsemble.npolys[1])

        self.assertEqual(nExecutions, 3)

if __name__ == "__main__":
    Testing.main([(TestEnsemble, 'test')])
