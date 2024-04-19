#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkCommonDataModel import vtkIterativeClosestPointTransform
from vtkmodules.vtkFiltersCore import vtkFeatureEdges
from vtkmodules.vtkFiltersSources import vtkSuperquadricSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestICPTransform(vtkmodules.test.Testing.vtkTest):

    def testICPTransform(self):

        renWin = vtkRenderWindow()

        #iren = vtkRenderWindowInteractor()
        #iren.SetRenderWindow(renWin)

        # Create objects

        sscale = {2:[0.7, 0.7, 0.7],
                  3:[0.5, 0.5, 0.5]}
        scenter = {2:[-0.25, 0.25, 0.0],
                   3:[ 0.4, -0.3, 0.0]}
        scolors = {2:[0.2, 0.6, 0.1],
                   3:[0.1, 0.2, 0.6]}

        s = dict() # The super quadric sources

        for sidx in range(1, 4):
            s.update({sidx:vtkSuperquadricSource()})
            s[sidx].ToroidalOff()
            s[sidx].SetThetaResolution(20)
            s[sidx].SetPhiResolution(20)
            s[sidx].SetPhiRoundness(0.7 + (sidx - 2) * 0.4)
            s[sidx].SetThetaRoundness(0.85 + (sidx - 1) * 0.4)
            if sidx in sscale:
                s[sidx].SetScale(sscale[sidx])
            if sidx in scenter:
                s[sidx].SetCenter(scenter[sidx])

            s[sidx].Update()

        ren = dict() # Renderers
        sm = dict() # Mappers for the super quadric source
        sa = dict() # Actors for the super quadric source
        fe = dict() # Feature edges
        fem = dict() # Feature edges mappers
        fea = dict() # Feature edges actors
        icp = dict() # Iterated closest point transforms

        # Create the renderers

        for ridx in range(1, 4):
            ren.update({ridx: vtkRenderer()})
            ren[ridx].SetViewport((ridx - 1) / 3.0, 0.0, ridx / 3.0, 1.0)
            ren[ridx].SetBackground(0.7, 0.8, 1.0)
            cam = ren[ridx].GetActiveCamera()
            cam.SetPosition(1.7, 1.4, 1.7)
            renWin.AddRenderer(ren[ridx])

            # renderer 1 has all 3 objects, render i has object 1 and i (i=2, 3)
            # add actors (corresponding to the objects) to each renderer
            # and ICP transforms from objects i or to 1.
            # object 1 has feature edges too.

            for sidx in range(1, 4):

                if ridx == 1 or sidx == 1 or ridx == sidx:
                    sm.update({ridx:{sidx:vtkPolyDataMapper()}})
                    sm[ridx][sidx].SetInputConnection(s[sidx].GetOutputPort())

                    sa.update({ridx:{sidx:vtkActor()}})
                    sa[ridx][sidx].SetMapper(sm[ridx][sidx])

                    prop = sa[ridx][sidx].GetProperty()
                    if sidx in scolors:
                        prop.SetColor(scolors[sidx])

                    if sidx == 1:
                        prop.SetOpacity(0.2)

                        fe.update({ridx:{sidx:vtkFeatureEdges()}})
                        src = s[sidx]
                        fe[ridx][sidx].SetInputConnection(src.GetOutputPort())
                        fe[ridx][sidx].BoundaryEdgesOn()
                        fe[ridx][sidx].ColoringOff()
                        fe[ridx][sidx].ManifoldEdgesOff()

                        fem.update({ridx:{sidx:vtkPolyDataMapper()}})
                        fem[ridx][sidx].SetInputConnection(fe[ridx][sidx].GetOutputPort())
                        fem[ridx][sidx].SetResolveCoincidentTopologyToPolygonOffset()

                        fea.update({ridx:{sidx:vtkActor()}})
                        fea[ridx][sidx].SetMapper(fem[ridx][sidx])

                        ren[ridx].AddActor(fea[ridx][sidx])


                    ren[ridx].AddActor(sa[ridx][sidx])


                if ridx > 1 and ridx == sidx:
                    icp.update({ridx:{sidx:vtkIterativeClosestPointTransform()}})
                    icp[ridx][sidx].SetSource(s[sidx].GetOutput())
                    icp[ridx][sidx].SetTarget(s[1].GetOutput())
                    icp[ridx][sidx].SetCheckMeanDistance(1)
                    icp[ridx][sidx].SetMaximumMeanDistance(0.001)
                    icp[ridx][sidx].SetMaximumNumberOfIterations(30)
                    icp[ridx][sidx].SetMaximumNumberOfLandmarks(50)
                    sa[ridx][sidx].SetUserTransform(icp[ridx][sidx])


        icp[3][3].StartByMatchingCentroidsOn()

        renWin.SetSize(400, 100)

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestICPTransform.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestICPTransform, 'test')])
