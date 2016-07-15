# This tests that vtkPolyDataNormals handles cell data
# for strips properly. The filter splits strips to generate
# proper normals so it also needs to split cell data.
import vtk

ps = vtk.vtkPlaneSource()
ps.SetYResolution(10)

tf = vtk.vtkTriangleFilter()
tf.SetInputConnection(ps.GetOutputPort())

ts = vtk.vtkStripper()
ts.SetInputConnection(tf.GetOutputPort())

ts.Update()
strips = ts.GetOutput()

a = vtk.vtkFloatArray()
a.InsertNextTuple1(1)
a.SetName("foo")

strips.GetCellData().AddArray(a)

s = vtk.vtkSphereSource()
s.Update()

sphere = s.GetOutput()

a2 = vtk.vtkFloatArray()
a2.SetNumberOfTuples(96)
a2.FillComponent(0, 2)
a2.SetName("foo")

sphere.GetCellData().AddArray(a2)

app = vtk.vtkAppendPolyData()
app.AddInputData(strips)
app.AddInputData(sphere)

pdn = vtk.vtkPolyDataNormals()
pdn.SetInputConnection(app.GetOutputPort())
pdn.Update()

output = pdn.GetOutput()

foo = output.GetCellData().GetArray("foo")
for i in range(0, 96):
    assert(foo.GetValue(i) == 2)

for i in range(96, 116):
    assert(foo.GetValue(i) == 1)