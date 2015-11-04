#!/usr/bin/env python
# -*- coding: utf-8 -*-

import math

import vtk
import vtk.test.Testing

# ------------------------------------------------------------
# Purpose: Test more parametric functions.
# ------------------------------------------------------------

class TestMoreParametricFunctions(vtk.test.Testing.vtkTest):
    def testMoreParametricFunctions(self):
        # ------------------------------------------------------------
        # For each parametric surface:
        # 1) Create it
        # 2) Assign mappers and actors
        # 3) Position the object
        # 5) Add a label
        # ------------------------------------------------------------

        # ------------------------------------------------------------
        # Create Kuen's Surface.
        # ------------------------------------------------------------
        kuen = vtk.vtkParametricKuen()
        kuenSource = vtk.vtkParametricFunctionSource()
        kuenSource.SetParametricFunction(kuen)
        kuenSource.SetScalarModeToU()

        kuenMapper = vtk.vtkPolyDataMapper()
        kuenMapper.SetInputConnection(kuenSource.GetOutputPort())

        kuenActor = vtk.vtkActor()
        kuenActor.SetMapper(kuenMapper)
        kuenActor.SetPosition(0, -19, 0)
        kuenActor.RotateX(90)

        kuenTextMapper = vtk.vtkTextMapper()
        kuenTextMapper.SetInput("Kuen")
        kuenTextMapper.GetTextProperty().SetJustificationToCentered()
        kuenTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        kuenTextMapper.GetTextProperty().SetColor(1, 0, 0)
        kuenTextMapper.GetTextProperty().SetFontSize(14)
        kuenTextActor = vtk.vtkActor2D()
        kuenTextActor.SetMapper(kuenTextMapper)
        kuenTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        kuenTextActor.GetPositionCoordinate().SetValue(0, -22.5, 0)

        # ------------------------------------------------------------
        # Create a Pseudosphere
        # ------------------------------------------------------------
        pseudo = vtk.vtkParametricPseudosphere()
        pseudo.SetMinimumU(-3)
        pseudo.SetMaximumU(3)
        pseudoSource = vtk.vtkParametricFunctionSource()
        pseudoSource.SetParametricFunction(pseudo)
        pseudoSource.SetScalarModeToY()

        pseudoMapper = vtk.vtkPolyDataMapper()
        pseudoMapper.SetInputConnection(pseudoSource.GetOutputPort())

        pseudoActor = vtk.vtkActor()
        pseudoActor.SetMapper(pseudoMapper)
        pseudoActor.SetPosition(8, -19, 0)
        pseudoActor.RotateX(90)

        pseudoTextMapper = vtk.vtkTextMapper()
        pseudoTextMapper.SetInput("Pseudosphere")
        pseudoTextMapper.GetTextProperty().SetJustificationToCentered()
        pseudoTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        pseudoTextMapper.GetTextProperty().SetColor(1, 0, 0)
        pseudoTextMapper.GetTextProperty().SetFontSize(14)
        pseudoTextActor = vtk.vtkActor2D()
        pseudoTextActor.SetMapper(pseudoTextMapper)
        pseudoTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        pseudoTextActor.GetPositionCoordinate().SetValue(8, -22.5, 0)

        # ------------------------------------------------------------
        # Create a Bohemian Dome
        # ------------------------------------------------------------
        bdome = vtk.vtkParametricBohemianDome()
        bdomeSource = vtk.vtkParametricFunctionSource()
        bdomeSource.SetParametricFunction(bdome)
        bdomeSource.SetScalarModeToU()

        bdomeMapper = vtk.vtkPolyDataMapper()
        bdomeMapper.SetInputConnection(bdomeSource.GetOutputPort())

        bdomeActor = vtk.vtkActor()
        bdomeActor.SetMapper(bdomeMapper)
        bdomeActor.SetPosition(16, -19, 0)
        bdomeActor.RotateY(90)

        bdomeTextMapper = vtk.vtkTextMapper()
        bdomeTextMapper.SetInput("Bohemian Dome")
        bdomeTextMapper.GetTextProperty().SetJustificationToCentered()
        bdomeTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        bdomeTextMapper.GetTextProperty().SetColor(1, 0, 0)
        bdomeTextMapper.GetTextProperty().SetFontSize(14)
        bdomeTextActor = vtk.vtkActor2D()
        bdomeTextActor.SetMapper(bdomeTextMapper)
        bdomeTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        bdomeTextActor.GetPositionCoordinate().SetValue(16, -22.5, 0)

        # ------------------------------------------------------------
        # Create Henneberg's Minimal Surface
        # ------------------------------------------------------------
        hberg = vtk.vtkParametricHenneberg()
        hberg.SetMinimumU(-.3)
        hberg.SetMaximumU(.3)
        hbergSource = vtk.vtkParametricFunctionSource()
        hbergSource.SetParametricFunction(hberg)
        hbergSource.SetScalarModeToV()

        hbergMapper = vtk.vtkPolyDataMapper()
        hbergMapper.SetInputConnection(hbergSource.GetOutputPort())

        hbergActor = vtk.vtkActor()
        hbergActor.SetMapper(hbergMapper)
        hbergActor.SetPosition(24, -19, 0)
        hbergActor.RotateY(90)

        hbergTextMapper = vtk.vtkTextMapper()
        hbergTextMapper.SetInput("Henneberg")
        hbergTextMapper.GetTextProperty().SetJustificationToCentered()
        hbergTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        hbergTextMapper.GetTextProperty().SetColor(1, 0, 0)
        hbergTextMapper.GetTextProperty().SetFontSize(14)
        hbergTextActor = vtk.vtkActor2D()
        hbergTextActor.SetMapper(hbergTextMapper)
        hbergTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        hbergTextActor.GetPositionCoordinate().SetValue(24, -22.5, 0)

        # ------------------------------------------------------------
        # Create Catalan's Minimal Surface
        # ------------------------------------------------------------
        catalan = vtk.vtkParametricCatalanMinimal()
        catalan.SetMinimumU(-2.*math.pi)
        catalan.SetMaximumU( 2.*math.pi)
        catalanSource = vtk.vtkParametricFunctionSource()
        catalanSource.SetParametricFunction(catalan)
        catalanSource.SetScalarModeToV()

        catalanMapper = vtk.vtkPolyDataMapper()
        catalanMapper.SetInputConnection(catalanSource.GetOutputPort())

        catalanActor = vtk.vtkActor()
        catalanActor.SetMapper(catalanMapper)
        catalanActor.SetPosition(0, -27, 0)
        catalanActor.SetScale(.5, .5, .5)

        catalanTextMapper = vtk.vtkTextMapper()
        catalanTextMapper.SetInput("Catalan")
        catalanTextMapper.GetTextProperty().SetJustificationToCentered()
        catalanTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        catalanTextMapper.GetTextProperty().SetColor(1, 0, 0)
        catalanTextMapper.GetTextProperty().SetFontSize(14)
        catalanTextActor = vtk.vtkActor2D()
        catalanTextActor.SetMapper(catalanTextMapper)
        catalanTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        catalanTextActor.GetPositionCoordinate().SetValue(0, -30.5, 0)

        # ------------------------------------------------------------
        # Create Bour's Minimal Surface
        # ------------------------------------------------------------
        bour = vtk.vtkParametricBour()
        bourSource = vtk.vtkParametricFunctionSource()
        bourSource.SetParametricFunction(bour)
        bourSource.SetScalarModeToU()

        bourMapper = vtk.vtkPolyDataMapper()
        bourMapper.SetInputConnection(bourSource.GetOutputPort())

        bourActor = vtk.vtkActor()
        bourActor.SetMapper(bourMapper)
        bourActor.SetPosition(8, -27, 0)

        bourTextMapper = vtk.vtkTextMapper()
        bourTextMapper.SetInput("Bour")
        bourTextMapper.GetTextProperty().SetJustificationToCentered()
        bourTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        bourTextMapper.GetTextProperty().SetColor(1, 0, 0)
        bourTextMapper.GetTextProperty().SetFontSize(14)
        bourTextActor = vtk.vtkActor2D()
        bourTextActor.SetMapper(bourTextMapper)
        bourTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        bourTextActor.GetPositionCoordinate().SetValue(8, -30.5, 0)

        # ------------------------------------------------------------
        # Create Plucker's Conoid Surface
        # ------------------------------------------------------------
        plucker = vtk.vtkParametricPluckerConoid()
        pluckerSource = vtk.vtkParametricFunctionSource()
        pluckerSource.SetParametricFunction(plucker)
        pluckerSource.SetScalarModeToZ()

        pluckerMapper = vtk.vtkPolyDataMapper()
        pluckerMapper.SetInputConnection(pluckerSource.GetOutputPort())

        pluckerActor = vtk.vtkActor()
        pluckerActor.SetMapper(pluckerMapper)
        pluckerActor.SetPosition(16, -27, 0)

        pluckerTextMapper = vtk.vtkTextMapper()
        pluckerTextMapper.SetInput("Plucker")
        pluckerTextMapper.GetTextProperty().SetJustificationToCentered()
        pluckerTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        pluckerTextMapper.GetTextProperty().SetColor(1, 0, 0)
        pluckerTextMapper.GetTextProperty().SetFontSize(14)
        pluckerTextActor = vtk.vtkActor2D()
        pluckerTextActor.SetMapper(pluckerTextMapper)
        pluckerTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        pluckerTextActor.GetPositionCoordinate().SetValue(16, -30.5, 0)

        # ------------------------------------------------------------
        # Create the RenderWindow, Renderer and all vtkActors
        # ------------------------------------------------------------
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)
        iren = vtk.vtkRenderWindowInteractor()
        iren.SetRenderWindow(renWin)

        # add actors
        ren.AddViewProp(kuenActor)
        ren.AddViewProp(pseudoActor)
        ren.AddViewProp(bdomeActor)
        ren.AddViewProp(hbergActor)
        ren.AddViewProp(catalanActor)
        ren.AddViewProp(bourActor)
        ren.AddViewProp(pluckerActor)
        #add text actors
        ren.AddViewProp(kuenTextActor)
        ren.AddViewProp(pseudoTextActor)
        ren.AddViewProp(bdomeTextActor)
        ren.AddViewProp(hbergTextActor)
        ren.AddViewProp(catalanTextActor)
        ren.AddViewProp(bourTextActor)
        ren.AddViewProp(pluckerTextActor)

        ren.SetBackground(0.9, 0.9, 0.9)
        renWin.SetSize(500, 500)
        ren.ResetCamera()

        iren.Initialize()
        renWin.Render()

        img_file = "TestMoreParametricFunctions.png"
        vtk.test.Testing.compareImage(iren.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=10)
        vtk.test.Testing.interact()

if __name__ == "__main__":
    vtk.test.Testing.main([(TestMoreParametricFunctions, 'test')])
