import vtk

class VTKAlgorithm(object):
    """This is a superclass which can be derived to implement
    Python classes that work with vtkPythonAlgorithm. It implements
    Initialize(), ProcessRequest(), FillInputPortInformation() and
    FillOutputPortInformation().

    Initialize() sets the input and output ports based on data
    members.

    ProcessRequest() calls RequestXXX() methods to implement
    various pipeline passes.

    FillInputPortInformation() and FillOutputPortInformation() set
    the input and output types based on data members.
    """

    def __init__(self, nInputPorts=1, inputType='vtkDataSet',
                       nOutputPorts=1, outputType='vtkPolyData'):
        """Sets up default NumberOfInputPorts, NumberOfOutputPorts,
        InputType and OutputType that are used by various initialization
        methods."""

        self.NumberOfInputPorts = nInputPorts
        self.NumberOfOutputPorts = nOutputPorts
        self.InputType = inputType
        self.OutputType = outputType

    def Initialize(self, vtkself):
        """Sets up number of input and output ports based on
        NumberOfInputPorts and NumberOfOutputPorts."""

        vtkself.SetNumberOfInputPorts(self.NumberOfInputPorts)
        vtkself.SetNumberOfOutputPorts(self.NumberOfOutputPorts)

    def GetInputData(self, inInfo, i, j):
        """Convenience method that returns an input data object
        given a vector of information objects and two indices."""

        return inInfo[i].GetInformationObject(j).Get(vtk.vtkDataObject.DATA_OBJECT())

    def GetOutputData(self, outInfo, i):
        """Convenience method that returns an output data object
        given an information object and an index."""
        return outInfo.GetInformationObject(i).Get(vtk.vtkDataObject.DATA_OBJECT())

    def RequestDataObject(self, vtkself, request, inInfo, outInfo):
        """Overwritten by subclass to manage data object creation.
        There is not need to overwrite this class if the output can
        be created based on the OutputType data member."""
        return 1

    def RequestInformation(self, vtkself, request, inInfo, outInfo):
        """Overwritten by subclass to provide meta-data to downstream
        pipeline."""
        return 1

    def RequestUpdateExtent(self, vtkself, request, inInfo, outInfo):
        """Overwritten by subclass to modify data request going
        to upstream pipeline."""
        return 1

    def RequestData(self, vtkself, request, inInfo, outInfo):
        """Overwritten by subclass to execute the algorithm."""
        raise NotImplementedError('RequestData must be implemented')

    def ProcessRequest(self, vtkself, request, inInfo, outInfo):
        """Splits a request to RequestXXX() methods."""
        if request.Has(vtk.vtkDemandDrivenPipeline.REQUEST_DATA_OBJECT()):
            return self.RequestDataObject(vtkself, request, inInfo, outInfo)
        elif request.Has(vtk.vtkDemandDrivenPipeline.REQUEST_INFORMATION()):
            return self.RequestInformation(vtkself, request, inInfo, outInfo)
        elif request.Has(vtk.vtkStreamingDemandDrivenPipeline.REQUEST_UPDATE_EXTENT()):
            return self.RequestUpdateExtent(vtkself, request, inInfo, outInfo)
        elif request.Has(vtk.vtkDemandDrivenPipeline.REQUEST_DATA()):
            return self.RequestData(vtkself, request, inInfo, outInfo)

        return 1

    def FillInputPortInformation(self, vtkself, port, info):
        """Sets the required input type to InputType."""
        info.Set(vtk.vtkAlgorithm.INPUT_REQUIRED_DATA_TYPE(), self.InputType)
        return 1

    def FillOutputPortInformation(self, vtkself, port, info):
        """Sets the default output type to OutputType."""
        info.Set(vtk.vtkDataObject.DATA_TYPE_NAME(), self.OutputType)
        return 1
