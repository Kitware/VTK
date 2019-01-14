#!/usr/bin/env python
from vtk import *
import os.path

from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def setup_view(link, file, domain1, domain2, hue_range):
  reader = vtkDelimitedTextReader()
  reader.SetHaveHeaders(True)
  reader.SetFileName(file)

  ttg = vtkTableToGraph()
  ttg.SetInputConnection(0, reader.GetOutputPort())
  ttg.AddLinkVertex(domain1, domain1, False)
  ttg.AddLinkVertex(domain2, domain2, False)
  ttg.AddLinkEdge(domain1, domain2)

  cat = vtkStringToCategory()
  cat.SetInputConnection(ttg.GetOutputPort())
  cat.SetInputArrayToProcess(0,0,0,4,"domain")

  view = vtkGraphLayoutView()
  view.SetVertexLabelArrayName("label")
  view.VertexLabelVisibilityOn()
  view.SetVertexColorArrayName("category")
  view.ColorVerticesOn()
  rep = view.AddRepresentationFromInputConnection(cat.GetOutputPort())
  rep.SetSelectionType(2)
  rep.SetAnnotationLink(link)
  view.GetRenderWindow().SetSize(500,500)
  theme = vtkViewTheme.CreateMellowTheme()
  theme.SetLineWidth(5)
  theme.SetCellOpacity(0.9)
  theme.SetCellAlphaRange(0.5,0.5)
  theme.SetPointOpacity(0.5)
  theme.SetPointSize(10)
  theme.SetPointHueRange(hue_range[0], hue_range[1])
  theme.SetSelectedCellColor(1,0,1)
  theme.SetSelectedPointColor(1,0,1)
  view.ApplyViewTheme(theme)
  theme.FastDelete()

  view.ResetCamera()
  view.Render()

  return view

if __name__ == "__main__":
  data_dir = VTK_DATA_ROOT + "/Data/Infovis/"
  if not os.path.exists(data_dir):
    data_dir = VTK_DATA_ROOT + "/Data/Infovis/"
  dt_reader = vtkDelimitedTextReader()
  dt_reader.SetHaveHeaders(True)
  dt_reader.SetFileName(data_dir + "document-term.csv")
  dt_reader.Update()
  link = vtkAnnotationLink()
  link.AddDomainMap(dt_reader.GetOutput())

  tc_view = setup_view(
    link, data_dir + "term-concept.csv",
    "term", "concept", [0.2, 0.0])
  pd_view = setup_view(
    link, data_dir + "person-document.csv",
    "person", "document", [0.75, 0.25])

  updater = vtkViewUpdater()
  updater.AddAnnotationLink(link)
  updater.AddView(tc_view)
  updater.AddView(pd_view)

  tc_view.GetInteractor().Start()

