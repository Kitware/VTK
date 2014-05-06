#! /usr/bin/env python

"""
Test NIFTI support in VTK by reading a file, writing it, and
then re-reading it to ensure that the contents are identical.
"""

import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
from vtk.util.misc import vtkGetTempDir

VTK_DATA_ROOT = vtkGetDataRoot()
VTK_TEMP_DIR = vtkGetTempDir()

import sys
import os

testfiles = [
    ["minimal.nii.gz", "out_minimal.nii.gz"],
    ["minimal.img.gz", "out_minimal.hdr"]
]

dispfile = "avg152T1_RL_nifti.nii.gz"

def TestDisplay(file1):
    """Display the output"""

    inpath = os.path.join(str(VTK_DATA_ROOT), "Data", file1)

    reader = vtk.vtkNIFTIImageReader()
    reader.SetFileName(inpath)
    reader.Update()

    size = reader.GetOutput().GetDimensions()
    center = reader.GetOutput().GetCenter()
    spacing = reader.GetOutput().GetSpacing()
    center1 = (center[0], center[1], center[2])
    center2 = (center[0], center[1], center[2])
    if size[2] % 2 == 1:
        center1 = (center[0], center[1], center[2] + 0.5*spacing[2])
    if size[0] % 2 == 1:
        center2 = (center[0] + 0.5*spacing[0], center[1], center[2])
    vrange = reader.GetOutput().GetScalarRange()

    map1 = vtk.vtkImageSliceMapper()
    map1.BorderOn()
    map1.SliceAtFocalPointOn()
    map1.SliceFacesCameraOn()
    map1.SetInputConnection(reader.GetOutputPort())
    map2 = vtk.vtkImageSliceMapper()
    map2.BorderOn()
    map2.SliceAtFocalPointOn()
    map2.SliceFacesCameraOn()
    map2.SetInputConnection(reader.GetOutputPort())

    slice1 = vtk.vtkImageSlice()
    slice1.SetMapper(map1)
    slice1.GetProperty().SetColorWindow(vrange[1]-vrange[0])
    slice1.GetProperty().SetColorLevel(0.5*(vrange[0]+vrange[1]))
    slice2 = vtk.vtkImageSlice()
    slice2.SetMapper(map2)
    slice2.GetProperty().SetColorWindow(vrange[1]-vrange[0])
    slice2.GetProperty().SetColorLevel(0.5*(vrange[0]+vrange[1]))

    ratio = size[0]*1.0/(size[0]+size[2])

    ren1 = vtk.vtkRenderer()
    ren1.SetViewport(0,0,ratio,1.0)
    ren2 = vtk.vtkRenderer()
    ren2.SetViewport(ratio,0.0,1.0,1.0)
    ren1.AddViewProp(slice1)
    ren2.AddViewProp(slice2)

    cam1 = ren1.GetActiveCamera()
    cam1.ParallelProjectionOn()
    cam1.SetParallelScale(0.5*spacing[1]*size[1])
    cam1.SetFocalPoint(center1[0], center1[1], center1[2])
    cam1.SetPosition(center1[0], center1[1], center1[2] - 100.0)

    cam2 = ren2.GetActiveCamera()
    cam2.ParallelProjectionOn()
    cam2.SetParallelScale(0.5*spacing[1]*size[1])
    cam2.SetFocalPoint(center2[0], center2[1], center2[2])
    cam2.SetPosition(center2[0] + 100.0, center2[1], center2[2])

    if "-I" in sys.argv:
        style = vtk.vtkInteractorStyleImage()
        style.SetInteractionModeToImageSlicing()

        iren = vtk.vtkRenderWindowInteractor()
        iren.SetInteractorStyle(style)

    renwin = vtk.vtkRenderWindow()
    renwin.SetSize(size[0] + size[2], size[1])
    renwin.AddRenderer(ren1)
    renwin.AddRenderer(ren2)

    renwin.Render()

    if "-I" in sys.argv:
        renwin.SetInteractor(iren)
        iren.Initialize()
        iren.Start()

    return renwin

def TestReadWriteRead(infile, outfile):
    """Read, write, and re-read a file, return difference."""

    inpath = os.path.join(str(VTK_DATA_ROOT), "Data", infile)
    outpath = os.path.join(str(VTK_TEMP_DIR), outfile)

    # read a NIFTI file
    reader = vtk.vtkNIFTIImageReader()
    reader.SetFileName(inpath)
    reader.TimeAsVectorOn()
    reader.Update()

    writer = vtk.vtkNIFTIImageWriter()
    writer.SetInputConnection(reader.GetOutputPort())
    writer.SetFileName(outpath)
    # copy most information directoy from the header
    writer.SetNIFTIHeader(reader.GetNIFTIHeader())
    # this information will override the reader's header
    writer.SetQFac(reader.GetQFac())
    writer.SetTimeDimension(reader.GetTimeDimension())
    writer.SetQFormMatrix(reader.GetQFormMatrix())
    writer.SetSFormMatrix(reader.GetSFormMatrix())
    writer.Write()

    reader2 = vtk.vtkNIFTIImageReader()
    reader2.SetFileName(outpath)
    reader2.TimeAsVectorOn()
    reader2.Update()

    diff = vtk.vtkImageMathematics()
    diff.SetOperationToSubtract()
    diff.SetInputConnection(0,reader.GetOutputPort())
    diff.SetInputConnection(1,reader2.GetOutputPort())
    diff.Update()
    diffrange = diff.GetOutput().GetScalarRange()
    differr = diffrange[0]**2 + diffrange[1]**2

    return differr

for infile, outfile in testfiles:
    err = TestReadWriteRead(infile, outfile)
    if err:
        sys.stderr.write(
            "Input " + infile + " differs from outfile " + outfile)
        sys.exit(1)

renWin = TestDisplay(dispfile)
