from vtkmodules.vtkCommonCore import vtkCommand, VTK_STRING
from vtkmodules.vtkCommonDataModel import vtkPartitionedDataSetCollection
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
from vtkmodules.vtkIOFides import vtkFidesReader
from vtkmodules.util.misc import vtkGetDataRoot

from vtk.test import Testing

VTK_DATA_ROOT = vtkGetDataRoot()

class ErrorObserver:
    def __init__(self):
        self.error_received = False
        self.error_msg = ""
        self.CallDataType = VTK_STRING

    def __call__(self, caller, event, calldata):
        if event == "ErrorEvent":
            self.error_received = True
            self.error_msg = calldata

class TestFidesReaderNoConduit(Testing.vtkTest):
    def testErrorMacroTriggered(self):

        reader = vtkFidesReader()

        observer = ErrorObserver()
        reader.AddObserver(vtkCommand.ErrorEvent, observer)

        reader.SetDataSourceNode("source", None)

        # Validate the error macro was triggered
        self.assertTrue(
            observer.error_received,
            "Test failed: Expected vtkFidesReader to emit an error, but none was caught."
        )

        # Validate the exact error text
        expected_text = "VTK must be compiled with Python wrapping and Conduit support"
        self.assertIn(
            expected_text,
            observer.error_msg,
            f"Test failed: Caught unexpected error message: {observer.error_msg}"
        )

        # Now make sure attempting to set data source conduit node didn't
        # affect the reader's ability to process a bp file

        reader.SetFileName(VTK_DATA_ROOT + "/Data/vtk-uns-grid-2.json")
        reader.SetDataSourcePath("source",
            VTK_DATA_ROOT + "/Data/tris-blocks-time.bp")
        reader.UpdateInformation()
        self.assertTrue(reader.GetOutputInformation(0).Has(
            vtkStreamingDemandDrivenPipeline.TIME_STEPS()))
        nsteps = reader.GetOutputInformation(0).Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
        self.assertEqual(nsteps, 5)

        reader.Update()

        pdsc = reader.GetOutputDataObject(0)
        self.assertTrue(isinstance(pdsc, vtkPartitionedDataSetCollection))

if __name__ == "__main__":
    Testing.main([(TestFidesReaderNoConduit, 'test')])
