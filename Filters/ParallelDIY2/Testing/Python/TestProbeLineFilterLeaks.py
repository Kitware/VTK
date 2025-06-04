from vtkmodules.vtkFiltersSources import vtkLineSource,vtkCellTypeSource,vtkRandomHyperTreeGridSource
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkFiltersParallelDIY2 import vtkProbeLineFilter
from vtkmodules.vtkParallelCore import vtkDummyController



def common(source):
  line = vtkLineSource()
  line.SetPoint1(0,0,0)
  line.SetPoint2(10,10,10)
  line.SetResolution(1)
  dc = vtkDummyController()
  probe = vtkProbeLineFilter()
  probe.SetController(dc)
  probe.SetLineResolution(1000)
  probe.SetInputConnection(source.GetOutputPort(0))
  probe.SetSourceConnection(line.GetOutputPort())
  probe.SetSamplingPattern(2)
  probe.Update()
  assert probe.GetOutputDataObject(0).GetNumberOfPoints() == 1001

def Test_UnstructuredGrid():
  grid = vtkCellTypeSource()
  grid.SetBlocksDimensions(10,10,10)
  grid.Update()
  common(grid)

def Test_ImageData():
  image = vtkRTAnalyticSource()
  image.Update()
  common(image)

def Test_HyperTreeGrid():
  hgrid = vtkRandomHyperTreeGridSource()
  hgrid.Update()
  common(hgrid)


Test_ImageData()
Test_UnstructuredGrid()
Test_HyperTreeGrid()
