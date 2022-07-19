#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Callback object to capture errors
class callback:
    def __init__(self):
        self.reset()

    def __call__(self, o, e, d = None):
        self.caller = o
        self.event = e
        self.calldata = d

    def reset(self):
        self.caller = None
        self.event = None
        self.calldata = None


reader = vtk.vtkXMLPolyDataReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/cow.vtp")
reader.Update()

# Create a loop around the ear of the cow. It is a somewhat complex area
# where the heuristic greedy edge search method fails but Dijkstra can
# find the path.
loopPointPositions = [[ 4.5208645 ,  2.0485868 , -0.5763462 ],
       [ 4.5447617 ,  1.9674546 , -0.57545805],
       [ 4.538317  ,  1.8611917 , -0.5673257 ],
       [ 4.5059876 ,  1.7356979 , -0.55352426],
       [ 4.4522295 ,  1.5968721 , -0.53562874],
       [ 4.381498  ,  1.4506135 , -0.51521415],
       [ 4.2982492 ,  1.3028216 , -0.49385568],
       [ 4.206939  ,  1.1593955 , -0.47312826],
       [ 4.112025  ,  1.0262344 , -0.454607  ],
       [ 4.0179615 ,  0.9092375 , -0.43986693],
       [ 3.9292052 ,  0.8143042 , -0.43048313],
       [ 3.8492908 ,  0.74591345, -0.42754683],
       [ 3.7780685 ,  0.7028636 , -0.430214  ],
       [ 3.7144666 ,  0.68253285, -0.43715683],
       [ 3.657414  ,  0.6822994 , -0.44704747],
       [ 3.605839  ,  0.6995413 , -0.4585581 ],
       [ 3.5586705 ,  0.7316368 , -0.47036093],
       [ 3.514837  ,  0.7759641 , -0.4811281 ],
       [ 3.4732673 ,  0.8299013 , -0.4895318 ],
       [ 3.43289   ,  0.8908266 , -0.49424416],
       [ 3.3926337 ,  0.9561181 , -0.4939374 ],
       [ 3.3520544 ,  1.023584  , -0.48774803],
       [ 3.3132184 ,  1.0927521 , -0.47666985],
       [ 3.2788188 ,  1.1635803 , -0.4621611 ],
       [ 3.2515495 ,  1.2360263 , -0.4456799 ],
       [ 3.2341034 ,  1.3100479 , -0.42868453],
       [ 3.2291746 ,  1.385603  , -0.4126331 ],
       [ 3.2394562 ,  1.4626493 , -0.39898378],
       [ 3.267642  ,  1.5411446 , -0.38919482],
       [ 3.316425  ,  1.6210469 , -0.38472438],
       [ 3.388499  ,  1.7023138 , -0.38703063],
       [ 3.4850955 ,  1.784371  , -0.3970445 ],
       [ 3.6015983 ,  1.8645154 , -0.41358784],
       [ 3.731929  ,  1.9395119 , -0.43495524],
       [ 3.8700097 ,  2.0061252 , -0.4594413 ],
       [ 4.0097623 ,  2.0611203 , -0.48534057],
       [ 4.145108  ,  2.1012616 , -0.5109477 ],
       [ 4.2699695 ,  2.1233141 , -0.5345572 ],
       [ 4.3782682 ,  2.1240425 , -0.55446374],
       [ 4.463926  ,  2.1002119 , -0.5689619 ],
       [ 4.5208645 ,  2.0485868 , -0.5763462 ]]

loopPoints = vtk.vtkPoints()
for xyz in loopPointPositions:
    loopPoints.InsertNextPoint(xyz)

# Add attribute information
cowPolyData = vtk.vtkPolyData()
cowPolyData.ShallowCopy(reader.GetOutput())

ptScalarArray = vtk.vtkIntArray()
ptScalarArray.SetName("ScalarArray")
ptScalarArray.SetNumberOfComponents(1)
ptScalarArray.SetNumberOfTuples(cowPolyData.GetNumberOfPoints())
ptScalarArray.Fill(1)

cowPolyData.GetPointData().AddArray(ptScalarArray)

cellScalarArray = vtk.vtkIntArray()
cellScalarArray.SetName("ScalarArray")
cellScalarArray.SetNumberOfComponents(1)
cellScalarArray.SetNumberOfTuples(cowPolyData.GetNumberOfCells())
cellScalarArray.Fill(1)

cowPolyData.GetCellData().AddArray(cellScalarArray)

# Filter setup
selectionFilter = vtk.vtkSelectPolyData()
selectionFilter.SetInputData(cowPolyData)
selectionFilter.SetLoop(loopPoints)
selectionFilter.GenerateSelectionScalarsOn()
selectionFilter.SetSelectionScalarsArrayName("SelectionArray")
selectionFilter.SetSelectionModeToSmallestRegion()

# Run selection filter using greedy method (expected to fail)

# Add error observer to catch the expected error (uncaught error would make the test fail)
cb = callback()
cb.CallDataType = vtk.VTK_STRING
observerId = selectionFilter.AddObserver(vtk.vtkCommand.ErrorEvent, cb)
# Run the computation
selectionFilter.SetEdgeSearchModeToGreedy()
selectionFilter.Update()
# Remove the error observer
selectionFilter.RemoveObserver(observerId)
# Check the results
if cb.event:
    print(f"As expected, greedy edge search failed: {cb.calldata}")
numberOfOutputPoints = selectionFilter.GetOutput().GetNumberOfPoints()
print(f"Number of points extracted using greedy edge search (0 means failure): {numberOfOutputPoints}")

# Run selection filter using Dijkstra method (expected to succeed)

# Run the computation
selectionFilter.SetEdgeSearchModeToDijkstra()
selectionFilter.Update()
# Check the results
numberOfOutputPoints = selectionFilter.GetOutput().GetNumberOfPoints()
print(f"Number of points extracted using Dijkstra edge search (0 means failure): {numberOfOutputPoints}")
if numberOfOutputPoints == 0:
    raise ValueError("Dijkstra edge search failed.")

testArray = selectionFilter.GetOutput().GetPointData().GetArray("SelectionArray")
if testArray is None:
    raise ValueError("Selection scalar array failed to generate using Dijkstra edge search.")

testPtScalarArray = selectionFilter.GetOutput().GetPointData().GetArray("ScalarArray")
if testPtScalarArray is None:
    raise ValueError("Point scalar array did not pass through using Dijkstra edge search.")

testCellScalarArray = selectionFilter.GetOutput().GetCellData().GetArray("ScalarArray")
if testCellScalarArray is None:
    raise ValueError("Cell scalar array did not pass through using Dijkstra edge search.")

# Display results

# Clip the mesh with the selection (cuts out a hole around the cow's ear)
clipFilter = vtk.vtkClipPolyData()
clipFilter.SetInputConnection(selectionFilter.GetOutputPort())

# Set up renderer
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Set up actor
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(clipFilter.GetOutputPort())
actor = vtk.vtkActor()
actor.SetMapper(mapper)
ren1.AddActor(actor)

# Set up view
ren1.GetActiveCamera().Azimuth(140)
ren1.ResetCamera()
renWin.SetSize(300,300)

renWin.Render()

# Uncomment for interactive rendering
# iren.Start()
