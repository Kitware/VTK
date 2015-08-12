#!/usr/bin/env python

import sys
from functools import partial
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

from vtk.tk.vtkTkRenderWindowInteractor import vtkTkRenderWindowInteractor
from vtk.tk.vtkTkRenderWidget import vtkTkRenderWidget

if sys.hexversion < 0x03000000:
    # for Python2
    import Tkinter as tkinter
else:
    # for Python3
    import tkinter

# tkinter constants.
E = tkinter.E
W = tkinter.W
N = tkinter.N
S = tkinter.S
HORIZONTAL = tkinter.HORIZONTAL
RIGHT = tkinter.RIGHT
NO = tkinter.NO
NORMAL = tkinter.NORMAL
DISABLED = tkinter.DISABLED

class SuperQuadricViewer(Testing.vtkTest):
    '''
    Provide a testing framework for squadViewer.

    Note:
        root, the top-level widget for Tk,
        tkrw, the vtkTkRenderWidget and
        renWin, the vtkRenderindow
        are accessible from any function in this class
        after SetUp() has run.
    '''

    def SetUp(self):
        '''
        Set up squadViewer
        '''
        def OnClosing():
            self.root.quit()

        def SetPhi(squad, win, phi):
            squad.SetPhiRoundness(float(phi))
            win.Render()

        def SetTheta(squad, win, theta):
            squad.SetThetaRoundness(float(theta))
            win.Render()

        def SetThickness(squad, win, thickness):
            squad.SetThickness(float(thickness))
            win.Render()

        def SetTexture(actor, texture, win):
            if doTexture.get():
                actor.SetTexture(texture)
            else:
                actor.SetTexture(None)
            win.Render()

        def SetToroid(squad, scale, win):
            squad.SetToroidal(toroid.get())
            if toroid.get():
                scale.config(state=NORMAL, fg='black')
            else:
                scale.config(state=DISABLED, fg='gray')
            win.Render()

        self.root = tkinter.Tk()
        self.root.title("superquadric viewer")
        # Define what to do when the user explicitly closes a window.
        self.root.protocol("WM_DELETE_WINDOW", OnClosing)

        # Create render window
        self.tkrw = vtkTkRenderWidget(self.root, width=550, height=450)
        self.tkrw.BindTkRenderWidget()
        self.renWin = self.tkrw.GetRenderWindow()

        # Create parameter sliders
        #
        prs = tkinter.Scale(self.root, from_=0, to=3.5, res=0.1,
                             orient=HORIZONTAL, label="phi roundness")
        trs = tkinter.Scale(self.root, from_=0, to=3.5, res=0.1,
                             orient=HORIZONTAL, label="theta roundness")
        thicks = tkinter.Scale(self.root, from_=0.01, to=1.0, res=0.01,
                             orient=HORIZONTAL, label="thickness")

        # Create check buttons
        #
        toroid = tkinter.IntVar()
        toroid.set(0)
        doTexture = tkinter.IntVar()
        doTexture.set(0)

        rframe = tkinter.Frame(self.root)
        torbut = tkinter.Checkbutton(rframe, text="Toroid", variable=toroid)
        texbut = tkinter.Checkbutton(rframe, text="Texture", variable=doTexture)

        # Put it all together
        #
        torbut.pack(padx=10, pady=5, ipadx=20, ipady=5, side=RIGHT, anchor=S)
        texbut.pack(padx=10, pady=5, ipadx=20, ipady=5, side=RIGHT, anchor=S)

        self.tkrw.grid(sticky=N+E+W+S, columnspan=2)
        rframe.grid(sticky=N+E+W+S)
        thicks.grid(sticky=N+S+E+W, padx=10, ipady=5, row=1, column=1)
        prs.grid(sticky=N+E+W+S, padx=10, ipady=5, row = 2, column = 0)
        trs.grid(sticky=N+E+W+S, padx=10, ipady=5, row = 2, column = 1)
        tkinter.Pack.propagate(rframe,NO)

        prs.set(1.0)
        trs.set(0.7)
        thicks.set(0.3)
        toroid.set(1)
        doTexture.set(0)

        # Create pipeline
        #
        squad = vtk.vtkSuperquadricSource()
        squad.SetPhiResolution(20)
        squad.SetThetaResolution(25)

        pnmReader = vtk.vtkPNMReader()
        pnmReader.SetFileName(VTK_DATA_ROOT + "/Data/earth.ppm")
        atext = vtk.vtkTexture()
        atext.SetInputConnection(pnmReader.GetOutputPort())
        atext.InterpolateOn()

        appendSquads = vtk.vtkAppendPolyData()
        appendSquads.AddInputConnection(squad.GetOutputPort())

        mapper = vtk.vtkPolyDataMapper()
        mapper.SetInputConnection(squad.GetOutputPort())
        mapper.ScalarVisibilityOff()
        actor = vtk.vtkActor()
        actor.SetMapper(mapper)
        actor.SetTexture(atext)
        actor.GetProperty().SetDiffuseColor(0.5, 0.8, 0.8)
        actor.GetProperty().SetAmbient(0.2)
        actor.GetProperty().SetAmbientColor(0.2, 0.2, 0.2)

        squad.SetPhiRoundness(prs.get())
        squad.SetThetaRoundness(trs.get())
        squad.SetToroidal(toroid.get())
        squad.SetThickness(thicks.get())
        squad.SetScale(1, 1, 1)
        SetTexture(actor, atext, self.renWin)

        # Create renderer stuff
        #
        ren = vtk.vtkRenderer()
        ren.SetAmbient(1.0, 1.0, 1.0)
        self.renWin.AddRenderer(ren)


        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(actor)
        ren.SetBackground(0.25, 0.2, 0.2)
        ren.ResetCamera()
        ren.GetActiveCamera().Zoom(1.2)
        ren.GetActiveCamera().Elevation(40)
        ren.GetActiveCamera().Azimuth(-20)

        # Associate the functions with the sliders and check buttons.
        #
        prs.config(command=partial(SetPhi, squad, self.renWin))
        trs.config(command=partial(SetTheta, squad, self.renWin))
        thicks.config(command=partial(SetThickness,squad, self.renWin))
        torbut.config(command=partial(SetToroid, squad, thicks, self.renWin))
        texbut.config(command=partial(SetTexture, actor, atext, self.renWin))

    def DoIt(self):
        self.SetUp()
        self.renWin.Render()
        self.tkrw.Render()
        self.root.update()
        # If you want to interact and use the sliders etc,
        # uncomment the following line.
        #self.root.mainloop()
        img_file = "squadViewer.png"
        Testing.compareImage(self.renWin, Testing.getAbsImagePath(img_file))
        Testing.interact()

if __name__ == '__main__':
    cases = [(SuperQuadricViewer, 'DoIt')]
    del SuperQuadricViewer
    Testing.main(cases)
