import vtk


# -----------------------------------------------------------------------------
# Rendering
# -----------------------------------------------------------------------------

def render_dataset(dataset, title, remove_ghost_interfaces):
    geom = vtk.vtkGeometryFilter()
    geom.SetInputData(dataset)
    geom.SetRemoveGhostInterfaces(remove_ghost_interfaces)
    geom.Update()

    mapper = vtk.vtkDataSetMapper()
    mapper.SetInputData(geom.GetOutput())
    mapper.SetScalarVisibility(False)

    actor = vtk.vtkActor()
    actor.SetMapper(mapper)

    prop = actor.GetProperty()
    prop.EdgeVisibilityOn()
    prop.SetEdgeColor(0.0, 0.0, 0.0)
    prop.SetLineWidth(4.0)
    prop.SetColor(1.0, 1.0, 1.0)

    text = vtk.vtkTextActor()
    text.SetInput(title)
    tprop = text.GetTextProperty()
    tprop.SetFontSize(24)
    tprop.SetColor(1.0, 1.0, 1.0)
    text.SetPosition(10, 10)

    renderer = vtk.vtkRenderer()
    renderer.AddActor(actor)
    renderer.AddViewProp(text)
    renderer.SetBackground(0.1, 0.1, 0.1)

    camera = renderer.GetActiveCamera()
    camera.SetPosition(1, 1, 1)
    camera.SetFocalPoint(0, 0, 0)
    camera.SetViewUp(0, 0, 1)
    renderer.ResetCamera()
    camera.Zoom(0.7)
    renderer.ResetCameraClippingRange()

    window = vtk.vtkRenderWindow()
    window.AddRenderer(renderer)
    window.SetSize(320, 400)
    window.Render()

    w2i = vtk.vtkWindowToImageFilter()
    w2i.SetInput(window)
    w2i.Update()

    image = vtk.vtkImageData()
    image.ShallowCopy(w2i.GetOutput())
    return image


# -----------------------------------------------------------------------------
# Base dataset
# -----------------------------------------------------------------------------

def create_image_data():
    data = vtk.vtkImageData()
    data.SetDimensions(3, 3, 3)
    return data


# -----------------------------------------------------------------------------
# Conversions
# -----------------------------------------------------------------------------

def image_data_to_rectilinear_grid(data):
    dims = data.GetDimensions()
    spacing = data.GetSpacing()
    origin = data.GetOrigin()
    nx, ny, nz = dims

    x = vtk.vtkDoubleArray()
    x.SetNumberOfValues(nx)
    for i in range(nx):
        x.SetValue(i, origin[0] + i * spacing[0])

    y = vtk.vtkDoubleArray()
    y.SetNumberOfValues(ny)
    for j in range(ny):
        y.SetValue(j, origin[1] + j * spacing[1])

    z = vtk.vtkDoubleArray()
    z.SetNumberOfValues(nz)
    for k in range(nz):
        z.SetValue(k, origin[2] + k * spacing[2])

    rgrid = vtk.vtkRectilinearGrid()
    rgrid.SetDimensions(nx, ny, nz)
    rgrid.SetXCoordinates(x)
    rgrid.SetYCoordinates(y)
    rgrid.SetZCoordinates(z)

    rgrid.GetPointData().ShallowCopy(data.GetPointData())
    rgrid.GetCellData().ShallowCopy(data.GetCellData())
    return rgrid


def image_data_to_structured_grid(data):
    alg = vtk.vtkImageToStructuredGrid()
    alg.SetInputData(data)
    alg.Update()
    return alg.GetOutput()


def image_data_to_unstructured_grid(data):
    alg = vtk.vtkAppendFilter()
    alg.AddInputData(data)
    alg.Update()
    return alg.GetOutput()


def image_data_to_explicit_structured_grid(data):
    ugrid = image_data_to_unstructured_grid(data)

    dims = data.GetDimensions()
    ni, nj, nk = dims[0] - 1, dims[1] - 1, dims[2] - 1
    n_cells = ugrid.GetNumberOfCells()

    arr_i = vtk.vtkIntArray()
    arr_i.SetName("BLOCK_I")
    arr_i.SetNumberOfTuples(n_cells)

    arr_j = vtk.vtkIntArray()
    arr_j.SetName("BLOCK_J")
    arr_j.SetNumberOfTuples(n_cells)

    arr_k = vtk.vtkIntArray()
    arr_k.SetName("BLOCK_K")
    arr_k.SetNumberOfTuples(n_cells)

    cid = 0
    for k in range(nk):
        for j in range(nj):
            for i in range(ni):
                arr_i.SetValue(cid, i)
                arr_j.SetValue(cid, j)
                arr_k.SetValue(cid, k)
                cid += 1

    cd = ugrid.GetCellData()
    cd.AddArray(arr_i)
    cd.AddArray(arr_j)
    cd.AddArray(arr_k)

    alg = vtk.vtkUnstructuredGridToExplicitStructuredGrid()
    alg.SetInputData(ugrid)
    alg.SetInputArrayToProcess(0, 0, 0, 1, "BLOCK_I")
    alg.SetInputArrayToProcess(1, 0, 0, 1, "BLOCK_J")
    alg.SetInputArrayToProcess(2, 0, 0, 1, "BLOCK_K")
    alg.Update()

    return alg.GetOutput()


# -----------------------------------------------------------------------------
# Ghost cell injection
# -----------------------------------------------------------------------------

def add_ghost_cells(data, ghost_flag):
    ghost = vtk.vtkUnsignedCharArray()
    ghost.SetName(vtk.vtkDataSetAttributes.GhostArrayName())
    ghost.SetNumberOfTuples(data.GetNumberOfCells())

    for i in range(4):
        ghost.SetValue(i, 0)

    for i in range(4, 8):
        ghost.SetValue(i, ghost_flag)

    cd = data.GetCellData()
    cd.AddArray(ghost)
    cd.SetActiveScalars(vtk.vtkDataSetAttributes.GhostArrayName())


# -----------------------------------------------------------------------------
# Test matrix
# -----------------------------------------------------------------------------

MESH_FACTORIES = {
    "ImageData": create_image_data,
    "RectilinearGrid": lambda: image_data_to_rectilinear_grid(create_image_data()),
    "StructuredGrid": lambda: image_data_to_structured_grid(create_image_data()),
    "UnstructuredGrid": lambda: image_data_to_unstructured_grid(create_image_data()),
    "ExplicitStructuredGrid": lambda: image_data_to_explicit_structured_grid(create_image_data()),
}

# -----------------------------------------------------------------------------
# Render 4 rows
# -----------------------------------------------------------------------------

rows = []

for label, ghost_flag in (
        ("HIDDENCELL", vtk.vtkDataSetAttributes.HIDDENCELL),
        ("DUPLICATECELL", vtk.vtkDataSetAttributes.DUPLICATECELL),
):
    for remove_interfaces in (False, True):
        images = []

        for mesh_type, factory in MESH_FACTORIES.items():
            dataset = factory()
            add_ghost_cells(dataset, ghost_flag)

            title = f"{mesh_type}\n{label}\nRemoveInterfaces={remove_interfaces}"
            images.append(render_dataset(dataset, title, remove_interfaces))

        append_x = vtk.vtkImageAppend()
        append_x.SetAppendAxis(0)
        for img in images:
            append_x.AddInputData(img)
        append_x.Update()

        rows.append(append_x.GetOutput())

# -----------------------------------------------------------------------------
# Combine rows vertically
# -----------------------------------------------------------------------------

append_y = vtk.vtkImageAppend()
append_y.SetAppendAxis(1)
for row in rows:
    append_y.AddInputData(row)
append_y.Update()

# # Optionally write to file
# writer = vtk.vtkPNGWriter()
# writer.SetFileName("combined.png")
# writer.SetInputData(append_y.GetOutput())
# writer.Write()

final_image = append_y.GetOutput()

mapper = vtk.vtkImageSliceMapper()
mapper.SetInputData(final_image)

image_slice = vtk.vtkImageSlice()
image_slice.SetMapper(mapper)

extent = final_image.GetExtent()
origin = final_image.GetOrigin()
spacing = final_image.GetSpacing()

# Center coordinates
xc = origin[0] + 0.5 * (extent[0] + extent[1]) * spacing[0]
yc = origin[1] + 0.5 * (extent[2] + extent[3]) * spacing[1]
yd = (extent[3] - extent[2] + 1) * spacing[1]

renderer = vtk.vtkRenderer()
renderer.AddViewProp(image_slice)
renderer.SetBackground(0.1, 0.1, 0.1)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(renderer)
renWin.SetSize(1000, 1000)

camera = renderer.GetActiveCamera()
camera.ParallelProjectionOn()
d = camera.GetDistance()
camera.SetParallelScale(0.5 * yd)
camera.SetFocalPoint(xc, yc, 0.0)
camera.SetPosition(xc, yc, d)

iren = vtk.vtkRenderWindowInteractor()
style = vtk.vtkInteractorStyleImage()
iren.SetInteractorStyle(style)
iren.SetRenderWindow(renWin)

renWin.Render()
iren.Initialize()
iren.Start()
