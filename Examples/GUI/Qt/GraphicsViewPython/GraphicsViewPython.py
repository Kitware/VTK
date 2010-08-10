#!/usr/bin/env python

import sys
from PyQt4 import QtOpenGL,QtGui,QtCore
import vtk

class MyOpenGLScene(QtGui.QGraphicsScene):
  def __init__(self, ctx):
    super(MyOpenGLScene, self).__init__()
    self.mCtx = ctx
    self.mQVTKItem = vtk.QVTKGraphicsItem(self.mCtx)
    self.addItem(self.mQVTKItem)
    self.mQVTKItem.setGeometry(20,20,128,128)
    self.mRen = vtk.vtkRenderer()
    self.mQVTKItem.GetRenderWindow().AddRenderer(self.mRen)
    cone = vtk.vtkConeSource()
    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInput(cone.GetOutput())
    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    self.mRen.AddViewProp(actor)
    self.mRen.ResetCamera()

class MyGraphicsView(QtGui.QGraphicsView):
  def __init__(self):
    super(MyGraphicsView, self).__init__()
    self.mWidget = vtk.QVTKWidget2()
    self.setViewport(self.mWidget)
    self.setViewportUpdateMode(QtGui.QGraphicsView.FullViewportUpdate)
    self.mScene = MyOpenGLScene(self.mWidget.context())
    self.setScene(self.mScene)
    self.mRen = vtk.vtkRenderer()
    self.mRen.SetBackground(0,0,0)
    self.mRen.SetBackground2(1,1,1)
    self.mRen.SetGradientBackground(1)
    textActor = vtk.vtkTextActor3D()
    textActor.SetInput("Qt & VTK!!")
    self.mRen.AddViewProp(textActor)
    self.mRen.ResetCamera()
    self.mWidget.GetRenderWindow().AddRenderer(self.mRen)
    self.mWidget.GetRenderWindow().SetSwapBuffers(0)

  def resizeEvent(self, evt):
    super(MyGraphicsView, self).resizeEvent(evt)
    s = evt.size()
    self.mWidget.GetRenderWindow().SetSize(s.width(), s.height())
    self.scene().setSceneRect(0, 0, s.width(), s.height())

  def drawBackground(self, painter, rect):
    painter.beginNativePainting();
    self.mWidget.GetRenderWindow().PushState();
    self.mWidget.GetRenderWindow().Render();
    self.mWidget.GetRenderWindow().PopState();
    painter.endNativePainting();



if __name__ == "__main__":
  app = QtGui.QApplication(sys.argv)
  win = MyGraphicsView()
  win.show()
  win.resize(800,600)
  sys.exit(app.exec_())
