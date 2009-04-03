from vtk import *
import os.path

def setup_view(link, file, domain1, domain2, hue_range):
  reader = vtkDelimitedTextReader()
  reader.SetHaveHeaders(True)
  reader.SetFileName(file)

  ttg = vtkTableToGraph()
  ttg.SetInputConnection(reader.GetOutputPort())
  ttg.SetInputConnection(reader.GetOutputPort())
  ttg.AddLinkVertex(domain1, domain1, False)
  ttg.AddLinkVertex(domain2, domain2, False)
  ttg.AddLinkEdge(domain1, domain2)

  cat = vtkStringToCategory()
  cat.SetInputConnection(ttg.GetOutputPort())
  cat.SetInputArrayToProcess(0,0,0,4,"domain")

  view = vtkGraphLayoutView()
  view.SetSelectionType(2)
  view.SetVertexLabelArrayName("label")
  view.VertexLabelVisibilityOn()
  view.SetVertexColorArrayName("category")
  view.ColorVerticesOn()
  rep = view.AddRepresentationFromInputConnection(cat.GetOutputPort())
  rep.SetSelectionLink(link)
  win = vtkRenderWindow()
  win.SetSize(500,500)
  view.SetupRenderWindow(win)
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

  view.GetRenderer().ResetCamera()
  win.Render()

  return (view, win)

if __name__ == "__main__":
  data_dir = "../../../../VTKData/Data/Infovis/"
  if not os.path.exists(data_dir):
    data_dir = "../../../../../VTKData/Data/Infovis/"
  dt_reader = vtkDelimitedTextReader()
  dt_reader.SetHaveHeaders(True)
  dt_reader.SetFileName(data_dir + "document-term.csv")
  dt_reader.Update()
  link = vtkSelectionLink()
  link.AddDomainMap(dt_reader.GetOutput())

  (tc_view, tc_win) = setup_view(
    link, data_dir + "term-concept.csv",
    "term", "concept", [0.2, 0.0])
  (pd_view, pd_win) = setup_view(
    link, data_dir + "person-document.csv",
    "person", "document", [0.75, 0.25])

  updater = vtkViewUpdater()
  updater.AddView(tc_view)
  updater.AddView(pd_view)

  tc_win.GetInteractor().Initialize()
  tc_win.GetInteractor().Start()

