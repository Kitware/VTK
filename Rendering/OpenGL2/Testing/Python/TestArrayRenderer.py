# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
from vtkmodules.vtkCommonCore import vtkStringToken
from vtkmodules.vtkCommonDataModel import vtkCellGrid
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
from vtkmodules.vtkFiltersSources import vtkCubeSource, vtkOutlineCornerFilter
from vtkmodules.vtkFiltersCellGrid import vtkCellGridComputeSides
from vtkmodules.vtkRenderingCore import vtkRenderWindow, vtkRenderer, vtkActor, vtkPolyDataMapper
from vtkmodules.vtkRenderingOpenGL2 import vtkArrayRenderer, vtkShader, vtkShaderProgram
from vtkmodules.vtkInteractionWidgets import vtkCameraOrientationWidget
from vtkmodules.vtkIOCellGrid import vtkCellGridReader
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

from vtkmodules.test import Testing

# Add the directory containing this file to the python path
# so we can import the shader source from the subdirectory:
import os
import sys
sys.path.append(os.path.dirname(__file__))
from shaders import vertShaderSource, fragShaderSource

filename = os.path.join(VTK_DATA_ROOT, 'Data', 'dgTetrahedra.dg')
cellAttName = None
cellAttType = None
interact = '-I' in sys.argv
if len(sys.argv) > 1:
    if sys.argv[1] != '-D':
        interact = True # Always run interactively if no data directory.
        # When run manually, accept a filename, an attribute name (for
        # coloring), and an attribute type.
        filename = sys.argv[1]
        print('Read', filename)
        if len(sys.argv) > 2:
            cellAttName = vtkStringToken(sys.argv[2])
            print('Color by', cellAttName.Data())
        else:
            cellAttName = None
        if len(sys.argv) > 3:
            cellAttType = vtkStringToken(sys.argv[3])
        else:
            cellAttType = None

# Create objects for a scene.
rw = vtkRenderWindow()
ri = rw.MakeRenderWindowInteractor()
rr = vtkRenderer()
ac = vtkActor()
ar = vtkArrayRenderer()
cr = vtkCellGridReader()
cs = vtkCellGridComputeSides()
ow = vtkCameraOrientationWidget()

# Configure the pipeline
rw.AddRenderer(rr)
rr.AddActor(ac)
cr.SetFileName(filename)
cs.SetInputConnection(cr.GetOutputPort())
cs.Update()
# Grab the cell-grid object representing the surface to render.
# We'll use arrays from the cell-grid to define the rendering.
cg = cs.GetOutputDataObject(0)
if cellAttName == None:
    cellAttName = vtkStringToken('scalar2')
if cellAttType == None:
    cellAttType = vtkStringToken('DG HGRAD C1')
bds = [0., 0., 0., 0., 0., 0.]
cg.GetBounds(bds)
print('Bounds are ', bds)
cg.GetUnorderedCellAttributeIds()
for cellTypeName in cg.GetCellTypes():
    print('  %s' % cellTypeName)
ac.SetMapper(ar)
ar.SetInputDataObject(0, cg)
ar.SetNumberOfElements(1)
ar.SetElementType(vtkArrayRenderer.Triangle)
ar.SetVertexShaderSource(vertShaderSource)
ar.SetFragmentShaderSource(fragShaderSource)

# Bind some arrays from the dataset.

# 0. Bind tetrahedron-specific metadata texture
side_offsets = cg.GetCellType('vtkDGTet').GetSideOffsetsAndShapes()
ar.BindArrayToTexture('side_offsets', side_offsets)
side_local = cg.GetCellType('vtkDGTet').GetSideConnectivity()
ar.BindArrayToTexture('side_local', side_local)
cell_parametrics = cg.GetCellType('vtkDGTet').GetReferencePoints()
ar.BindArrayToTexture('cell_parametrics', cell_parametrics)

# 1. Bind (cell,side) tuples
sideConn = cg.GetAttributes(vtkStringToken('triangle sides of vtkDGTet')).GetScalars()
ar.BindArrayToTexture(vtkStringToken('sides'), sideConn)
ar.SetNumberOfInstances(sideConn.GetNumberOfTuples())

# 2. Bind attribute data (connectivity and value arrays)
fr = [1, -1] # field range (defaulted to invalid range)
for attId in cg.GetUnorderedCellAttributeIds():
    cellAtt = cg.GetCellAttributeById(attId)
    print(cellAtt.GetName().Data())
    conn = cellAtt.GetArrayForCellTypeAndRole('vtkDGTet', 'connectivity')
    vals = cellAtt.GetArrayForCellTypeAndRole('vtkDGTet', 'values')
    # Note: Since we are rendering a single cell-shape at a time, we do
    #       not include the cell type-name in the texture sampler name.
    cname = vtkStringToken(cellAtt.GetName().Data() + '_conn')
    vname = vtkStringToken(cellAtt.GetName().Data() + '_vals')
    # ar.BindArrayToTexture(cname, conn, True) # Reshape to a 1-D array with 1 component per tuple.
    # ar.BindArrayToTexture(vname, vals, False)
    # print('  Binding "%s" and "%s"' % (cname.Data(), vname.Data()))
    if cellAtt.GetName() == cellAttName:
        cname = vtkStringToken('field_conn')
        vname = vtkStringToken('field_vals')
        ar.BindArrayToTexture(cname, conn, True) # Reshape to a 1-D array with 1 component per tuple.
        ar.BindArrayToTexture(vname, vals, True) # Always reshape since #comps can be > 4.
        ar.PrepareColormap(None)
        # Get the range of the field and, if invalid, reset to a valid default range centered around 0.
        cg.GetCellAttributeRange(cellAtt, 0, fr, True)
        if fr[0] > fr[1]:
            fr = [-1e-11, 1e-11]
        print('fr', fr)
        ac.GetShaderProperty().GetFragmentCustomUniforms().SetUniform3f('field_range', (fr[0], fr[1], fr[1] - fr[0]))
        print('  Binding "%s" and "%s" for "%s"' % \
            (cname.Data(), vname.Data(), cellAttName.Data()))
        print('    Range [%g, %g] %g' % (fr[0], fr[1], fr[1] - fr[0]))
    elif cellAtt == cg.GetShapeAttribute():
        cname = vtkStringToken('shape_conn')
        vname = vtkStringToken('shape_vals')
        ar.BindArrayToTexture(cname, conn, True) # Reshape to a 1-D array with 1 component per tuple.
        ar.BindArrayToTexture(vname, vals, False)
        print('  Binding "%s" and "%s" for "%s", shape_vals size (%d vals x %d bytes/val]' % (cname.Data(), vname.Data(), cellAtt.GetName().Data(), vals.GetMaxId()+1, vals.GetDataTypeSize()))

# Add a unit cube centered at the origin to the scene
cs = vtkCubeSource()
cf = vtkOutlineCornerFilter()
ca = vtkActor()
cm = vtkPolyDataMapper()
ca.SetMapper(cm)
cf.SetInputConnection(cs.GetOutputPort())
cm.SetInputConnection(cf.GetOutputPort())
rr.AddActor(ca)

# Final scene prep
rr.ResetCamera()
rr.SetBackground(0.5, 0.5, 0.8) # Something that both black and white contrast with.
ow.SetParentRenderer(rr)
ri.Initialize()
ow.On()
rw.Render()

# If you want to see the final shader sources with all of VTK's
# automated replacements, uncomment this line. You can also edit
# the files and re-run to try our modifications. Just be careful
# as the existence of these files will affect future runs of
# this script if they exist.
# ar.GetShaderProgram().SetFileNamePrefixForDebugging('/tmp/foo')

# Run the interactor
if interact:
    ri.Start()
else:
    Testing.processCmdLine()
    Testing.compareImage(rw, Testing.getAbsImagePath('TestArrayRenderer.png'))
