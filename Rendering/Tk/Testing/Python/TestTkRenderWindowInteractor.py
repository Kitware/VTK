import os, sys
import vtk
from vtk.test import Testing

if sys.hexversion < 0x03000000:
    # for Python2
    import Tkinter as tkinter
    from Tkinter import Pack
else:
    # for Python3
    import tkinter
    from tkinter import Pack

from vtk.tk.vtkTkRenderWindowInteractor import vtkTkRenderWindowInteractor


class TestTkRenderWindowInteractor(Testing.vtkTest):

    # Stick your VTK pipeline here if you want to create the pipeline
    # only once.  If you put it in the constructor or in the function
    # the pipeline will be created afresh for each and every test.

    # create a dummy Tkinter root window.
    root = tkinter.Tk()

    # create a rendering window and renderer
    ren = vtk.vtkRenderer()
    tkrw = vtkTkRenderWindowInteractor(root, width=300, height=300)
    tkrw.Initialize()
    tkrw.pack()
    rw = tkrw.GetRenderWindow()
    rw.AddRenderer(ren)

    # create an actor and give it cone geometry
    cs = vtk.vtkConeSource()
    cs.SetResolution(8)
    map = vtk.vtkPolyDataMapper()
    map.SetInputConnection(cs.GetOutputPort())
    act = vtk.vtkActor()
    act.SetMapper(map)

    # assign our actor to the renderer
    ren.AddActor(act)

    def testvtkTkRenderWindowInteractor(self):
        "Test if vtkTkRenderWindowInteractor works."
        self.tkrw.Start()
        self.tkrw.Render()
        self.root.update()
        img_file = "TestTkRenderWindowInteractor.png"
        Testing.compareImage(self.rw, Testing.getAbsImagePath(img_file))
        Testing.interact()

    # These are useful blackbox tests (not dummy ones!)
    def testParse(self):
        "Test if vtkTkRenderWindowInteractor is parseable"
        self._testParse(self.tkrw)

    def testGetSet(self):
        "Testing Get/Set methods of vtkTkRenderWindowInteractor"
        self._testGetSet(self.tkrw)

    def testBoolean(self):
        "Testing Boolean methods of vtkTkRenderWindowInteractor"
        self._testBoolean(self.tkrw)

if __name__ == "__main__":
    cases = [(TestTkRenderWindowInteractor, 'test')]
    del TestTkRenderWindowInteractor
    Testing.main(cases)
