#!/usr/bin/env python

# Test interactions of scalar coloring with various ScalarMaterialMode
# settings.
import vtk

renWin = vtk.vtkRenderWindow()
iRen = vtk.vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin)
renWin.SetSize(500, 600)

sphere = vtk.vtkSphereSource()

elev = vtk.vtkElevationFilter()
elev.SetLowPoint(-0.25, -0.25, -0.25)
elev.SetHighPoint(.25, .25, .25)
elev.SetInputConnection(sphere.GetOutputPort())

lut = vtk.vtkLookupTable()
lut.SetSaturationRange(0, 0)
lut.SetValueRange(0, 1)
lut.SetRange(0, 1)
lut.Build()


def add(**kwargs):
    ren = vtk.vtkRenderer()
    ren.SetBackground(0.5, 0.5, 0.5)
    #ren.SetViewport(0, 0, 0.5, 1)
    renWin.AddRenderer(ren)

    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputConnection(elev.GetOutputPort())
    mapper.SetLookupTable(lut)
    mapper.SetInterpolateScalarsBeforeMapping(kwargs['interpolate_scalars_before_mapping'])
    mapper.SetScalarMaterialMode(kwargs["material_mode"])

    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    ren.AddActor(actor)

    prop = actor.GetProperty()
    prop.SetAmbient(kwargs['ambient'])
    prop.SetDiffuse(kwargs['diffuse'])
    prop.SetAmbientColor(1, 0, 0)
    prop.SetDiffuseColor(0, 1, 0)

    textActorL = vtk.vtkTextActor()
    txt = " InterpolateScalarsBeforeMapping: %d\n"\
          " ScalarMaterialMode: %s\n"\
          " Ambient: %.2f\t Ambient Color: 1, 0, 0\n"\
          " Diffuse: %.2f\t Diffuse Color: 0, 1, 0"
    txt = txt % (kwargs['interpolate_scalars_before_mapping'],
            mapper.GetScalarMaterialModeAsString(), prop.GetAmbient(), prop.GetDiffuse())
    textActorL.SetInput(txt)
    ren.AddActor(textActorL)
    return (ren, actor, mapper)

ren, actor, mapper = add(interpolate_scalars_before_mapping=1, ambient=0.49,
        diffuse=0.51, material_mode=vtk.VTK_MATERIALMODE_DEFAULT)
ren.SetViewport(0, 0, 0.5, 0.25)

ren, actor, mapper = add(interpolate_scalars_before_mapping=0, ambient=0.49,
        diffuse=0.51, material_mode=vtk.VTK_MATERIALMODE_DEFAULT)
ren.SetViewport(0.5, 0, 1, 0.25)

ren, actor, mapper = add(interpolate_scalars_before_mapping=1, ambient=0.49,
        diffuse=0.51, material_mode=vtk.VTK_MATERIALMODE_AMBIENT)
ren.SetViewport(0, 0.25, 0.5, 0.5)

ren, actor, mapper = add(interpolate_scalars_before_mapping=0, ambient=0.49,
        diffuse=0.51, material_mode=vtk.VTK_MATERIALMODE_AMBIENT)
ren.SetViewport(0.5, 0.25, 1, 0.5)

ren, actor, mapper = add(interpolate_scalars_before_mapping=1, ambient=0.49,
        diffuse=0.51, material_mode=vtk.VTK_MATERIALMODE_DIFFUSE)
ren.SetViewport(0, 0.5, 0.5, 0.75)

ren, actor, mapper = add(interpolate_scalars_before_mapping=0, ambient=0.49,
        diffuse=0.51, material_mode=vtk.VTK_MATERIALMODE_DIFFUSE)
ren.SetViewport(0.5, 0.5, 1, 0.75)

ren, actor, mapper = add(interpolate_scalars_before_mapping=1, ambient=0.49,
        diffuse=0.51, material_mode=vtk.VTK_MATERIALMODE_AMBIENT_AND_DIFFUSE)
ren.SetViewport(0, 0.75, 0.5, 1)

ren, actor, mapper = add(interpolate_scalars_before_mapping=0, ambient=0.49,
        diffuse=0.51, material_mode=vtk.VTK_MATERIALMODE_AMBIENT_AND_DIFFUSE)
ren.SetViewport(0.5, 0.75, 1, 1)

#---------------------------------------------------------
