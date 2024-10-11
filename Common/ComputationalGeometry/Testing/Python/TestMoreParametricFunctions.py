#!/usr/bin/env python
# -*- coding: utf-8 -*-

import math

from vtkmodules.vtkCommonComputationalGeometry import (
    vtkParametricBohemianDome,
    vtkParametricBour,
    vtkParametricCatalanMinimal,
    vtkParametricHenneberg,
    vtkParametricKuen,
    vtkParametricPluckerConoid,
    vtkParametricPseudosphere,
)
from vtkmodules.vtkFiltersSources import vtkParametricFunctionSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkActor2D,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTextMapper,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing

# ------------------------------------------------------------
# Purpose: Test more parametric functions.
# ------------------------------------------------------------

class TestMoreParametricFunctions(vtkmodules.test.Testing.vtkTest):
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
        kuen = vtkParametricKuen()
        kuenSource = vtkParametricFunctionSource()
        kuenSource.SetParametricFunction(kuen)
        kuenSource.SetScalarModeToU()

        kuenMapper = vtkPolyDataMapper()
        kuenMapper.SetInputConnection(kuenSource.GetOutputPort())

        kuenActor = vtkActor()
        kuenActor.SetMapper(kuenMapper)
        kuenActor.SetPosition(0, -19, 0)
        kuenActor.RotateX(90)

        kuenTextMapper = vtkTextMapper()
        kuenTextMapper.SetInput("Kuen")
        kuenTextMapper.GetTextProperty().SetJustificationToCentered()
        kuenTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        kuenTextMapper.GetTextProperty().SetColor(1, 0, 0)
        kuenTextMapper.GetTextProperty().SetFontSize(14)
        kuenTextActor = vtkActor2D()
        kuenTextActor.SetMapper(kuenTextMapper)
        kuenTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        kuenTextActor.GetPositionCoordinate().SetValue(0, -22.5, 0)

        # ------------------------------------------------------------
        # Create a Pseudosphere
        # ------------------------------------------------------------
        pseudo = vtkParametricPseudosphere()
        pseudo.SetMinimumU(-3)
        pseudo.SetMaximumU(3)
        pseudoSource = vtkParametricFunctionSource()
        pseudoSource.SetParametricFunction(pseudo)
        pseudoSource.SetScalarModeToY()

        pseudoMapper = vtkPolyDataMapper()
        pseudoMapper.SetInputConnection(pseudoSource.GetOutputPort())

        pseudoActor = vtkActor()
        pseudoActor.SetMapper(pseudoMapper)
        pseudoActor.SetPosition(8, -19, 0)
        pseudoActor.RotateX(90)

        pseudoTextMapper = vtkTextMapper()
        pseudoTextMapper.SetInput("Pseudosphere")
        pseudoTextMapper.GetTextProperty().SetJustificationToCentered()
        pseudoTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        pseudoTextMapper.GetTextProperty().SetColor(1, 0, 0)
        pseudoTextMapper.GetTextProperty().SetFontSize(14)
        pseudoTextActor = vtkActor2D()
        pseudoTextActor.SetMapper(pseudoTextMapper)
        pseudoTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        pseudoTextActor.GetPositionCoordinate().SetValue(8, -22.5, 0)

        # ------------------------------------------------------------
        # Create a Bohemian Dome
        # ------------------------------------------------------------
        bdome = vtkParametricBohemianDome()
        bdomeSource = vtkParametricFunctionSource()
        bdomeSource.SetParametricFunction(bdome)
        bdomeSource.SetScalarModeToU()

        bdomeMapper = vtkPolyDataMapper()
        bdomeMapper.SetInputConnection(bdomeSource.GetOutputPort())

        bdomeActor = vtkActor()
        bdomeActor.SetMapper(bdomeMapper)
        bdomeActor.SetPosition(16, -19, 0)
        bdomeActor.RotateY(90)

        bdomeTextMapper = vtkTextMapper()
        bdomeTextMapper.SetInput("Bohemian Dome")
        bdomeTextMapper.GetTextProperty().SetJustificationToCentered()
        bdomeTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        bdomeTextMapper.GetTextProperty().SetColor(1, 0, 0)
        bdomeTextMapper.GetTextProperty().SetFontSize(14)
        bdomeTextActor = vtkActor2D()
        bdomeTextActor.SetMapper(bdomeTextMapper)
        bdomeTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        bdomeTextActor.GetPositionCoordinate().SetValue(16, -22.5, 0)

        # ------------------------------------------------------------
        # Create Henneberg's Minimal Surface
        # ------------------------------------------------------------
        hberg = vtkParametricHenneberg()
        hberg.SetMinimumU(-.3)
        hberg.SetMaximumU(.3)
        hbergSource = vtkParametricFunctionSource()
        hbergSource.SetParametricFunction(hberg)
        hbergSource.SetScalarModeToV()

        hbergMapper = vtkPolyDataMapper()
        hbergMapper.SetInputConnection(hbergSource.GetOutputPort())

        hbergActor = vtkActor()
        hbergActor.SetMapper(hbergMapper)
        hbergActor.SetPosition(24, -19, 0)
        hbergActor.RotateY(90)

        hbergTextMapper = vtkTextMapper()
        hbergTextMapper.SetInput("Henneberg")
        hbergTextMapper.GetTextProperty().SetJustificationToCentered()
        hbergTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        hbergTextMapper.GetTextProperty().SetColor(1, 0, 0)
        hbergTextMapper.GetTextProperty().SetFontSize(14)
        hbergTextActor = vtkActor2D()
        hbergTextActor.SetMapper(hbergTextMapper)
        hbergTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        hbergTextActor.GetPositionCoordinate().SetValue(24, -22.5, 0)

        # ------------------------------------------------------------
        # Create Catalan's Minimal Surface
        # ------------------------------------------------------------
        catalan = vtkParametricCatalanMinimal()
        catalan.SetMinimumU(-2.*math.pi)
        catalan.SetMaximumU( 2.*math.pi)
        catalanSource = vtkParametricFunctionSource()
        catalanSource.SetParametricFunction(catalan)
        catalanSource.SetScalarModeToV()

        catalanMapper = vtkPolyDataMapper()
        catalanMapper.SetInputConnection(catalanSource.GetOutputPort())

        catalanActor = vtkActor()
        catalanActor.SetMapper(catalanMapper)
        catalanActor.SetPosition(0, -27, 0)
        catalanActor.SetScale(.5, .5, .5)

        catalanTextMapper = vtkTextMapper()
        catalanTextMapper.SetInput("Catalan")
        catalanTextMapper.GetTextProperty().SetJustificationToCentered()
        catalanTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        catalanTextMapper.GetTextProperty().SetColor(1, 0, 0)
        catalanTextMapper.GetTextProperty().SetFontSize(14)
        catalanTextActor = vtkActor2D()
        catalanTextActor.SetMapper(catalanTextMapper)
        catalanTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        catalanTextActor.GetPositionCoordinate().SetValue(0, -30.5, 0)

        # ------------------------------------------------------------
        # Create Bour's Minimal Surface
        # ------------------------------------------------------------
        bour = vtkParametricBour()
        bourSource = vtkParametricFunctionSource()
        bourSource.SetParametricFunction(bour)
        bourSource.SetScalarModeToU()

        bourMapper = vtkPolyDataMapper()
        bourMapper.SetInputConnection(bourSource.GetOutputPort())

        bourActor = vtkActor()
        bourActor.SetMapper(bourMapper)
        bourActor.SetPosition(8, -27, 0)

        bourTextMapper = vtkTextMapper()
        bourTextMapper.SetInput("Bour")
        bourTextMapper.GetTextProperty().SetJustificationToCentered()
        bourTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        bourTextMapper.GetTextProperty().SetColor(1, 0, 0)
        bourTextMapper.GetTextProperty().SetFontSize(14)
        bourTextActor = vtkActor2D()
        bourTextActor.SetMapper(bourTextMapper)
        bourTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        bourTextActor.GetPositionCoordinate().SetValue(8, -30.5, 0)

        # ------------------------------------------------------------
        # Create Plucker's Conoid Surface
        # ------------------------------------------------------------
        plucker = vtkParametricPluckerConoid()
        pluckerSource = vtkParametricFunctionSource()
        pluckerSource.SetParametricFunction(plucker)
        pluckerSource.SetScalarModeToZ()

        pluckerMapper = vtkPolyDataMapper()
        pluckerMapper.SetInputConnection(pluckerSource.GetOutputPort())

        pluckerActor = vtkActor()
        pluckerActor.SetMapper(pluckerMapper)
        pluckerActor.SetPosition(16, -27, 0)

        pluckerTextMapper = vtkTextMapper()
        pluckerTextMapper.SetInput("Plucker")
        pluckerTextMapper.GetTextProperty().SetJustificationToCentered()
        pluckerTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        pluckerTextMapper.GetTextProperty().SetColor(1, 0, 0)
        pluckerTextMapper.GetTextProperty().SetFontSize(14)
        pluckerTextActor = vtkActor2D()
        pluckerTextActor.SetMapper(pluckerTextMapper)
        pluckerTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        pluckerTextActor.GetPositionCoordinate().SetValue(16, -30.5, 0)

        # ------------------------------------------------------------
        # Create the RenderWindow, Renderer and all vtkActors
        # ------------------------------------------------------------
        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)
        iren = vtkRenderWindowInteractor()
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
        vtkmodules.test.Testing.compareImage(iren.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
    vtkmodules.test.Testing.main([(TestMoreParametricFunctions, 'test')])
