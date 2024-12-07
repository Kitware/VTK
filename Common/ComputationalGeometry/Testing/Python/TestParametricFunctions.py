#!/usr/bin/env python
# -*- coding: utf-8 -*-

from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonComputationalGeometry import (
    vtkParametricBoy,
    vtkParametricConicSpiral,
    vtkParametricCrossCap,
    vtkParametricDini,
    vtkParametricEllipsoid,
    vtkParametricEnneper,
    vtkParametricFigure8Klein,
    vtkParametricKlein,
    vtkParametricMobius,
    vtkParametricRandomHills,
    vtkParametricRoman,
    vtkParametricSpline,
    vtkParametricSuperEllipsoid,
    vtkParametricSuperToroid,
    vtkParametricTorus,
)
from vtkmodules.vtkFiltersSources import vtkParametricFunctionSource
from vtkmodules.vtkIOImage import vtkJPEGReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkActor2D,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTextMapper,
    vtkTexture,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# ------------------------------------------------------------
# Purpose: Test the parametric functions.
# ------------------------------------------------------------

class TestParametricFunctions(vtkmodules.test.Testing.vtkTest):
    def testParametricFunctions(self):
        # ------------------------------------------------------------
        # Get a texture
        # ------------------------------------------------------------
        textureReader = vtkJPEGReader()
        textureReader.SetFileName(VTK_DATA_ROOT + "/Data/beach.jpg")
        texture = vtkTexture()
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
        torus = vtkParametricTorus()
        torusSource = vtkParametricFunctionSource()
        torusSource.SetParametricFunction(torus)
        torusSource.SetScalarModeToPhase()

        torusMapper = vtkPolyDataMapper()
        torusMapper.SetInputConnection(torusSource.GetOutputPort())
        torusMapper.SetScalarRange(0, 360)

        torusActor = vtkActor()
        torusActor.SetMapper(torusMapper)
        torusActor.SetPosition(0, 12, 0)

        torusTextMapper = vtkTextMapper()
        torusTextMapper.SetInput("Torus")
        torusTextMapper.GetTextProperty().SetJustificationToCentered()
        torusTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        torusTextMapper.GetTextProperty().SetColor(1, 0, 0)
        torusTextMapper.GetTextProperty().SetFontSize(14)
        torusTextActor = vtkActor2D()
        torusTextActor.SetMapper(torusTextMapper)
        torusTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        torusTextActor.GetPositionCoordinate().SetValue(0, 9.5, 0)

        # ------------------------------------------------------------
        # Create a Klein bottle
        # ------------------------------------------------------------
        klein = vtkParametricKlein()
        kleinSource = vtkParametricFunctionSource()
        kleinSource.SetParametricFunction(klein)
        kleinSource.SetScalarModeToU0V0()

        kleinMapper = vtkPolyDataMapper()
        kleinMapper.SetInputConnection(kleinSource.GetOutputPort())
        kleinMapper.SetScalarRange(0, 3)

        kleinActor = vtkActor()
        kleinActor.SetMapper(kleinMapper)
        kleinActor.SetPosition(8, 10.5, 0)

        kleinTextMapper = vtkTextMapper()
        kleinTextMapper.SetInput("Klein")
        kleinTextMapper.GetTextProperty().SetJustificationToCentered()
        kleinTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        kleinTextMapper.GetTextProperty().SetColor(1, 0, 0)
        kleinTextMapper.GetTextProperty().SetFontSize(14)
        kleinTextActor = vtkActor2D()
        kleinTextActor.SetMapper(kleinTextMapper)
        kleinTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        kleinTextActor.GetPositionCoordinate().SetValue(8, 9.5, 0)

        # ------------------------------------------------------------
        # Create a Figure-8 Klein
        # ------------------------------------------------------------
        klein2 = vtkParametricFigure8Klein()
        klein2Source = vtkParametricFunctionSource()
        klein2Source.SetParametricFunction(klein2)
        klein2Source.GenerateTextureCoordinatesOn()

        klein2Mapper = vtkPolyDataMapper()
        klein2Mapper.SetInputConnection(klein2Source.GetOutputPort())
        klein2Mapper.SetScalarRange(0, 3)

        klein2Actor = vtkActor()
        klein2Actor.SetMapper(klein2Mapper)
        klein2Actor.SetPosition(16, 12, 0)
        klein2Actor.SetTexture(texture)


        fig8KleinTextMapper = vtkTextMapper()
        fig8KleinTextMapper.SetInput("Fig-8.Klein")
        fig8KleinTextMapper.GetTextProperty().SetJustificationToCentered()
        fig8KleinTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        fig8KleinTextMapper.GetTextProperty().SetColor(1, 0, 0)
        fig8KleinTextMapper.GetTextProperty().SetFontSize(14)
        fig8KleinTextActor = vtkActor2D()
        fig8KleinTextActor.SetMapper(fig8KleinTextMapper)
        fig8KleinTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        fig8KleinTextActor.GetPositionCoordinate().SetValue(16, 9.5, 0)

        # ------------------------------------------------------------
        # Create a Mobius strip
        # ------------------------------------------------------------
        mobius = vtkParametricMobius()
        mobiusSource = vtkParametricFunctionSource()
        mobiusSource.SetParametricFunction(mobius)
        mobiusSource.GenerateTextureCoordinatesOn()

        mobiusMapper = vtkPolyDataMapper()
        mobiusMapper.SetInputConnection(mobiusSource.GetOutputPort())

        mobiusActor = vtkActor()
        mobiusActor.SetMapper(mobiusMapper)
        mobiusActor.RotateX(45)
        mobiusActor.SetPosition(24, 12, 0)
        mobiusActor.SetTexture(texture)

        mobiusTextMapper = vtkTextMapper()
        mobiusTextMapper.SetInput("Mobius")
        mobiusTextMapper.GetTextProperty().SetJustificationToCentered()
        mobiusTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        mobiusTextMapper.GetTextProperty().SetColor(1, 0, 0)
        mobiusTextMapper.GetTextProperty().SetFontSize(14)
        mobiusTextActor = vtkActor2D()
        mobiusTextActor.SetMapper(mobiusTextMapper)
        mobiusTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        mobiusTextActor.GetPositionCoordinate().SetValue(24, 9.5, 0)

        # ------------------------------------------------------------
        # Create a super toroid
        # ------------------------------------------------------------
        toroid = vtkParametricSuperToroid()
        toroid.SetN1(2)
        toroid.SetN2(3)
        toroidSource = vtkParametricFunctionSource()
        toroidSource.SetParametricFunction(toroid)
        toroidSource.SetScalarModeToU()

        toroidMapper = vtkPolyDataMapper()
        toroidMapper.SetInputConnection(toroidSource.GetOutputPort())
        toroidMapper.SetScalarRange(0, 6.28)

        toroidActor = vtkActor()
        toroidActor.SetMapper(toroidMapper)
        toroidActor.SetPosition(0, 4, 0)

        superToroidTextMapper = vtkTextMapper()
        superToroidTextMapper.SetInput("Super.Toroid")
        superToroidTextMapper.GetTextProperty().SetJustificationToCentered()
        superToroidTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        superToroidTextMapper.GetTextProperty().SetColor(1, 0, 0)
        superToroidTextMapper.GetTextProperty().SetFontSize(14)
        superToroidTextActor = vtkActor2D()
        superToroidTextActor.SetMapper(superToroidTextMapper)
        superToroidTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        superToroidTextActor.GetPositionCoordinate().SetValue(0, 1.5, 0)

        # ------------------------------------------------------------
        # Create a super ellipsoid
        # ------------------------------------------------------------
        superEllipsoid = vtkParametricSuperEllipsoid()
        superEllipsoid.SetXRadius(1.25)
        superEllipsoid.SetYRadius(1.5)
        superEllipsoid.SetZRadius(1.0)
        superEllipsoid.SetN1(1.1)
        superEllipsoid.SetN2(1.75)
        superEllipsoidSource = vtkParametricFunctionSource()
        superEllipsoidSource.SetParametricFunction(superEllipsoid)
        superEllipsoidSource.SetScalarModeToV()

        superEllipsoidMapper = vtkPolyDataMapper()
        superEllipsoidMapper.SetInputConnection(superEllipsoidSource.GetOutputPort())
        superEllipsoidMapper.SetScalarRange(0, 3.14)

        superEllipsoidActor = vtkActor()
        superEllipsoidActor.SetMapper(superEllipsoidMapper)
        superEllipsoidActor.SetPosition(8, 4, 0)

        superEllipsoidTextMapper = vtkTextMapper()
        superEllipsoidTextMapper.SetInput("Super.Ellipsoid")
        superEllipsoidTextMapper.GetTextProperty().SetJustificationToCentered()
        superEllipsoidTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        superEllipsoidTextMapper.GetTextProperty().SetColor(1, 0, 0)
        superEllipsoidTextMapper.GetTextProperty().SetFontSize(14)
        superEllipsoidTextActor = vtkActor2D()
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
        inputPoints = vtkPoints()
        for i in range(0, 10):
            inputPoints.InsertPoint(i, splinePoints[i])

        spline = vtkParametricSpline()
        spline.SetPoints(inputPoints)
        spline.ClosedOff()
        splineSource = vtkParametricFunctionSource()
        splineSource.SetParametricFunction(spline)

        splineMapper = vtkPolyDataMapper()
        splineMapper.SetInputConnection(splineSource.GetOutputPort())

        splineActor = vtkActor()
        splineActor.SetMapper(splineMapper)
        splineActor.SetPosition(16, 4, 0)
        splineActor.GetProperty().SetColor(0, 0, 0)

        splineTextMapper = vtkTextMapper()
        splineTextMapper.SetInput("Open.Spline")
        splineTextMapper.GetTextProperty().SetJustificationToCentered()
        splineTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        splineTextMapper.GetTextProperty().SetColor(1, 0, 0)
        splineTextMapper.GetTextProperty().SetFontSize(14)
        splineTextActor = vtkActor2D()
        splineTextActor.SetMapper(splineTextMapper)
        splineTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        splineTextActor.GetPositionCoordinate().SetValue(16, 1.5, 0)

        # ------------------------------------------------------------
        # Create a closed 1D spline
        # ------------------------------------------------------------
        spline2 = vtkParametricSpline()
        spline2.SetPoints(inputPoints)
        spline2.ClosedOn()
        spline2Source = vtkParametricFunctionSource()
        spline2Source.SetParametricFunction(spline2)

        spline2Mapper = vtkPolyDataMapper()
        spline2Mapper.SetInputConnection(spline2Source.GetOutputPort())

        spline2Actor = vtkActor()
        spline2Actor.SetMapper(spline2Mapper)
        spline2Actor.SetPosition(24, 4, 0)
        spline2Actor.GetProperty().SetColor(0, 0, 0)

        spline2TextMapper = vtkTextMapper()
        spline2TextMapper.SetInput("Closed.Spline")
        spline2TextMapper.GetTextProperty().SetJustificationToCentered()
        spline2TextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        spline2TextMapper.GetTextProperty().SetColor(1, 0, 0)
        spline2TextMapper.GetTextProperty().SetFontSize(14)
        spline2TextActor = vtkActor2D()
        spline2TextActor.SetMapper(spline2TextMapper)
        spline2TextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        spline2TextActor.GetPositionCoordinate().SetValue(24, 1.5, 0)

        # ------------------------------------------------------------
        # Create a spiral conic
        # ------------------------------------------------------------
        sconic = vtkParametricConicSpiral()
        sconic.SetA(0.8)
        sconic.SetB(2.5)
        sconic.SetC(0.4)
        sconicSource = vtkParametricFunctionSource()
        sconicSource.SetParametricFunction(sconic)
        sconicSource.SetScalarModeToDistance()

        sconicMapper = vtkPolyDataMapper()
        sconicMapper.SetInputConnection(sconicSource.GetOutputPort())
        sconicActor = vtkActor()
        sconicActor.SetMapper(sconicMapper)
        sconicMapper.SetScalarRange(0, 9)
        sconicActor.SetPosition(0, -4, 0)
        sconicActor.SetScale(1.2, 1.2, 1.2)

        sconicTextMapper = vtkTextMapper()
        sconicTextMapper.SetInput("Spiral.Conic")
        sconicTextMapper.GetTextProperty().SetJustificationToCentered()
        sconicTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        sconicTextMapper.GetTextProperty().SetColor(1, 0, 0)
        sconicTextMapper.GetTextProperty().SetFontSize(14)
        sconicTextActor = vtkActor2D()
        sconicTextActor.SetMapper(sconicTextMapper)
        sconicTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        sconicTextActor.GetPositionCoordinate().SetValue(0, -6.5, 0)

        # ------------------------------------------------------------
        # Create Boy's surface
        # ------------------------------------------------------------
        boy = vtkParametricBoy()
        boySource = vtkParametricFunctionSource()
        boySource.SetParametricFunction(boy)
        boySource.SetScalarModeToModulus()

        boyMapper = vtkPolyDataMapper()
        boyMapper.SetInputConnection(boySource.GetOutputPort())
        boyMapper.SetScalarRange(0, 2)
        boyActor = vtkActor()
        boyActor.SetMapper(boyMapper)
        boyActor.SetPosition(8, -4, 0)
        boyActor.SetScale(1.5, 1.5, 1.5)

        boyTextMapper = vtkTextMapper()
        boyTextMapper.SetInput("Boy")
        boyTextMapper.GetTextProperty().SetJustificationToCentered()
        boyTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        boyTextMapper.GetTextProperty().SetColor(1, 0, 0)
        boyTextMapper.GetTextProperty().SetFontSize(14)
        boyTextActor = vtkActor2D()
        boyTextActor.SetMapper(boyTextMapper)
        boyTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        boyTextActor.GetPositionCoordinate().SetValue(8, -6.5, 0)

        # ------------------------------------------------------------
        # Create a cross cap
        # ------------------------------------------------------------
        crossCap = vtkParametricCrossCap()
        crossCapSource = vtkParametricFunctionSource()
        crossCapSource.SetParametricFunction(crossCap)
        crossCapSource.SetScalarModeToY()

        crossCapMapper = vtkPolyDataMapper()
        crossCapMapper.SetInputConnection(crossCapSource.GetOutputPort())
        crossCapActor = vtkActor()
        crossCapActor.SetMapper(crossCapMapper)
        crossCapActor.RotateX(65)
        crossCapActor.SetPosition(16, -4, 0)
        crossCapActor.SetScale(1.5, 1.5, 1.5)

        crossCapTextMapper = vtkTextMapper()
        crossCapTextMapper.SetInput("Cross.Cap")
        crossCapTextMapper.GetTextProperty().SetJustificationToCentered()
        crossCapTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        crossCapTextMapper.GetTextProperty().SetColor(1, 0, 0)
        crossCapTextMapper.GetTextProperty().SetFontSize(14)
        crossCapTextActor = vtkActor2D()
        crossCapTextActor.SetMapper(crossCapTextMapper)
        crossCapTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        crossCapTextActor.GetPositionCoordinate().SetValue(16, -6.5, 0)

        # ------------------------------------------------------------
        # Create Dini's surface
        # ------------------------------------------------------------
        dini = vtkParametricDini()
        diniSource = vtkParametricFunctionSource()
        diniSource.SetScalarModeToDistance()
        diniSource.SetParametricFunction(dini)

        diniMapper = vtkPolyDataMapper()
        diniMapper.SetInputConnection(diniSource.GetOutputPort())

        diniActor = vtkActor()
        diniActor.SetMapper(diniMapper)
        diniActor.RotateX(-90)
        diniActor.SetPosition(24, -3, 0)
        diniActor.SetScale(1.5, 1.5, 0.5)

        diniTextMapper = vtkTextMapper()
        diniTextMapper.SetInput("Dini")
        diniTextMapper.GetTextProperty().SetJustificationToCentered()
        diniTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        diniTextMapper.GetTextProperty().SetColor(1, 0, 0)
        diniTextMapper.GetTextProperty().SetFontSize(14)
        diniTextActor = vtkActor2D()
        diniTextActor.SetMapper(diniTextMapper)
        diniTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        diniTextActor.GetPositionCoordinate().SetValue(24, -6.5, 0)

        # ------------------------------------------------------------
        # Create Enneper's surface
        # ------------------------------------------------------------
        enneper = vtkParametricEnneper()
        enneperSource = vtkParametricFunctionSource()
        enneperSource.SetParametricFunction(enneper)
        enneperSource.SetScalarModeToQuadrant()

        enneperMapper = vtkPolyDataMapper()
        enneperMapper.SetInputConnection(enneperSource.GetOutputPort())
        enneperMapper.SetScalarRange(1, 4)

        enneperActor = vtkActor()
        enneperActor.SetMapper(enneperMapper)
        enneperActor.SetPosition(0, -12, 0)
        enneperActor.SetScale(0.25, 0.25, 0.25)

        enneperTextMapper = vtkTextMapper()
        enneperTextMapper.SetInput("Enneper")
        enneperTextMapper.GetTextProperty().SetJustificationToCentered()
        enneperTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        enneperTextMapper.GetTextProperty().SetColor(1, 0, 0)
        enneperTextMapper.GetTextProperty().SetFontSize(14)
        enneperTextActor = vtkActor2D()
        enneperTextActor.SetMapper(enneperTextMapper)
        enneperTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        enneperTextActor.GetPositionCoordinate().SetValue(0, -14.5, 0)

        # ------------------------------------------------------------
        # Create an ellipsoidal surface
        # ------------------------------------------------------------
        ellipsoid = vtkParametricEllipsoid()
        ellipsoid.SetXRadius(1)
        ellipsoid.SetYRadius(0.75)
        ellipsoid.SetZRadius(0.5)
        ellipsoidSource = vtkParametricFunctionSource()
        ellipsoidSource.SetParametricFunction(ellipsoid)
        ellipsoidSource.SetScalarModeToZ()

        ellipsoidMapper = vtkPolyDataMapper()
        ellipsoidMapper.SetInputConnection(ellipsoidSource.GetOutputPort())
        ellipsoidMapper.SetScalarRange(-0.5, 0.5)

        ellipsoidActor = vtkActor()
        ellipsoidActor.SetMapper(ellipsoidMapper)
        ellipsoidActor.SetPosition(8, -12, 0)
        ellipsoidActor.SetScale(1.5, 1.5, 1.5)

        ellipsoidTextMapper = vtkTextMapper()
        ellipsoidTextMapper.SetInput("Ellipsoid")
        ellipsoidTextMapper.GetTextProperty().SetJustificationToCentered()
        ellipsoidTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        ellipsoidTextMapper.GetTextProperty().SetColor(1, 0, 0)
        ellipsoidTextMapper.GetTextProperty().SetFontSize(14)
        ellipsoidTextActor = vtkActor2D()
        ellipsoidTextActor.SetMapper(ellipsoidTextMapper)
        ellipsoidTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        ellipsoidTextActor.GetPositionCoordinate().SetValue(8, -14.5, 0)

        # ------------------------------------------------------------
        # Create a surface with random hills on it.
        # ------------------------------------------------------------
        randomHills = vtkParametricRandomHills()
        randomHills.AllowRandomGenerationOn()
        randomHillsSource = vtkParametricFunctionSource()
        randomHillsSource.SetParametricFunction(randomHills)
        randomHillsSource.GenerateTextureCoordinatesOn()

        randomHillsMapper = vtkPolyDataMapper()
        randomHillsMapper.SetInputConnection(randomHillsSource.GetOutputPort())

        randomHillsActor = vtkActor()
        randomHillsActor.SetMapper(randomHillsMapper)
        randomHillsActor.SetPosition(16, -14, 0)
        randomHillsActor.SetScale(0.2, 0.2, 0.2)
        randomHillsActor.SetTexture(texture)

        randomHillsTextMapper = vtkTextMapper()
        randomHillsTextMapper.SetInput("Random.Hills")
        randomHillsTextMapper.GetTextProperty().SetJustificationToCentered()
        randomHillsTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        randomHillsTextMapper.GetTextProperty().SetColor(1, 0, 0)
        randomHillsTextMapper.GetTextProperty().SetFontSize(14)
        randomHillsTextActor = vtkActor2D()
        randomHillsTextActor.SetMapper(randomHillsTextMapper)
        randomHillsTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        randomHillsTextActor.GetPositionCoordinate().SetValue(16, -14.5, 0)

        # ------------------------------------------------------------
        # Create Steiner's Roman Surface.
        # ------------------------------------------------------------
        roman = vtkParametricRoman()
        roman.SetRadius(1.5)
        romanSource = vtkParametricFunctionSource()
        romanSource.SetParametricFunction(roman)
        romanSource.SetScalarModeToX()

        romanMapper = vtkPolyDataMapper()
        romanMapper.SetInputConnection(romanSource.GetOutputPort())

        romanActor = vtkActor()
        romanActor.SetMapper(romanMapper)
        romanActor.SetPosition(24, -12, 0)

        romanTextMapper = vtkTextMapper()
        romanTextMapper.SetInput("Roman")
        romanTextMapper.GetTextProperty().SetJustificationToCentered()
        romanTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        romanTextMapper.GetTextProperty().SetColor(1, 0, 0)
        romanTextMapper.GetTextProperty().SetFontSize(14)
        romanTextActor = vtkActor2D()
        romanTextActor.SetMapper(romanTextMapper)
        romanTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        romanTextActor.GetPositionCoordinate().SetValue(24, -14.5, 0)

        # ------------------------------------------------------------
        # Create the RenderWindow, Renderer and both Actors
        # ------------------------------------------------------------
        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)
        iren = vtkRenderWindowInteractor()
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
        vtkmodules.test.Testing.compareImage(iren.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
    vtkmodules.test.Testing.main([(TestParametricFunctions, 'test')])
