
#include "TreeRingViewItem.h"
#include "vtkTreeRingView.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkgl.h"
#include "QVTKInteractor.h"
#include "vtkXMLTreeReader.h"
#include "vtkRenderedTreeAreaRepresentation.h"
#include "vtkViewTheme.h"
#include "vtkTextProperty.h"

TreeRingViewItem::TreeRingViewItem(QGLContext* ctx, QGraphicsItem* p)
  : QVTKGraphicsItem(ctx, p)
{
  TreeRingView.TakeReference(vtkTreeRingView::New());
  TreeRingView->SetInteractor(this->GetInteractor());
  TreeRingView->SetRenderWindow(this->GetRenderWindow());

  QString f1 = "/home/cjstimp/vtk/VTKData/Data/Infovis/XML/vtkclasses.xml";
  QString f2 = "/home/cjstimp/vtk/VTKData/Data/Infovis/XML/vtklibrary.xml";


  vtkSmartPointer<vtkXMLTreeReader> reader1 = vtkSmartPointer<vtkXMLTreeReader>::New();
  reader1->SetFileName(f1.toAscii().data());
  reader1->SetEdgePedigreeIdArrayName("graph edge");
  reader1->GenerateVertexPedigreeIdsOff();
  reader1->SetVertexPedigreeIdArrayName("id");

  vtkSmartPointer<vtkXMLTreeReader> reader2 = vtkSmartPointer<vtkXMLTreeReader>::New();
  reader2->SetFileName(f2.toAscii().data());
  reader2->SetEdgePedigreeIdArrayName("tree edge");
  reader2->GenerateVertexPedigreeIdsOff();
  reader2->SetVertexPedigreeIdArrayName("id");

  reader1->Update();
  reader2->Update();

  TreeRingView->DisplayHoverTextOn();
  TreeRingView->SetTreeFromInputConnection(reader2->GetOutputPort());
  TreeRingView->SetGraphFromInputConnection(reader1->GetOutputPort());

  TreeRingView->SetAreaColorArrayName("VertexDegree");

  // Uncomment for edge colors
  //TreeRingView->SetEdgeColorArrayName("graph edge");
  //TreeRingView->SetColorEdges(true);

  // Uncomment for edge labels
  //TreeRingView->SetEdgeLabelArrayName("graph edge");
  //TreeRingView->SetEdgeLabelVisibility(true);

  TreeRingView->SetAreaLabelArrayName("id");
  TreeRingView->SetAreaLabelVisibility(true);
  TreeRingView->SetAreaHoverArrayName("id");
  TreeRingView->SetAreaSizeArrayName("VertexDegree");
  vtkRenderedTreeAreaRepresentation::SafeDownCast(TreeRingView->GetRepresentation())->SetGraphHoverArrayName("graph edge");

  vtkViewTheme* const theme = vtkViewTheme::CreateMellowTheme();
  theme->SetLineWidth(1);
  theme->GetPointTextProperty()->ShadowOn();
  TreeRingView->ApplyViewTheme(theme);
  theme->Delete();

  TreeRingView->ResetCamera();


}

TreeRingViewItem::~TreeRingViewItem()
{
}
