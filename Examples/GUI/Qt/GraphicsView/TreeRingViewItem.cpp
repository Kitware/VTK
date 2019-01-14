
#include "TreeRingViewItem.h"
#include "vtkTreeRingView.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "QVTKInteractor.h"
#include "vtkXMLTreeReader.h"
#include "vtkRenderedTreeAreaRepresentation.h"
#include "vtkViewTheme.h"
#include "vtkTextProperty.h"
#include "vtkRenderer.h"
#include <QFile>

TreeRingViewItem::TreeRingViewItem(QGLContext* ctx, QGraphicsItem* p)
  : QVTKGraphicsItem(ctx, p)
{
  QPalette pal = this->palette();
  pal.setColor(QPalette::Window, QColor(255,255,255,250));
  this->setPalette(pal);

  TreeRingView.TakeReference(vtkTreeRingView::New());
  TreeRingView->SetRenderWindow(this->GetRenderWindow());

  QFile f1(":/Data/vtkclasses.xml");
  f1.open(QIODevice::ReadOnly);
  QByteArray f1_data = f1.readAll();

  QFile f2(":/Data/vtklibrary.xml");
  f2.open(QIODevice::ReadOnly);
  QByteArray f2_data = f2.readAll();

  vtkSmartPointer<vtkXMLTreeReader> reader1 = vtkSmartPointer<vtkXMLTreeReader>::New();
  reader1->SetXMLString(f1_data.data());
  reader1->SetEdgePedigreeIdArrayName("graph edge");
  reader1->GenerateVertexPedigreeIdsOff();
  reader1->SetVertexPedigreeIdArrayName("id");

  vtkSmartPointer<vtkXMLTreeReader> reader2 = vtkSmartPointer<vtkXMLTreeReader>::New();
  reader2->SetXMLString(f2_data.data());
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

  this->TreeRingView->GetRenderer()->SetGradientBackground(0);
  this->TreeRingView->GetRenderer()->SetBackground(0.1,0.1,0.1);

  TreeRingView->ResetCamera();


}

TreeRingViewItem::~TreeRingViewItem()
{
}
