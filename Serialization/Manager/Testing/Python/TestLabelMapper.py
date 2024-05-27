import math

from vtkmodules.test import Testing as vtkPyTesting
from vtkmodules.vtkSerializationManager import vtkObjectManager
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkCommonCore import vtkFloatArray, vtkLogger, vtkStringArray
from vtkmodules.vtkFiltersSources import vtkPolyLineSource
from vtkmodules.vtkFiltersCore import vtkElevationFilter
from vtkmodules.vtkRenderingCore import vtkActor, vtkActor2D, vtkPolyDataMapper, vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor
from vtkmodules.vtkRenderingLabel import vtkLabelPlacementMapper, vtkPointSetToLabelHierarchy

# Required for vtk factory
import vtkmodules.vtkRenderingContextOpenGL2  # noqa
import vtkmodules.vtkRenderingOpenGL2  # noqa
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleSwitch  # noqa

try:
    import numpy as np
except ImportError:
    print("This test requires numpy!")
    vtkPyTesting.skip()


class TestLabelMapper(vtkPyTesting.vtkTest):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def setUp(self):
        self.id_window = None
        self.client_window = None

        theta = np.linspace(-1 * np.pi, 1 * np.pi, 100)
        z = np.linspace(2, -2, 100)
        r = z**2 + 1
        x = r * np.sin(theta)
        y = r * np.cos(theta)
        points = np.column_stack((x, y, z))

        lineSource = vtkPolyLineSource()
        lineSource.SetNumberOfPoints(len(points))
        labels = vtkStringArray()
        for i, point in enumerate(points):
            lineSource.SetPoint(i, point[0], point[1], point[2])
            labels.InsertNextValue(f"x:{point[0]:.2f} y:{point[1]:.2f} z:{point[2]:.2f}")

        lineSource.Update()
        line = lineSource.GetOutput()
        labels.SetName('labels')
        line.GetPointData().AddArray(labels)

        elev = vtkElevationFilter()
        elev.SetInputData(line)
        mapper = vtkPolyDataMapper()
        mapper.SetInputConnection(elev.GetOutputPort())
        actor = vtkActor()
        actor.GetProperty().SetLineWidth(10)
        actor.GetProperty().SetRenderLinesAsTubes(True)
        actor.SetMapper(mapper)
        renderer = vtkRenderer()
        renderer.AddActor(actor)
        # renderer.SetBackground(1.0, 1.0, 1.0)

        hier = vtkPointSetToLabelHierarchy()
        hier.SetInputData(line)
        hier.SetLabelArrayName('labels')
        hier.Update()

        labelMapper = vtkLabelPlacementMapper()
        labelMapper.SetInputConnection(hier.GetOutputPort())
        labelActor = vtkActor2D()
        labelActor.SetMapper(labelMapper)

        renderer.AddActor(labelActor)

        self.server_window = vtkRenderWindow()
        iren = vtkRenderWindowInteractor()
        self.server_window.SetInteractor(iren)
        self.server_window.AddRenderer(renderer)

        self.server_window.SetMultiSamples(0)
        self.server_window.Render()

    def serialize(self):

        manager = vtkObjectManager()
        manager.Initialize()
        self.id_window = manager.RegisterObject(self.server_window)

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
        self.client_window = manager.GetObjectAtId(self.id_window)
        self.client_window.Render()

    def test(self):
        self.deserialize(*self.serialize())


if __name__ == "__main__":
    vtkLogger.Init()
    # vtkLogger.SetStderrVerbosity(vtkLogger.VERBOSITY_MAX)
    vtkPyTesting.main([(TestLabelMapper, 'test')])
