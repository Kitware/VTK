#!/usr/bin/env python
# -*- coding: utf-8 -*-

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# ------------------------------------------------------------
# Purpose: Test the parametric functions.
# ------------------------------------------------------------

class TestParametricFunctions(vtk.test.Testing.vtkTest):
    def testParametricFunctions(self):
        # ------------------------------------------------------------
        # Get a texture
        # ------------------------------------------------------------
        textureReader = vtk.vtkJPEGReader()
        textureReader.SetFileName(VTK_DATA_ROOT + "/Data/beach.jpg")
        texture = vtk.vtkTexture()
        texture.SetInputConnection(textureReader.GetOutputPort())

        # ------------------------------------------------------------
        # For each parametric surface:
        # 1) Create it
        # 2) Assign mappers and actors
        # 3) Position the object
        # 5) Add a label
        # ------------------------------------------------------------

        # ------------------------------------------------------------
        # Create a torus
        # ------------------------------------------------------------
        torus = vtk.vtkParametricTorus()
        torusSource = vtk.vtkParametricFunctionSource()
        torusSource.SetParametricFunction(torus)
        torusSource.SetScalarModeToPhase()

        torusMapper = vtk.vtkPolyDataMapper()
        torusMapper.SetInputConnection(torusSource.GetOutputPort())
        torusMapper.SetScalarRange(0, 360)

        torusActor = vtk.vtkActor()
        torusActor.SetMapper(torusMapper)
        torusActor.SetPosition(0, 12, 0)

        torusTextMapper = vtk.vtkTextMapper()
        torusTextMapper.SetInput("Torus")
        torusTextMapper.GetTextProperty().SetJustificationToCentered()
        torusTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        torusTextMapper.GetTextProperty().SetColor(1, 0, 0)
        torusTextMapper.GetTextProperty().SetFontSize(14)
        torusTextActor = vtk.vtkActor2D()
        torusTextActor.SetMapper(torusTextMapper)
        torusTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        torusTextActor.GetPositionCoordinate().SetValue(0, 9.5, 0)

        # ------------------------------------------------------------
        # Create a Klein bottle
        # ------------------------------------------------------------
        klein = vtk.vtkParametricKlein()
        kleinSource = vtk.vtkParametricFunctionSource()
        kleinSource.SetParametricFunction(klein)
        kleinSource.SetScalarModeToU0V0()

        kleinMapper = vtk.vtkPolyDataMapper()
        kleinMapper.SetInputConnection(kleinSource.GetOutputPort())
        kleinMapper.SetScalarRange(0, 3)

        kleinActor = vtk.vtkActor()
        kleinActor.SetMapper(kleinMapper)
        kleinActor.SetPosition(8, 10.5, 0)

        kleinTextMapper = vtk.vtkTextMapper()
        kleinTextMapper.SetInput("Klein")
        kleinTextMapper.GetTextProperty().SetJustificationToCentered()
        kleinTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        kleinTextMapper.GetTextProperty().SetColor(1, 0, 0)
        kleinTextMapper.GetTextProperty().SetFontSize(14)
        kleinTextActor = vtk.vtkActor2D()
        kleinTextActor.SetMapper(kleinTextMapper)
        kleinTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        kleinTextActor.GetPositionCoordinate().SetValue(8, 9.5, 0)

        # ------------------------------------------------------------
        # Create a Figure-8 Klein
        # ------------------------------------------------------------
        klein2 = vtk.vtkParametricFigure8Klein()
        klein2Source = vtk.vtkParametricFunctionSource()
        klein2Source.SetParametricFunction(klein2)
        klein2Source.GenerateTextureCoordinatesOn()

        klein2Mapper = vtk.vtkPolyDataMapper()
        klein2Mapper.SetInputConnection(klein2Source.GetOutputPort())
        klein2Mapper.SetScalarRange(0, 3)

        klein2Actor = vtk.vtkActor()
        klein2Actor.SetMapper(klein2Mapper)
        klein2Actor.SetPosition(16, 12, 0)
        klein2Actor.SetTexture(texture)


        fig8KleinTextMapper = vtk.vtkTextMapper()
        fig8KleinTextMapper.SetInput("Fig-8.Klein")
        fig8KleinTextMapper.GetTextProperty().SetJustificationToCentered()
        fig8KleinTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        fig8KleinTextMapper.GetTextProperty().SetColor(1, 0, 0)
        fig8KleinTextMapper.GetTextProperty().SetFontSize(14)
        fig8KleinTextActor = vtk.vtkActor2D()
        fig8KleinTextActor.SetMapper(fig8KleinTextMapper)
        fig8KleinTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        fig8KleinTextActor.GetPositionCoordinate().SetValue(16, 9.5, 0)

        # ------------------------------------------------------------
        # Create a Mobius strip
        # ------------------------------------------------------------
        mobius = vtk.vtkParametricMobius()
        mobiusSource = vtk.vtkParametricFunctionSource()
        mobiusSource.SetParametricFunction(mobius)
        mobiusSource.GenerateTextureCoordinatesOn()

        mobiusMapper = vtk.vtkPolyDataMapper()
        mobiusMapper.SetInputConnection(mobiusSource.GetOutputPort())

        mobiusActor = vtk.vtkActor()
        mobiusActor.SetMapper(mobiusMapper)
        mobiusActor.RotateX(45)
        mobiusActor.SetPosition(24, 12, 0)
        mobiusActor.SetTexture(texture)

        mobiusTextMapper = vtk.vtkTextMapper()
        mobiusTextMapper.SetInput("Mobius")
        mobiusTextMapper.GetTextProperty().SetJustificationToCentered()
        mobiusTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        mobiusTextMapper.GetTextProperty().SetColor(1, 0, 0)
        mobiusTextMapper.GetTextProperty().SetFontSize(14)
        mobiusTextActor = vtk.vtkActor2D()
        mobiusTextActor.SetMapper(mobiusTextMapper)
        mobiusTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        mobiusTextActor.GetPositionCoordinate().SetValue(24, 9.5, 0)

        # ------------------------------------------------------------
        # Create a super toroid
        # ------------------------------------------------------------
        toroid = vtk.vtkParametricSuperToroid()
        toroid.SetN1(2)
        toroid.SetN2(3)
        toroidSource = vtk.vtkParametricFunctionSource()
        toroidSource.SetParametricFunction(toroid)
        toroidSource.SetScalarModeToU()

        toroidMapper = vtk.vtkPolyDataMapper()
        toroidMapper.SetInputConnection(toroidSource.GetOutputPort())
        toroidMapper.SetScalarRange(0, 6.28)

        toroidActor = vtk.vtkActor()
        toroidActor.SetMapper(toroidMapper)
        toroidActor.SetPosition(0, 4, 0)

        superToroidTextMapper = vtk.vtkTextMapper()
        superToroidTextMapper.SetInput("Super.Toroid")
        superToroidTextMapper.GetTextProperty().SetJustificationToCentered()
        superToroidTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        superToroidTextMapper.GetTextProperty().SetColor(1, 0, 0)
        superToroidTextMapper.GetTextProperty().SetFontSize(14)
        superToroidTextActor = vtk.vtkActor2D()
        superToroidTextActor.SetMapper(superToroidTextMapper)
        superToroidTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        superToroidTextActor.GetPositionCoordinate().SetValue(0, 1.5, 0)

        # ------------------------------------------------------------
        # Create a super ellipsoid
        # ------------------------------------------------------------
        superEllipsoid = vtk.vtkParametricSuperEllipsoid()
        superEllipsoid.SetXRadius(1.25)
        superEllipsoid.SetYRadius(1.5)
        superEllipsoid.SetZRadius(1.0)
        superEllipsoid.SetN1(1.1)
        superEllipsoid.SetN2(1.75)
        superEllipsoidSource = vtk.vtkParametricFunctionSource()
        superEllipsoidSource.SetParametricFunction(superEllipsoid)
        superEllipsoidSource.SetScalarModeToV()

        superEllipsoidMapper = vtk.vtkPolyDataMapper()
        superEllipsoidMapper.SetInputConnection(superEllipsoidSource.GetOutputPort())
        superEllipsoidMapper.SetScalarRange(0, 3.14)

        superEllipsoidActor = vtk.vtkActor()
        superEllipsoidActor.SetMapper(superEllipsoidMapper)
        superEllipsoidActor.SetPosition(8, 4, 0)

        superEllipsoidTextMapper = vtk.vtkTextMapper()
        superEllipsoidTextMapper.SetInput("Super.Ellipsoid")
        superEllipsoidTextMapper.GetTextProperty().SetJustificationToCentered()
        superEllipsoidTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        superEllipsoidTextMapper.GetTextProperty().SetColor(1, 0, 0)
        superEllipsoidTextMapper.GetTextProperty().SetFontSize(14)
        superEllipsoidTextActor = vtk.vtkActor2D()
        superEllipsoidTextActor.SetMapper(superEllipsoidTextMapper)
        superEllipsoidTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        superEllipsoidTextActor.GetPositionCoordinate().SetValue(8, 1.5, 0)

        # ------------------------------------------------------------
        # Create an open 1D spline
        # ------------------------------------------------------------
        splinePoints = [
          [0.50380158308139134, -0.60679315105396936, -0.37248976406291578],
          [-0.4354646054261665, -0.85362339758017258, -0.84844312996065385],
          [0.2163147512899315, -0.39797507012168643, -0.76700353518454523],
          [0.97158415334838644, -0.58513467367046257, -0.35846037946569753],
          [-0.64359767997804918, -0.94620739107309249, -0.90762176546623086],
          [-0.39901219094126117, -0.1978931497772658, 0.0098316934936828471],
          [-0.75872745167404765, 0.067719714281950116, 0.165237936733867],
          [-0.84599731389712418, -0.67685466896596114, 0.10357868909071133],
          [0.84702754758625654, -0.0080077177882230677, -0.58571286666473044],
          [-0.076150034124101484, 0.14637647622561856, 0.1494359239700418] ]
        inputPoints = vtk.vtkPoints()
        for i in range(0, 10):
            inputPoints.InsertPoint(i, splinePoints[i])

        spline = vtk.vtkParametricSpline()
        spline.SetPoints(inputPoints)
        spline.ClosedOff()
        splineSource = vtk.vtkParametricFunctionSource()
        splineSource.SetParametricFunction(spline)

        splineMapper = vtk.vtkPolyDataMapper()
        splineMapper.SetInputConnection(splineSource.GetOutputPort())

        splineActor = vtk.vtkActor()
        splineActor.SetMapper(splineMapper)
        splineActor.SetPosition(16, 4, 0)
        splineActor.GetProperty().SetColor(0, 0, 0)

        splineTextMapper = vtk.vtkTextMapper()
        splineTextMapper.SetInput("Open.Spline")
        splineTextMapper.GetTextProperty().SetJustificationToCentered()
        splineTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        splineTextMapper.GetTextProperty().SetColor(1, 0, 0)
        splineTextMapper.GetTextProperty().SetFontSize(14)
        splineTextActor = vtk.vtkActor2D()
        splineTextActor.SetMapper(splineTextMapper)
        splineTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        splineTextActor.GetPositionCoordinate().SetValue(16, 1.5, 0)

        # ------------------------------------------------------------
        # Create a closed 1D spline
        # ------------------------------------------------------------
        spline2 = vtk.vtkParametricSpline()
        spline2.SetPoints(inputPoints)
        spline2.ClosedOn()
        spline2Source = vtk.vtkParametricFunctionSource()
        spline2Source.SetParametricFunction(spline2)

        spline2Mapper = vtk.vtkPolyDataMapper()
        spline2Mapper.SetInputConnection(spline2Source.GetOutputPort())

        spline2Actor = vtk.vtkActor()
        spline2Actor.SetMapper(spline2Mapper)
        spline2Actor.SetPosition(24, 4, 0)
        spline2Actor.GetProperty().SetColor(0, 0, 0)

        spline2TextMapper = vtk.vtkTextMapper()
        spline2TextMapper.SetInput("Closed.Spline")
        spline2TextMapper.GetTextProperty().SetJustificationToCentered()
        spline2TextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        spline2TextMapper.GetTextProperty().SetColor(1, 0, 0)
        spline2TextMapper.GetTextProperty().SetFontSize(14)
        spline2TextActor = vtk.vtkActor2D()
        spline2TextActor.SetMapper(spline2TextMapper)
        spline2TextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        spline2TextActor.GetPositionCoordinate().SetValue(24, 1.5, 0)

        # ------------------------------------------------------------
        # Create a spiral conic
        # ------------------------------------------------------------
        sconic = vtk.vtkParametricConicSpiral()
        sconic.SetA(0.8)
        sconic.SetB(2.5)
        sconic.SetC(0.4)
        sconicSource = vtk.vtkParametricFunctionSource()
        sconicSource.SetParametricFunction(sconic)
        sconicSource.SetScalarModeToDistance()

        sconicMapper = vtk.vtkPolyDataMapper()
        sconicMapper.SetInputConnection(sconicSource.GetOutputPort())
        sconicActor = vtk.vtkActor()
        sconicActor.SetMapper(sconicMapper)
        sconicMapper.SetScalarRange(0, 9)
        sconicActor.SetPosition(0, -4, 0)
        sconicActor.SetScale(1.2, 1.2, 1.2)

        sconicTextMapper = vtk.vtkTextMapper()
        sconicTextMapper.SetInput("Spiral.Conic")
        sconicTextMapper.GetTextProperty().SetJustificationToCentered()
        sconicTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        sconicTextMapper.GetTextProperty().SetColor(1, 0, 0)
        sconicTextMapper.GetTextProperty().SetFontSize(14)
        sconicTextActor = vtk.vtkActor2D()
        sconicTextActor.SetMapper(sconicTextMapper)
        sconicTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        sconicTextActor.GetPositionCoordinate().SetValue(0, -6.5, 0)

        # ------------------------------------------------------------
        # Create Boy's surface
        # ------------------------------------------------------------
        boy = vtk.vtkParametricBoy()
        boySource = vtk.vtkParametricFunctionSource()
        boySource.SetParametricFunction(boy)
        boySource.SetScalarModeToModulus()

        boyMapper = vtk.vtkPolyDataMapper()
        boyMapper.SetInputConnection(boySource.GetOutputPort())
        boyMapper.SetScalarRange(0, 2)
        boyActor = vtk.vtkActor()
        boyActor.SetMapper(boyMapper)
        boyActor.SetPosition(8, -4, 0)
        boyActor.SetScale(1.5, 1.5, 1.5)

        boyTextMapper = vtk.vtkTextMapper()
        boyTextMapper.SetInput("Boy")
        boyTextMapper.GetTextProperty().SetJustificationToCentered()
        boyTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        boyTextMapper.GetTextProperty().SetColor(1, 0, 0)
        boyTextMapper.GetTextProperty().SetFontSize(14)
        boyTextActor = vtk.vtkActor2D()
        boyTextActor.SetMapper(boyTextMapper)
        boyTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        boyTextActor.GetPositionCoordinate().SetValue(8, -6.5, 0)

        # ------------------------------------------------------------
        # Create a cross cap
        # ------------------------------------------------------------
        crossCap = vtk.vtkParametricCrossCap()
        crossCapSource = vtk.vtkParametricFunctionSource()
        crossCapSource.SetParametricFunction(crossCap)
        crossCapSource.SetScalarModeToY()

        crossCapMapper = vtk.vtkPolyDataMapper()
        crossCapMapper.SetInputConnection(crossCapSource.GetOutputPort())
        crossCapActor = vtk.vtkActor()
        crossCapActor.SetMapper(crossCapMapper)
        crossCapActor.RotateX(65)
        crossCapActor.SetPosition(16, -4, 0)
        crossCapActor.SetScale(1.5, 1.5, 1.5)

        crossCapTextMapper = vtk.vtkTextMapper()
        crossCapTextMapper.SetInput("Cross.Cap")
        crossCapTextMapper.GetTextProperty().SetJustificationToCentered()
        crossCapTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        crossCapTextMapper.GetTextProperty().SetColor(1, 0, 0)
        crossCapTextMapper.GetTextProperty().SetFontSize(14)
        crossCapTextActor = vtk.vtkActor2D()
        crossCapTextActor.SetMapper(crossCapTextMapper)
        crossCapTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        crossCapTextActor.GetPositionCoordinate().SetValue(16, -6.5, 0)

        # ------------------------------------------------------------
        # Create Dini's surface
        # ------------------------------------------------------------
        dini = vtk.vtkParametricDini()
        diniSource = vtk.vtkParametricFunctionSource()
        diniSource.SetScalarModeToDistance()
        diniSource.SetParametricFunction(dini)

        diniMapper = vtk.vtkPolyDataMapper()
        diniMapper.SetInputConnection(diniSource.GetOutputPort())

        diniActor = vtk.vtkActor()
        diniActor.SetMapper(diniMapper)
        diniActor.RotateX(-90)
        diniActor.SetPosition(24, -3, 0)
        diniActor.SetScale(1.5, 1.5, 0.5)

        diniTextMapper = vtk.vtkTextMapper()
        diniTextMapper.SetInput("Dini")
        diniTextMapper.GetTextProperty().SetJustificationToCentered()
        diniTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        diniTextMapper.GetTextProperty().SetColor(1, 0, 0)
        diniTextMapper.GetTextProperty().SetFontSize(14)
        diniTextActor = vtk.vtkActor2D()
        diniTextActor.SetMapper(diniTextMapper)
        diniTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        diniTextActor.GetPositionCoordinate().SetValue(24, -6.5, 0)

        # ------------------------------------------------------------
        # Create Enneper's surface
        # ------------------------------------------------------------
        enneper = vtk.vtkParametricEnneper()
        enneperSource = vtk.vtkParametricFunctionSource()
        enneperSource.SetParametricFunction(enneper)
        enneperSource.SetScalarModeToQuadrant()

        enneperMapper = vtk.vtkPolyDataMapper()
        enneperMapper.SetInputConnection(enneperSource.GetOutputPort())
        enneperMapper.SetScalarRange(1, 4)

        enneperActor = vtk.vtkActor()
        enneperActor.SetMapper(enneperMapper)
        enneperActor.SetPosition(0, -12, 0)
        enneperActor.SetScale(0.25, 0.25, 0.25)

        enneperTextMapper = vtk.vtkTextMapper()
        enneperTextMapper.SetInput("Enneper")
        enneperTextMapper.GetTextProperty().SetJustificationToCentered()
        enneperTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        enneperTextMapper.GetTextProperty().SetColor(1, 0, 0)
        enneperTextMapper.GetTextProperty().SetFontSize(14)
        enneperTextActor = vtk.vtkActor2D()
        enneperTextActor.SetMapper(enneperTextMapper)
        enneperTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        enneperTextActor.GetPositionCoordinate().SetValue(0, -14.5, 0)

        # ------------------------------------------------------------
        # Create an ellipsoidal surface
        # ------------------------------------------------------------
        ellipsoid = vtk.vtkParametricEllipsoid()
        ellipsoid.SetXRadius(1)
        ellipsoid.SetYRadius(0.75)
        ellipsoid.SetZRadius(0.5)
        ellipsoidSource = vtk.vtkParametricFunctionSource()
        ellipsoidSource.SetParametricFunction(ellipsoid)
        ellipsoidSource.SetScalarModeToZ()

        ellipsoidMapper = vtk.vtkPolyDataMapper()
        ellipsoidMapper.SetInputConnection(ellipsoidSource.GetOutputPort())
        ellipsoidMapper.SetScalarRange(-0.5, 0.5)

        ellipsoidActor = vtk.vtkActor()
        ellipsoidActor.SetMapper(ellipsoidMapper)
        ellipsoidActor.SetPosition(8, -12, 0)
        ellipsoidActor.SetScale(1.5, 1.5, 1.5)

        ellipsoidTextMapper = vtk.vtkTextMapper()
        ellipsoidTextMapper.SetInput("Ellipsoid")
        ellipsoidTextMapper.GetTextProperty().SetJustificationToCentered()
        ellipsoidTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        ellipsoidTextMapper.GetTextProperty().SetColor(1, 0, 0)
        ellipsoidTextMapper.GetTextProperty().SetFontSize(14)
        ellipsoidTextActor = vtk.vtkActor2D()
        ellipsoidTextActor.SetMapper(ellipsoidTextMapper)
        ellipsoidTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        ellipsoidTextActor.GetPositionCoordinate().SetValue(8, -14.5, 0)

        # ------------------------------------------------------------
        # Create a surface with random hills on it.
        # ------------------------------------------------------------
        randomHills = vtk.vtkParametricRandomHills()
        randomHills.AllowRandomGenerationOn()
        randomHillsSource = vtk.vtkParametricFunctionSource()
        randomHillsSource.SetParametricFunction(randomHills)
        randomHillsSource.GenerateTextureCoordinatesOn()

        randomHillsMapper = vtk.vtkPolyDataMapper()
        randomHillsMapper.SetInputConnection(randomHillsSource.GetOutputPort())

        randomHillsActor = vtk.vtkActor()
        randomHillsActor.SetMapper(randomHillsMapper)
        randomHillsActor.SetPosition(16, -14, 0)
        randomHillsActor.SetScale(0.2, 0.2, 0.2)
        randomHillsActor.SetTexture(texture)

        randomHillsTextMapper = vtk.vtkTextMapper()
        randomHillsTextMapper.SetInput("Random.Hills")
        randomHillsTextMapper.GetTextProperty().SetJustificationToCentered()
        randomHillsTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        randomHillsTextMapper.GetTextProperty().SetColor(1, 0, 0)
        randomHillsTextMapper.GetTextProperty().SetFontSize(14)
        randomHillsTextActor = vtk.vtkActor2D()
        randomHillsTextActor.SetMapper(randomHillsTextMapper)
        randomHillsTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        randomHillsTextActor.GetPositionCoordinate().SetValue(16, -14.5, 0)

        # ------------------------------------------------------------
        # Create Steiner's Roman Surface.
        # ------------------------------------------------------------
        roman = vtk.vtkParametricRoman()
        roman.SetRadius(1.5)
        romanSource = vtk.vtkParametricFunctionSource()
        romanSource.SetParametricFunction(roman)
        romanSource.SetScalarModeToX()

        romanMapper = vtk.vtkPolyDataMapper()
        romanMapper.SetInputConnection(romanSource.GetOutputPort())

        romanActor = vtk.vtkActor()
        romanActor.SetMapper(romanMapper)
        romanActor.SetPosition(24, -12, 0)

        romanTextMapper = vtk.vtkTextMapper()
        romanTextMapper.SetInput("Roman")
        romanTextMapper.GetTextProperty().SetJustificationToCentered()
        romanTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        romanTextMapper.GetTextProperty().SetColor(1, 0, 0)
        romanTextMapper.GetTextProperty().SetFontSize(14)
        romanTextActor = vtk.vtkActor2D()
        romanTextActor.SetMapper(romanTextMapper)
        romanTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        romanTextActor.GetPositionCoordinate().SetValue(24, -14.5, 0)

        # ------------------------------------------------------------
        # Create the RenderWindow, Renderer and both Actors
        # ------------------------------------------------------------
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)
        iren = vtk.vtkRenderWindowInteractor()
        iren.SetRenderWindow(renWin)

        # add actors
        ren.AddViewProp(torusActor)
        ren.AddViewProp(kleinActor)
        ren.AddViewProp(klein2Actor)
        ren.AddViewProp(toroidActor)
        ren.AddViewProp(superEllipsoidActor)
        ren.AddViewProp(mobiusActor)
        ren.AddViewProp(splineActor)
        ren.AddViewProp(spline2Actor)
        ren.AddViewProp(sconicActor)
        ren.AddViewProp(boyActor)
        ren.AddViewProp(crossCapActor)
        ren.AddViewProp(diniActor)
        ren.AddViewProp(enneperActor)
        ren.AddViewProp(ellipsoidActor)
        ren.AddViewProp(randomHillsActor)
        ren.AddViewProp(romanActor)
        #add text actors
        ren.AddViewProp(torusTextActor)
        ren.AddViewProp(kleinTextActor)
        ren.AddViewProp(fig8KleinTextActor)
        ren.AddViewProp(mobiusTextActor)
        ren.AddViewProp(superToroidTextActor)
        ren.AddViewProp(superEllipsoidTextActor)
        ren.AddViewProp(splineTextActor)
        ren.AddViewProp(spline2TextActor)
        ren.AddViewProp(sconicTextActor)
        ren.AddViewProp(boyTextActor)
        ren.AddViewProp(crossCapTextActor)
        ren.AddViewProp(diniTextActor)
        ren.AddViewProp(enneperTextActor)
        ren.AddViewProp(ellipsoidTextActor)
        ren.AddViewProp(randomHillsTextActor)
        ren.AddViewProp(romanTextActor)

        ren.SetBackground(0.7, 0.8, 1)
        renWin.SetSize(500, 500)
        ren.ResetCamera()
        ren.GetActiveCamera().Zoom(1.3)

        iren.Initialize()
        renWin.Render()

        img_file = "TestParametricFunctions.png"
        # NOTE: this test has a companion .tcl test. The threshold set
        #  here should be the same as the threshold in the .tcl
        #  test. Both tests should produce exactly the same results.
        vtk.test.Testing.compareImage(iren.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=10)
        vtk.test.Testing.interact()

if __name__ == "__main__":
    vtk.test.Testing.main([(TestParametricFunctions, 'test')])
