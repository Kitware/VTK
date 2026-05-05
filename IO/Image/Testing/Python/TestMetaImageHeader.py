#!/usr/bin/env python
import tempfile

import vtk


def write(data: vtk.vtkImageData, path: str) -> None:
    writer = vtk.vtkMetaImageWriter()
    writer.SetFileName(path)
    writer.SetInputData(data)
    writer.Write()


def read(path: str) -> vtk.vtkImageData:
    reader = vtk.vtkMetaImageReader()
    reader.SetFileName(path)
    reader.Update()
    return reader.GetOutput()


def make_data():
    img = vtk.vtkImageData()
    img.SetDimensions(5, 4, 3)
    img.SetSpacing(2, 3, 4)
    img.SetOrigin(1, 2, 3)
    img.SetDirectionMatrix(1, 0, 0, 0, -1, 0, 0, 0, 1)

    scalars = vtk.vtkFloatArray()
    scalars.SetNumberOfComponents(1)
    scalars.SetNumberOfTuples(5 * 4 * 3)
    for i in range(5 * 4 * 3):
        scalars.SetValue(i, float(i))
    img.GetPointData().SetScalars(scalars)
    return img


def assert_equal(a, b) -> None:
    assert a == b, f"{a} vs {b}"


img1 = make_data()

with tempfile.NamedTemporaryFile(suffix=".mha") as tmp_file:
    write(img1, tmp_file.name)
    img2 = read(tmp_file.name)


assert_equal(img1.GetOrigin(), img2.GetOrigin())
assert_equal(img1.GetSpacing(), img2.GetSpacing())
assert_equal(img1.GetDirectionMatrix().GetData(), img2.GetDirectionMatrix().GetData())
