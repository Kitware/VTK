/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTanglegramItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTanglegramItem.h"

#include "vtkColorSeries.h"
#include "vtkContext2D.h"
#include "vtkDataSetAttributes.h"
#include "vtkDendrogramItem.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkTree.h"
#include "vtkTreeBFSIterator.h"
#include "vtkTreeDFSIterator.h"
#include "vtkVariantArray.h"

#include <queue>

vtkStandardNewMacro(vtkTanglegramItem);

//-----------------------------------------------------------------------------
vtkTanglegramItem::vtkTanglegramItem()
{
  this->Dendrogram1 = vtkSmartPointer<vtkDendrogramItem>::New();
  this->Dendrogram1->ExtendLeafNodesOn();
  this->AddItem(this->Dendrogram1);

  this->Dendrogram2 = vtkSmartPointer<vtkDendrogramItem>::New();
  this->Dendrogram2->ExtendLeafNodesOn();
  this->AddItem(this->Dendrogram2);

  this->Table = vtkSmartPointer<vtkTable>::New();
  this->Tree1Label = NULL;
  this->Tree2Label = NULL;

  this->LookupTable = vtkSmartPointer<vtkLookupTable>::New();

  this->PositionSet = false;
  this->TreeReordered = false;
  this->Interactive = true;

  this->Orientation = vtkDendrogramItem::LEFT_TO_RIGHT;

  this->MinimumVisibleFontSize = 8;
  this->LabelSizeDifference = 4;

  this->CorrespondenceLineWidth = 2.0;
}

//-----------------------------------------------------------------------------
vtkTanglegramItem::~vtkTanglegramItem()
{
  delete []this->Tree1Label;
  delete []this->Tree2Label;
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::SetTree1(vtkTree *tree)
{
  this->Dendrogram1->SetTree(tree);
  this->Dendrogram1->SetOrientation(this->Orientation);
  this->PositionSet = false;
  this->TreeReordered = false;
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::SetTree2(vtkTree *tree)
{
  this->Dendrogram2->SetTree(tree);
  this->Dendrogram2->SetOrientation((this->Orientation + 2) % 4);
  this->PositionSet = false;
  this->TreeReordered = false;
}

//-----------------------------------------------------------------------------
vtkTable * vtkTanglegramItem::GetTable()
{
  return this->Table;
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::SetTable(vtkTable *table)
{
  if (table == NULL)
    {
    return;
    }

  this->Table = table;

  this->SourceNames = vtkStringArray::SafeDownCast(this->Table->GetColumn(0));
  this->GenerateLookupTable();
  this->TreeReordered = false;
}

//-----------------------------------------------------------------------------
bool vtkTanglegramItem::Paint(vtkContext2D *painter)
{
  this->RefreshBuffers(painter);

  if (!this->TreeReordered)
    {
    this->ReorderTree();

    // this will force Dendrogram2's PrunedTree to re-copy itself from the
    // newly rearranged tree.
    this->Dendrogram2->PrepareToPaint(painter);
    }

  if (!this->PositionSet)
    {
    this->PositionTree2();
    }

  this->PaintChildren(painter);

  if (this->Table != NULL)
    {
    this->PaintCorrespondenceLines(painter);
    }

  if (this->Tree1Label != NULL || this->Tree2Label != NULL)
    {
    this->PaintTreeLabels(painter);
    }

  return true;
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::RefreshBuffers(vtkContext2D *painter)
{
  this->Dendrogram1->PrepareToPaint(painter);
  this->Spacing = this->Dendrogram1->GetLeafSpacing();
  this->Dendrogram1->GetBounds(this->Tree1Bounds);
  this->LabelWidth1 = this->Dendrogram1->GetLabelWidth();

  this->Dendrogram2->PrepareToPaint(painter);
  this->Dendrogram2->GetBounds(this->Tree2Bounds);
  this->LabelWidth2 = this->Dendrogram2->GetLabelWidth();

  this->Tree1Names = vtkStringArray::SafeDownCast(
    this->Dendrogram1->GetPrunedTree()->GetVertexData()->
    GetAbstractArray("node name"));

  this->Tree2Names = vtkStringArray::SafeDownCast(
    this->Dendrogram2->GetPrunedTree()->GetVertexData()->
    GetAbstractArray("node name"));
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::PositionTree2()
{
  // values used to calculate the amount of space we should leave between
  // the two trees.
  double averageX =
    ((abs(this->Tree1Bounds[1] - this->Tree1Bounds[0]) +
      abs(this->Tree2Bounds[1] - this->Tree2Bounds[0])) / 2.0);
  double averageY =
    ((abs(this->Tree1Bounds[3] - this->Tree1Bounds[2]) +
      abs(this->Tree2Bounds[3] - this->Tree2Bounds[2])) / 2.0);

  // the starting X position for tree #2
  double x, x1, x2;

  // the starting Y position for tree #2
  double y, y1, y2;

  switch(this->Orientation)
    {
    case vtkDendrogramItem::DOWN_TO_UP:
      x1 = (this->Tree1Bounds[1] + this->Tree1Bounds[0]) / 2.0;
      x2 = (this->Tree2Bounds[1] + this->Tree2Bounds[0]) / 2.0;
      x = x1 - x2;

      y = this->Tree1Bounds[3] +
        abs(this->Tree2Bounds[3] - this->Tree2Bounds[2]) +
        averageY;
      break;

    case vtkDendrogramItem::UP_TO_DOWN:
      x1 = (this->Tree1Bounds[1] + this->Tree1Bounds[0]) / 2.0;
      x2 = (this->Tree2Bounds[1] + this->Tree2Bounds[0]) / 2.0;
      x = x1 - x2;

      y = this->Tree1Bounds[2] -
        abs(this->Tree2Bounds[3] - this->Tree2Bounds[2]) -
        averageY;
      break;

    case vtkDendrogramItem::RIGHT_TO_LEFT:

      x = this->Tree1Bounds[0] -
        abs(this->Tree2Bounds[1] - this->Tree2Bounds[0]) -
        averageX;

      y1 = (this->Tree1Bounds[3] + this->Tree1Bounds[2]) / 2.0;
      y2 = (this->Tree2Bounds[3] + this->Tree2Bounds[2]) / 2.0;
      y = y1 - y2;
      break;

    case vtkDendrogramItem::LEFT_TO_RIGHT:
    default:

      x = this->Tree1Bounds[1] +
        abs(this->Tree2Bounds[1] - this->Tree2Bounds[0]) +
        averageX;

      y1 = (this->Tree1Bounds[3] + this->Tree1Bounds[2]) / 2.0;
      y2 = (this->Tree2Bounds[3] + this->Tree2Bounds[2]) / 2.0;
      y = y1 - y2;
      break;
    }

  this->Dendrogram2->SetPosition(x, y);
  this->PositionSet = true;
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::PaintCorrespondenceLines(vtkContext2D *painter)
{
  int textOrientation = painter->GetTextProp()->GetOrientation();
  painter->GetTextProp()->SetOrientation(0.0);

  float previousWidth = painter->GetPen()->GetWidth();
  painter->GetPen()->SetWidth(this->CorrespondenceLineWidth);

  for (vtkIdType row = 0; row < this->Table->GetNumberOfRows();
       ++row)
    {
    std::string source = this->SourceNames->GetValue(row);
    vtkIdType tree1Index = this->Tree1Names->LookupValue(source);
    if (tree1Index == -1)
      {
      continue;
      }

    double sourcePosition[2] = {0, 0};
    if (!this->Dendrogram1->GetPositionOfVertex(source, sourcePosition))
      {
      continue;
      }
    double sourceEdgePosition[2];
    sourceEdgePosition[0] = sourcePosition[0];
    sourceEdgePosition[1] = sourcePosition[1];

    for (vtkIdType col = 1; col < this->Table->GetNumberOfColumns(); ++col)
      {
      double matrixValue = this->Table->GetValue(row, col).ToDouble();
      if (matrixValue == 0.0)
        {
        continue;
        }

      std::string target = this->Table->GetColumnName(col);
      if (target == "")
        {
        continue;
        }

      vtkIdType tree2Index = this->Tree2Names->LookupValue(target);
      if (tree2Index == -1)
        {
        continue;
        }

      double targetPosition[2] = {0, 0};
      if (!this->Dendrogram2->GetPositionOfVertex(target, targetPosition))
        {
        continue;
        }
      double targetEdgePosition[2];
      targetEdgePosition[0] = targetPosition[0];
      targetEdgePosition[1] = targetPosition[1];

      int fontSize =
        painter->ComputeFontSizeForBoundedString("Igq", VTK_FLOAT_MAX,
                                                 this->Spacing);

      switch(this->Orientation)
        {
        case vtkDendrogramItem::DOWN_TO_UP:
          if (fontSize < this->MinimumVisibleFontSize)
            {
            sourcePosition[1] = this->Tree1Bounds[3] + this->Spacing;
            targetPosition[1] = this->Tree2Bounds[2] - this->Spacing;
            }
          else
            {
            float stringBounds[4];
            painter->ComputeStringBounds(source, stringBounds);
            sourcePosition[1] =
              this->Tree1Bounds[3] - (this->LabelWidth1 - stringBounds[2]);

            sourceEdgePosition[1] = this->Tree1Bounds[3] + this->Spacing;

            targetEdgePosition[1] = this->Tree2Bounds[2] - this->Spacing;

            painter->ComputeStringBounds(target, stringBounds);
            targetPosition[1] =
              this->Tree2Bounds[2] + (this->LabelWidth2 - stringBounds[2]);
            }
          break;

        case vtkDendrogramItem::UP_TO_DOWN:
          if (fontSize < this->MinimumVisibleFontSize)
            {
            sourcePosition[1] = this->Tree1Bounds[2] - this->Spacing;
            targetPosition[1] = this->Tree2Bounds[3] + this->Spacing;
            }
          else
            {
            float stringBounds[4];
            painter->ComputeStringBounds(source, stringBounds);
            sourcePosition[1] =
              this->Tree1Bounds[2] + (this->LabelWidth1 - stringBounds[2]);

            sourceEdgePosition[1] = this->Tree1Bounds[2] - this->Spacing;

            targetEdgePosition[1] = this->Tree2Bounds[3] + this->Spacing;

            painter->ComputeStringBounds(target, stringBounds);
            targetPosition[1] =
              this->Tree2Bounds[3] - (this->LabelWidth2 - stringBounds[2]);
            }
          break;

        case vtkDendrogramItem::RIGHT_TO_LEFT:
          if (fontSize < this->MinimumVisibleFontSize)
            {
            sourcePosition[0] = this->Tree1Bounds[0] - this->Spacing;
            targetPosition[0] = this->Tree2Bounds[1] + this->Spacing;
            }
          else
            {
            float stringBounds[4];
            painter->ComputeStringBounds(source, stringBounds);
            sourcePosition[0] =
              this->Tree1Bounds[0] + (this->LabelWidth1 - stringBounds[2]);

            sourceEdgePosition[0] = this->Tree1Bounds[0] - this->Spacing;

            targetEdgePosition[0] = this->Tree2Bounds[1] + this->Spacing;

            painter->ComputeStringBounds(target, stringBounds);
            targetPosition[0] =
              this->Tree2Bounds[1] - (this->LabelWidth2 - stringBounds[2]);
            }
          break;

        case vtkDendrogramItem::LEFT_TO_RIGHT:
        default:
          if (fontSize < this->MinimumVisibleFontSize)
            {
            sourcePosition[0] = this->Tree1Bounds[1] + this->Spacing;
            targetPosition[0] = this->Tree2Bounds[0] - this->Spacing;
            }
          else
            {
            float stringBounds[4];
            painter->ComputeStringBounds(source, stringBounds);
            sourcePosition[0] =
              this->Tree1Bounds[1] - (this->LabelWidth1 - stringBounds[2]);

            sourceEdgePosition[0] = this->Tree1Bounds[1] + this->Spacing;

            targetEdgePosition[0] = this->Tree2Bounds[0] - this->Spacing;

            painter->ComputeStringBounds(target, stringBounds);
            targetPosition[0] =
              this->Tree2Bounds[0] + (this->LabelWidth2 - stringBounds[2]);
            }
          break;
        }

      double color[4];
      this->LookupTable->GetColor(matrixValue, color);

      if (fontSize < this->MinimumVisibleFontSize)
        {
        painter->GetPen()->SetColorF(color[0], color[1], color[2]);
        painter->DrawLine(sourcePosition[0], sourcePosition[1],
                          targetPosition[0], targetPosition[1]);
        continue;
        }

      painter->GetPen()->SetColorF(0.0, 0.0, 0.0);
      painter->GetPen()->SetLineType(vtkPen::DOT_LINE);

      painter->DrawLine(sourcePosition[0], sourcePosition[1],
                        sourceEdgePosition[0], sourceEdgePosition[1]);

      painter->DrawLine(targetEdgePosition[0], targetEdgePosition[1],
                        targetPosition[0], targetPosition[1]);

      painter->GetPen()->SetColorF(color[0], color[1], color[2]);
      painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
      painter->DrawLine(sourceEdgePosition[0], sourceEdgePosition[1],
                        targetEdgePosition[0], targetEdgePosition[1]);
      }
    }

  painter->GetPen()->SetColorF(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetOrientation(textOrientation);
  painter->GetPen()->SetWidth(previousWidth);
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::PaintTreeLabels(vtkContext2D *painter)
{
  int fontSize = painter->GetTextProp()->GetFontSize();
  painter->GetTextProp()->SetFontSize(fontSize + this->LabelSizeDifference);

  int justification = painter->GetTextProp()->GetJustification();
  painter->GetTextProp()->SetJustificationToCentered();

  int textOrientation = painter->GetTextProp()->GetOrientation();
  painter->GetTextProp()->SetOrientation(0.0);

  painter->GetTextProp()->BoldOn();

  double x, y;
  switch(this->Orientation)
    {
    case vtkDendrogramItem::DOWN_TO_UP:
      if (this->Tree1Label != NULL)
        {
        x = (this->Tree1Bounds[1] + this->Tree1Bounds[0]) / 2.0;
        y = this->Tree1Bounds[2] - this->Spacing;
        painter->DrawString(x, y, this->Tree1Label);
        }

      if (this->Tree2Label != NULL)
        {
        x = (this->Tree2Bounds[1] + this->Tree2Bounds[0]) / 2.0;
        y = this->Tree2Bounds[3] + this->Spacing;
        painter->DrawString(x, y, this->Tree2Label);
        }
      break;

    case vtkDendrogramItem::UP_TO_DOWN:
      if (this->Tree1Label != NULL)
        {
        x = (this->Tree1Bounds[1] + this->Tree1Bounds[0]) / 2.0;
        y = this->Tree1Bounds[3] + this->Spacing;
        painter->DrawString(x, y, this->Tree1Label);
        }

      if (this->Tree2Label != NULL)
        {
        x = (this->Tree2Bounds[1] + this->Tree2Bounds[0]) / 2.0;
        y = this->Tree2Bounds[2] - this->Spacing;
        painter->DrawString(x, y, this->Tree2Label);
        }
      break;

    case vtkDendrogramItem::RIGHT_TO_LEFT:
      if (this->Tree1Label != NULL)
        {
        x = this->Tree1Bounds[0] + this->LabelWidth1 + this->Spacing / 2.0;
        y = this->Tree1Bounds[3] + this->Spacing * 2.0;
        painter->DrawString(x, y, this->Tree1Label);
        }

      if (this->Tree2Label != NULL)
        {
        x = this->Tree2Bounds[1] - this->LabelWidth2 - this->Spacing / 2.0;
        y = this->Tree2Bounds[3] + this->Spacing * 2.0;
        painter->DrawString(x, y, this->Tree2Label);
        }
      break;

    case vtkDendrogramItem::LEFT_TO_RIGHT:
    default:
      if (this->Tree1Label != NULL)
        {
        x = this->Tree1Bounds[1] - this->LabelWidth1 - this->Spacing / 2.0;
        y = this->Tree1Bounds[3] + this->Spacing * 2.0;
        painter->DrawString(x, y, this->Tree1Label);
        }
  painter->GetTextProp()->SetOrientation(0.0);
      if (this->Tree2Label != NULL)
        {
        x = this->Tree2Bounds[0] + this->LabelWidth1 + this->Spacing / 2.0;
        y = this->Tree2Bounds[3] + this->Spacing * 2.0;
        painter->DrawString(x, y, this->Tree2Label);
        }
      break;
    }


  painter->GetTextProp()->SetFontSize(fontSize);
  painter->GetTextProp()->SetJustification(justification);
  painter->GetTextProp()->SetOrientation(textOrientation);
  painter->GetTextProp()->BoldOff();
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::ReorderTree()
{
  if (this->Dendrogram1->GetTree()->GetNumberOfVertices() == 0 ||
      this->Dendrogram2->GetTree()->GetNumberOfVertices() == 0 ||
      this->Table == NULL)
    {
    return;
    }

  vtkTree *tree = this->Dendrogram2->GetTree();

  this->Tree2Names = vtkStringArray::SafeDownCast(
    tree->GetVertexData()-> GetAbstractArray("node name"));

  vtkNew<vtkTreeBFSIterator> bfsIterator;
  bfsIterator->SetTree(tree);
  bfsIterator->SetStartVertex(tree->GetRoot());
  while(bfsIterator->HasNext())
    {
    vtkIdType vertex = bfsIterator->Next();
    if (tree->GetNumberOfChildren(vertex) < 2)
      {
      continue;
      }
    this->ReorderTreeAtVertex(vertex, tree);
    }

  this->TreeReordered = true;
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::ReorderTreeAtVertex(vtkIdType parent, vtkTree *tree)
{
  // Set up a priority queue to reorganize the vertices.  This queue sorts all
  // the children of parent based on their "score".  This score roughly
  // correponds to where the children should be positioned within the
  // dendrogram to minimize crossings.  See the comments within
  // GetPositionScoreForVertex() for more info.
  std::priority_queue<vtkDendrogramItem::WeightedVertex,
                      std::vector<vtkDendrogramItem::WeightedVertex>,
                      vtkDendrogramItem::CompareWeightedVertices> queue;

  for(vtkIdType i = 0; i < tree->GetNumberOfChildren(parent); ++i)
    {
    vtkIdType child = tree->GetChild(parent, i);
    double score = this->GetPositionScoreForVertex(child, tree);
    vtkDendrogramItem::WeightedVertex wv = {child, score};
    queue.push(wv);
    }

  vtkNew<vtkIdTypeArray> newChildOrder;
  while (!queue.empty())
    {
    vtkDendrogramItem::WeightedVertex wv = queue.top();
    queue.pop();
    newChildOrder->InsertNextValue(wv.ID);
    }

  tree->ReorderChildren(parent, newChildOrder.GetPointer());
}

//-----------------------------------------------------------------------------
double vtkTanglegramItem::GetPositionScoreForVertex(vtkIdType vertex,
                                                    vtkTree *tree)
{
  // score will be the average "height" (y coordinate for unrotated tanglegram)
  // of all the leaf nodes in the fixed tree that are associated with leaf nodes
  // that descend from the vertex parameter.
  double score = 0.0;
  double numLeafNodesFound = 0.0;
  double position[2] = {0, 0};

  // which dimension (x or y) should be used to calculate this vertex's score.
  // this is determined by the orientation of our tanglegram.
  int dimension = 1;
  if (this->Orientation == vtkDendrogramItem::DOWN_TO_UP ||
      this->Orientation == vtkDendrogramItem::UP_TO_DOWN)
    {
    dimension = 0;
    }

  vtkNew<vtkTreeDFSIterator> dfsIterator;
  dfsIterator->SetTree(tree);
  dfsIterator->SetStartVertex(vertex);

  // search for leaf nodes that descend from this vertex
  while(dfsIterator->HasNext())
    {
    vtkIdType v = dfsIterator->Next();
    if (!tree->IsLeaf(v))
      {
      continue;
      }

    // get this leaf node's name
    std::string tree2Name = this->Tree2Names->GetValue(v);

    // find where this name appears in the correspondence table
    vtkDoubleArray *column = vtkDoubleArray::SafeDownCast(
      this->Table->GetColumnByName(tree2Name.c_str()));

    if (column == NULL)
      {
      continue;
      }

    for (vtkIdType row = 0; row < column->GetNumberOfTuples(); ++row)
      {
      if (column->GetValue(row) > 0.0)
        {
        // get the position of the associated leaf node in the fixed tree
        // and use it to update our score.
        std::string tree1Name = this->Table->GetValue(row, 0).ToString();
        if (!this->Dendrogram1->GetPositionOfVertex(tree1Name, position))
          {
          continue;
          }
        score += position[dimension];
        ++numLeafNodesFound;
        }
      }
    }

  if (numLeafNodesFound == 0)
    {
    return VTK_DOUBLE_MAX;
    }

  int sign = 1;
  if (this->Orientation == vtkDendrogramItem::LEFT_TO_RIGHT ||
      this->Orientation == vtkDendrogramItem::UP_TO_DOWN)
    {
    // multiply by -1 because we want high numbers to be near the top.
    sign = -1;
    }

  return sign * score / numLeafNodesFound;
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::SetOrientation(int orientation)
{
  this->Orientation = orientation;
  this->Dendrogram1->SetOrientation(this->Orientation);
  this->Dendrogram2->SetOrientation((this->Orientation + 2) % 4);
}

//-----------------------------------------------------------------------------
int vtkTanglegramItem::GetOrientation()
{
  return this->Orientation;
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::GenerateLookupTable()
{
  this->LookupTable->SetNumberOfTableValues(255);
  this->LookupTable->Build();

  vtkNew<vtkColorSeries> colorSeries;
  colorSeries->SetColorScheme(vtkColorSeries::BREWER_QUALITATIVE_ACCENT);
  colorSeries->BuildLookupTable(this->LookupTable);

  this->LookupTable->IndexedLookupOff();

  double min = VTK_DOUBLE_MAX;
  double max = VTK_DOUBLE_MIN;

  for (vtkIdType row = 0; row < this->Table->GetNumberOfRows();
       ++row)
    {
    for (vtkIdType col = 1; col < this->Table->GetNumberOfColumns(); ++col)
      {
      double d = this->Table->GetValue(row, col).ToDouble();
      if (d == 0.0)
        {
        continue;
        }
      if (d > max)
        {
        max = d;
        }
      if (d < min)
        {
        min = d;
        }
      }
    }

  this->LookupTable->SetRange(min, max);
}

//-----------------------------------------------------------------------------
bool vtkTanglegramItem::MouseDoubleClickEvent(
  const vtkContextMouseEvent &event)
{
  bool tree1Changed = this->Dendrogram1->MouseDoubleClickEvent(event);
  bool tree2Changed = false;
  if (!tree1Changed)
    {
    tree2Changed = this->Dendrogram2->MouseDoubleClickEvent(event);
    }

  return tree1Changed || tree2Changed;
}

//-----------------------------------------------------------------------------
float vtkTanglegramItem::GetTreeLineWidth()
{
  return this->Dendrogram1->GetLineWidth();
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::SetTreeLineWidth(float width)
{
  this->Dendrogram1->SetLineWidth(width);
  this->Dendrogram2->SetLineWidth(width);
}

//-----------------------------------------------------------------------------
bool vtkTanglegramItem::Hit(const vtkContextMouseEvent &vtkNotUsed(mouse))
{
  // If we are interactive, we want to catch anything that propagates to the
  // background, otherwise we do not want any mouse events.
  return this->Interactive;
}

//-----------------------------------------------------------------------------
void vtkTanglegramItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
