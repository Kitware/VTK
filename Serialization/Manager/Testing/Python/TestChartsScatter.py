import math

from vtkmodules.test import Testing as vtkPyTesting
from vtkmodules.vtkSerializationManager import vtkObjectManager
from vtkmodules.vtkChartsCore import vtkChart, vtkChartXY, vtkPlotPoints
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkCommonCore import vtkFloatArray, vtkLogger
from vtkmodules.vtkCommonDataModel import vtkTable
from vtkmodules.vtkViewsContext2D import vtkContextView
from vtkmodules.vtkRenderingCore import vtkTexture
from vtkmodules.vtkRenderingContext2D import vtkContextScene

# Required for vtk factory
import vtkmodules.vtkRenderingContextOpenGL2  # noqa
import vtkmodules.vtkRenderingOpenGL2  # noqa
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleSwitch  # noqa


class TestChartsScatter(vtkPyTesting.vtkTest):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def setUp(self):
        self.id_view = None
        colors = vtkNamedColors()

        self.server_view = vtkContextView()
        self.server_view.GetRenderer().SetBackground(colors.GetColor3d("SlateGray"))
        self.server_view.GetRenderWindow().SetSize(400, 300)
        self.client_view = None

        chart = vtkChartXY()
        self.server_view.GetScene().AddItem(chart)
        chart.SetShowLegend(True)

        table = vtkTable()

        arrX = vtkFloatArray()
        arrX.SetName("X Axis")

        arrC = vtkFloatArray()
        arrC.SetName("Cosine")

        arrS = vtkFloatArray()
        arrS.SetName("Sine")

        arrT = vtkFloatArray()
        arrT.SetName("Sine-Cosine")

        table.AddColumn(arrC)
        table.AddColumn(arrS)
        table.AddColumn(arrX)
        table.AddColumn(arrT)

        numPoints = 40

        inc = 7.5 / (numPoints - 1)
        table.SetNumberOfRows(numPoints)
        for i in range(numPoints):
            table.SetValue(i, 0, i * inc)
            table.SetValue(i, 1, math.cos(i * inc))
            table.SetValue(i, 2, math.sin(i * inc))
            table.SetValue(i, 3, math.sin(i * inc) - math.cos(i * inc))

        points = chart.AddPlot(vtkChart.POINTS)
        points.SetInputData(table, 0, 1)
        points.SetColor(0, 0, 0, 255)
        points.SetWidth(1.0)
        points.SetMarkerStyle(vtkPlotPoints.CROSS)

        points = chart.AddPlot(vtkChart.POINTS)
        points.SetInputData(table, 0, 2)
        points.SetColor(0, 0, 0, 255)
        points.SetWidth(1.0)
        points.SetMarkerStyle(vtkPlotPoints.PLUS)

        points = chart.AddPlot(vtkChart.POINTS)
        points.SetInputData(table, 0, 3)
        points.SetColor(0, 0, 255, 255)
        points.SetWidth(1.0)
        points.SetMarkerStyle(vtkPlotPoints.CIRCLE)

        self.server_view.GetRenderWindow().SetMultiSamples(0)
        self.server_view.GetRenderWindow().SetWindowName("ScatterPlot")
        self.server_view.GetRenderWindow().Render()

    def serialize(self):

        manager = vtkObjectManager()
        manager.Initialize()
        self.id_view = manager.RegisterObject(self.server_view)

        manager.UpdateStatesFromObjects()
        active_ids = manager.GetAllDependencies(0)
        manager.Export("state")

        states = map(manager.GetState, active_ids)
        hash_to_blob_map = {blob_hash: manager.GetBlob(
            blob_hash) for blob_hash in manager.GetBlobHashes(active_ids)}
        return states, hash_to_blob_map

    def deserialize(self, states, hash_to_blob_map):

        manager = vtkObjectManager()
        manager.Initialize()
        for state in states:
            manager.RegisterState(state)
        for hash_text, blob in hash_to_blob_map.items():
            manager.RegisterBlob(hash_text, blob)

        manager.UpdateObjectsFromStates()
        active_ids = manager.GetAllDependencies(0)
        self.client_view = manager.GetObjectAtId(self.id_view)
        self.client_view.GetInteractor().Render()

    def test(self):
        self.deserialize(*self.serialize())


if __name__ == "__main__":
    vtkLogger.Init()
    vtkLogger.SetStderrVerbosity(vtkLogger.VERBOSITY_MAX)
    vtkPyTesting.main([(TestChartsScatter, 'test')])
