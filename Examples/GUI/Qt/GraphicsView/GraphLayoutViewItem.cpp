
#include "GraphLayoutViewItem.h"
#include "vtkGraphLayoutView.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkgl.h"
#include "QVTKInteractor.h"
#include "vtkXMLTreeReader.h"
#include "vtkRenderedTreeAreaRepresentation.h"
#include "vtkStringArray.h"
#include "vtkIdTypeArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkRenderedGraphRepresentation.h"
#include "vtkStringToNumeric.h"
#include "vtkViewTheme.h"
#include "vtkTextProperty.h"
#include <QFile>

GraphLayoutViewItem::GraphLayoutViewItem(QGLContext* ctx, QGraphicsItem* p)
  : QVTKGraphicsItem(ctx, p)
{
  GraphLayoutView.TakeReference(vtkGraphLayoutView::New());
  GraphLayoutView->SetRenderWindow(this->GetRenderWindow());

  QFile f1(":/Data/treetest.xml");
  f1.open(QIODevice::ReadOnly);
  QByteArray f1_data = f1.readAll();

  vtkSmartPointer<vtkXMLTreeReader> reader = vtkSmartPointer<vtkXMLTreeReader>::New();
  reader->SetXMLString(f1_data.data());
  reader->SetMaskArrays(true);
  reader->Update();
  vtkTree* t = reader->GetOutput();
  vtkSmartPointer<vtkStringArray> label = vtkSmartPointer<vtkStringArray>::New();
  label->SetName("edge label");
  vtkSmartPointer<vtkIdTypeArray> dist = vtkSmartPointer<vtkIdTypeArray>::New();
  dist->SetName("distance");
  for (vtkIdType i = 0; i < t->GetNumberOfEdges(); i++)
  {
    dist->InsertNextValue(i);
    switch (i % 3)
    {
      case 0:
        label->InsertNextValue("a");
        break;
      case 1:
        label->InsertNextValue("b");
        break;
      case 2:
        label->InsertNextValue("c");
        break;
    }
  }
  t->GetEdgeData()->AddArray(dist);
  t->GetEdgeData()->AddArray(label);

  vtkSmartPointer<vtkStringToNumeric> numeric = vtkSmartPointer<vtkStringToNumeric>::New();
  numeric->SetInputConnection(reader->GetOutputPort());

  GraphLayoutView->DisplayHoverTextOn();
  GraphLayoutView->SetLayoutStrategyToCircular();
  GraphLayoutView->SetVertexLabelArrayName("name");
  GraphLayoutView->VertexLabelVisibilityOn();
  GraphLayoutView->SetVertexColorArrayName("size");
  GraphLayoutView->ColorVerticesOn();
  GraphLayoutView->SetRepresentationFromInputConnection(numeric->GetOutputPort());
  GraphLayoutView->SetEdgeColorArrayName("distance");
  GraphLayoutView->ColorEdgesOn();
  GraphLayoutView->SetEdgeLabelArrayName("edge label");
  GraphLayoutView->EdgeLabelVisibilityOn();
  vtkRenderedGraphRepresentation* rep =
    vtkRenderedGraphRepresentation::SafeDownCast(GraphLayoutView->GetRepresentation());
  rep->SetVertexHoverArrayName("name");
  rep->SetEdgeHoverArrayName("edge label");

  GraphLayoutView->SetHideVertexLabelsOnInteraction(1);
  GraphLayoutView->SetHideEdgeLabelsOnInteraction(1);

  GraphLayoutView->ResetCamera();

}

GraphLayoutViewItem::~GraphLayoutViewItem()
{
}
