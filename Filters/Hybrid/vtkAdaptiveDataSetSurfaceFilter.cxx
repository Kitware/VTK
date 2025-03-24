// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAdaptiveDataSetSurfaceFilter.h"

#include "vtkBitArray.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMatrix4x4.h"
#include "vtkMergePoints.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"

VTK_ABI_NAMESPACE_BEGIN
static const unsigned int VonNeumannCursors3D[] = { 0, 1, 2, 4, 5, 6 };
static const unsigned int VonNeumannOrientations3D[] = { 2, 1, 0, 0, 1, 2 };
static const unsigned int VonNeumannOffsets3D[] = { 0, 0, 0, 1, 1, 1 };

vtkStandardNewMacro(vtkAdaptiveDataSetSurfaceFilter);

enum class vtkAdaptiveDataSetSurfaceFilter::ShapeState : uint8_t
{
  VISIBLE = 0,
  OUT_OF_SCREEN = 1,
  SUB_PIXEL = 2,
};

//------------------------------------------------------------------------------
vtkAdaptiveDataSetSurfaceFilter::vtkAdaptiveDataSetSurfaceFilter()
{
  this->Merging = false;

  // vtkGeometryFilter allows an optional 2nd input. Need to
  // disable this.
  this->Superclass::SetNumberOfInputPorts(1);
}

//------------------------------------------------------------------------------
vtkAdaptiveDataSetSurfaceFilter::~vtkAdaptiveDataSetSurfaceFilter() = default;

//------------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->InData)
  {
    os << indent << "InData:\n";
    this->InData->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "InData: ( none )\n";
  }

  if (this->OutData)
  {
    os << indent << "OutData:\n";
    this->OutData->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "OutData: ( none )\n";
  }

  if (this->Points)
  {
    os << indent << "Points:\n";
    this->Points->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Points: ( none )\n";
  }

  if (this->Cells)
  {
    os << indent << "Cells:\n";
    this->Cells->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Cells: ( none )\n";
  }

  if (this->Mask)
  {
    os << indent << "Mask:\n";
    this->Mask->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Mask: ( none )\n";
  }

  if (this->Renderer)
  {
    os << indent << "Renderer:\n";
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Renderer: ( none )\n";
  }

  if (this->ModelViewMatrix)
  {
    os << indent << "ModelViewMatrix:\n";
    this->ModelViewMatrix->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "ModelViewMatrix: ( none )\n";
  }

  if (this->ProjectionMatrix)
  {
    os << indent << "ProjectionMatrix:\n";
    this->ProjectionMatrix->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "ProjectionMatrix: ( none )\n";
  }

  os << indent << "Dimension: " << this->Dimension << endl;
  os << indent << "Orientation: " << this->Orientation << endl;
  os << indent << "ViewPointDepend: " << this->ViewPointDepend << endl;
  os << indent << "Axis1: " << this->Axis1 << endl;
  os << indent << "Axis2: " << this->Axis2 << endl;
  os << indent << "FixedLevelMax: " << this->FixedLevelMax << endl;
  os << indent << "LastRendererSize: " << this->LastRendererSize[0] << ", "
     << this->LastRendererSize[1] << endl;
  os << indent << "IsParallel: " << this->IsParallel << endl;
  os << indent << "MaxLevel: " << this->MaxLevel << endl;
}

//------------------------------------------------------------------------------
int vtkAdaptiveDataSetSurfaceFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  vtkDataObject* input = vtkDataObject::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int objType = input->GetDataObjectType();
  if (objType != VTK_HYPER_TREE_GRID)
  {
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  return this->DataObjectExecute(input, output);
}

//------------------------------------------------------------------------------
int vtkAdaptiveDataSetSurfaceFilter::DataObjectExecute(vtkDataObject* inputDS, vtkPolyData* output)
{
  // Retrieve input grid
  vtkHyperTreeGrid* input = vtkHyperTreeGrid::SafeDownCast(inputDS);
  if (!input)
  {
    vtkErrorMacro("pre: input_not_HyperTreeGrid: " << inputDS->GetClassName());
    return 0;
  }

  if (!this->Renderer)
  {
    vtkErrorMacro("No renderer specified.");
    return 0;
  }

  // Retrieve useful grid parameters for speed of access
  this->Dimension = input->GetDimension();
  this->Orientation = input->GetOrientation();

  // Initialize output cell data
  this->InData = static_cast<vtkDataSetAttributes*>(input->GetCellData());
  this->OutData = static_cast<vtkDataSetAttributes*>(output->GetCellData());
  this->OutData->CopyAllocate(this->InData);

  if (this->Dimension == 1)
  {
    input->Get1DAxis(this->Axis1);
  }
  else if (this->Dimension == 2)
  {
    input->Get2DAxes(this->Axis1, this->Axis2);
  }

  vtkCamera* cam = this->Renderer->GetActiveCamera();

  this->ModelViewMatrix = cam->GetModelViewTransformMatrix();
  double aspect = this->LastRendererSize[0] / static_cast<double>(this->LastRendererSize[1]);
  this->ProjectionMatrix = cam->GetProjectionTransformMatrix(aspect, -1, 1);

  this->IsParallel = this->ProjectionMatrix->GetElement(3, 3) == 1.0;

  // Extract geometry from hyper tree grid
  this->ProcessTrees(input, output);

  this->UpdateProgress(1.);

  return 1;
}

//------------------------------------------------------------------------------
int vtkAdaptiveDataSetSurfaceFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::ProcessTrees(vtkHyperTreeGrid* input, vtkPolyData* output)
{
  if (this->Points)
  {
    this->Points->Delete();
  }
  // Create storage for corners of leaf cells
  this->Points = vtkPoints::New();

  // Create storage for unstructured leaf cells
  if (this->Cells)
  {
    this->Cells->Delete();
  }
  this->Cells = vtkCellArray::New();

  // Initialize a Locator
  if (this->Merging)
  {
    this->Locator = vtkMergePoints::New();
    this->Locator->InitPointInsertion(this->Points, input->GetBounds());
  }

  // Retrieve material mask
  this->Mask = input->HasMask() ? input->GetMask() : nullptr;

  if (this->Dimension == 3)
  {
    vtkIdType index;
    vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
    input->InitializeTreeIterator(it);
    vtkNew<vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight> cursor;
    while (it.GetNextTree(index))
    {
      if (this->CheckAbort())
      {
        break;
      }
      // In 3 dimensions, von Neumann neighborhood information is needed
      input->InitializeNonOrientedVonNeumannSuperCursorLight(cursor, index);
      this->RecursivelyProcessTree3D(cursor, 0);
    }
  }
  else
  {
    vtkIdType index;
    vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
    input->InitializeTreeIterator(it);
    vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
    while (it.GetNextTree(index))
    {
      if (this->CheckAbort())
      {
        break;
      }
      // Otherwise, geometric properties of the cells suffice
      input->InitializeNonOrientedGeometryCursor(cursor, index);
      if (this->Dimension == 1)
      {
        this->RecursivelyProcessTree1D(cursor, 0);
      }
      else
      {
        this->RecursivelyProcessTree2D(cursor, 0);
      }
    }
  }

  // Set output geometry and topology
  output->SetPoints(this->Points);
  if (this->Dimension == 1)
  {
    output->SetLines(this->Cells);
  }
  else
  {
    output->SetPolys(this->Cells);
  }

  for (int i = 0; i < this->OutData->GetNumberOfArrays(); i++)
  {
    this->OutData->GetAbstractArray(i)->Resize(output->GetNumberOfCells());
  }

  this->Points->Delete();
  this->Points = nullptr;
  this->Cells->Delete();
  this->Cells = nullptr;

  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::RecursivelyProcessTree1D(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor, int level)
{
  double origin = cursor->GetOrigin()[this->Axis1];

  std::array<std::array<double, 3>, 2> corners = { { { { origin, 0.0, 0.0 } },
    { { origin + cursor->GetSize()[this->Axis1], 0.0, 0.0 } } } };

  // We only process the nodes than are going to be rendered
  if (level < this->MaxLevel &&
    this->IsShapeVisible<2>(corners, level) == ShapeState::OUT_OF_SCREEN)
  {
    return;
  }

  if (cursor->IsLeaf() || level >= this->MaxLevel ||
    (this->FixedLevelMax != -1 && level >= this->FixedLevelMax))
  {
    this->ProcessLeaf1D(cursor);
  }
  else
  {
    // Cursor is not at leaf, recurse to all children
    const int numChildren = cursor->GetNumberOfChildren();
    for (int iChild = 0; iChild < numChildren; ++iChild)
    {
      if (this->CheckAbort())
      {
        break;
      }
      cursor->ToChild(iChild);
      this->RecursivelyProcessTree1D(cursor, level + 1);
      cursor->ToParent();
    }
  }
}

//------------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::RecursivelyProcessTree2D(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor, int level)
{
  double originAxis1 = cursor->GetOrigin()[this->Axis1];
  double originAxis2 = cursor->GetOrigin()[this->Axis2];

  std::array<std::array<double, 3>, 4> corners = { { { { originAxis1, originAxis2, 0.0 } },
    { { originAxis1 + cursor->GetSize()[this->Axis1], originAxis2, 0.0 } },
    { { originAxis1, originAxis2 + cursor->GetSize()[this->Axis2], 0.0 } },
    { { originAxis1 + cursor->GetSize()[this->Axis1], originAxis2 + cursor->GetSize()[this->Axis2],
      0.0 } } } };

  // We only process the nodes than are going to be rendered
  if (level < this->MaxLevel &&
    this->IsShapeVisible<4>(corners, level) == ShapeState::OUT_OF_SCREEN)
  {
    return;
  }

  if (cursor->IsLeaf() || level >= this->MaxLevel ||
    (this->FixedLevelMax != -1 && level >= this->FixedLevelMax))
  {
    this->ProcessLeaf2D(cursor);
  }
  else
  {
    // Cursor is not at leaf, recurse to all children
    const int numChildren = cursor->GetNumberOfChildren();
    for (int iChild = 0; iChild < numChildren; ++iChild)
    {
      if (this->CheckAbort())
      {
        break;
      }
      cursor->ToChild(iChild);
      this->RecursivelyProcessTree2D(cursor, level + 1);
      cursor->ToParent();
    }
  }
}

//------------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::ProcessLeaf1D(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  vtkIdType globalId = cursor->GetGlobalNodeIndex();
  if (this->Mask && this->Mask->GetValue(globalId))
  {
    return;
  }
  // In 1D the geometry is composed of edges, create storage for endpoint IDs
  vtkIdType id[2];

  // First endpoint is at origin of cursor
  const double* origin = cursor->GetOrigin();
  id[0] = this->Points->InsertNextPoint(origin);

  // Second endpoint is at origin of cursor plus its length
  double pt[3];
  memcpy(pt, origin, 3 * sizeof(double));
  pt[this->Orientation] += cursor->GetSize()[this->Orientation];
  id[1] = this->Points->InsertNextPoint(pt);

  // Insert edge into 1D geometry
  vtkIdType outId = this->Cells->InsertNextCell(2, id);
  this->OutData->CopyData(this->InData, globalId, outId);
}

//------------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::ProcessLeaf2D(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  // Cell at cursor center is a leaf, retrieve its global index
  vtkIdType id = cursor->GetGlobalNodeIndex();
  if (id < 0)
  {
    return;
  }

  // In 2D all unmasked faces are generated
  if (!this->Mask || !this->Mask->GetValue(id))
  {
    // Insert face into 2D geometry depending on orientation
    this->AddFace(id, cursor->GetOrigin(), cursor->GetSize(), 0, this->Orientation);
  }
}

//------------------------------------------------------------------------------
template <int N>
vtkAdaptiveDataSetSurfaceFilter::ShapeState vtkAdaptiveDataSetSurfaceFilter::IsShapeVisible(
  const std::array<std::array<double, 3>, N>& points, int level)
{
  if (!this->ViewPointDepend)
  {
    return ShapeState::VISIBLE;
  }
  double minX = VTK_DOUBLE_MAX;
  double minY = VTK_DOUBLE_MAX;
  double maxX = VTK_DOUBLE_MIN;
  double maxY = VTK_DOUBLE_MIN;
  double minZ = VTK_DOUBLE_MAX;
  double maxZ = VTK_DOUBLE_MIN;

  for (int i = 0; i < N; ++i)
  {
    std::array<double, 3> point = points[i];
    double pointWorld[4] = { point[0], point[1], point[2], 1.0 };
    double* pointCam = this->ModelViewMatrix->MultiplyDoublePoint(pointWorld);
    double* pointClip = this->ProjectionMatrix->MultiplyDoublePoint(pointCam);

    double x = pointClip[0];
    double y = pointClip[1];
    double z = pointClip[2];
    double w = pointClip[3];

    if (!this->IsParallel && w != 0.0)
    {
      x /= w;
      y /= w;
      z /= w;
    }

    minX = std::min(minX, x);
    minY = std::min(minY, y);
    minZ = std::min(minZ, z);
    maxX = std::max(maxX, x);
    maxY = std::max(maxY, y);
    maxZ = std::max(maxZ, z);
  }

  double minXScreen = (minX + 1) / 2 * this->LastRendererSize[0];
  double maxXScreen = (maxX + 1) / 2 * this->LastRendererSize[0];
  double minYScreen = (1 - minY) / 2 * this->LastRendererSize[1];
  double maxYScreen = (1 - maxY) / 2 * this->LastRendererSize[1];

  // Cell is smaller than one pixel, return true to process this cell but set MaxLevel so that we
  // don't compute this for other cells >= MaxLevel.
  if (maxXScreen - minXScreen < 1.0 && maxYScreen - minYScreen < 1.0)
  {
    // Only used for 2D and 1D.
    this->MaxLevel = level;

    return ShapeState::SUB_PIXEL;
  }

  if (maxX >= -1 && minX <= 1 && maxY >= -1 && minY <= 1 && maxZ >= -1 && minZ <= 1)
  {
    return ShapeState::VISIBLE;
  }
  return ShapeState::OUT_OF_SCREEN;
}

//------------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::RecursivelyProcessTree3D(
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* cursor, int level)
{
  double* origin = cursor->GetOrigin();

  std::array<std::array<double, 3>, 8> corners = { {
    { { origin[0], origin[1], origin[2] } },
    { { origin[0], origin[1], origin[2] + cursor->GetSize()[2] } },
    { { origin[0] + cursor->GetSize()[0], origin[1], origin[2] } },
    { { origin[0] + cursor->GetSize()[0], origin[1], origin[2] + cursor->GetSize()[2] } },
    { { origin[0], origin[1] + cursor->GetSize()[1], origin[2] } },
    { { origin[0], origin[1] + cursor->GetSize()[1], origin[2] + cursor->GetSize()[2] } },
    { { origin[0] + cursor->GetSize()[0], origin[1] + cursor->GetSize()[1], origin[2] } },
    { { origin[0] + cursor->GetSize()[0], origin[1] + cursor->GetSize()[1],
      origin[2] + cursor->GetSize()[2] } },
  } };

  ShapeState shapeState = this->IsShapeVisible<8>(corners, level);
  if (shapeState == ShapeState::OUT_OF_SCREEN)
  {
    return;
  }

  // Create geometry output if cursor is at leaf
  if (cursor->IsLeaf() || shapeState == ShapeState::SUB_PIXEL ||
    (this->Mask && this->Mask->GetValue(cursor->GetGlobalNodeIndex())) ||
    (this->FixedLevelMax != -1 && level >= this->FixedLevelMax))
  {
    this->ProcessLeaf3D(cursor);
  }
  else
  {
    // Cursor is not at leaf, recurse to all children
    int numChildren = cursor->GetNumberOfChildren();
    for (int iChild = 0; iChild < numChildren; ++iChild)
    {
      if (this->CheckAbort())
      {
        break;
      }
      cursor->ToChild(iChild);
      this->RecursivelyProcessTree3D(cursor, level + 1);
      cursor->ToParent();
    }
  }
}

//------------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::ProcessLeaf3D(
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* superCursor)
{
  // Cell at super cursor center is a leaf, retrieve its global index, level, and mask
  vtkIdType idCenter = superCursor->GetGlobalNodeIndex();
  unsigned level = superCursor->GetLevel();
  int masked = this->Mask ? this->Mask->GetValue(idCenter) : 0;

  // Iterate over all cursors of Von Neumann neighborhood around center
  unsigned int nc = superCursor->GetNumberOfCursors() - 1;
  for (unsigned int c = 0; c < nc; ++c)
  {
    if (this->CheckAbort())
    {
      break;
    }
    // Retrieve cursor to neighbor across face
    // Retrieve tree, leaf flag, and mask of neighbor cursor
    unsigned int levelN;
    bool leafN;
    vtkIdType idN;
    vtkHyperTree* treeN = superCursor->GetInformation(VonNeumannCursors3D[c], levelN, leafN, idN);

    int maskedN = 0;
    if (treeN)
    {
      maskedN = this->Mask ? this->Mask->GetValue(idN) : 0;
    }

    // In 3D masked and unmasked cells are handled differently:
    // - If cell is unmasked, and face neighbor is masked, or no such neighbor
    //   exists, then generate face.
    // - If cell is masked, and face neighbor exists and is an unmasked leaf, then
    //   generate face, breaking ties at same level. This ensures that faces between
    //   unmasked and masked cells will be generated once and only once.
    if ((!masked && (!treeN || maskedN)) ||
      (masked && treeN && leafN && levelN < level && !maskedN))
    {
      // Generate face with corresponding normal and offset
      this->AddFace(idCenter, superCursor->GetOrigin(), superCursor->GetSize(),
        VonNeumannOffsets3D[c], VonNeumannOrientations3D[c]);
    }
  }
}

//------------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::AddFace(
  vtkIdType inId, const double* origin, const double* size, int offset, unsigned int orientation)
{
  // Storage for point coordinates
  double pt[] = { 0., 0., 0. };

  // Storage for face vertex IDs
  vtkIdType ids[4];

  // First cell vertex is always at origin of cursor
  memcpy(pt, origin, 3 * sizeof(double));

  if (this->Locator)
  {
    if (offset)
    {
      // Offset point coordinate as needed
      pt[orientation] += size[orientation];
    }
    this->Locator->InsertUniquePoint(pt, ids[0]);
    // Create other face vertices depending on orientation
    unsigned int axis1 = orientation ? 0 : 1;
    unsigned int axis2 = orientation == 2 ? 1 : 2;
    pt[axis1] += size[axis1];
    this->Locator->InsertUniquePoint(pt, ids[1]);
    pt[axis2] += size[axis2];
    this->Locator->InsertUniquePoint(pt, ids[2]);
    pt[axis1] = origin[axis1];
    this->Locator->InsertUniquePoint(pt, ids[3]);
  }
  else
  {
    if (offset)
    {
      // Offset point coordinate as needed
      pt[orientation] += size[orientation];
    }
    ids[0] = this->Points->InsertNextPoint(pt);

    // Create other face vertices depending on orientation
    unsigned int axis1 = orientation ? 0 : 1;
    unsigned int axis2 = orientation == 2 ? 1 : 2;
    pt[axis1] += size[axis1];
    ids[1] = this->Points->InsertNextPoint(pt);
    pt[axis2] += size[axis2];
    ids[2] = this->Points->InsertNextPoint(pt);
    pt[axis1] = origin[axis1];
    ids[3] = this->Points->InsertNextPoint(pt);
  }

  // Insert next face
  vtkIdType outId = this->Cells->InsertNextCell(4, ids);

  // Copy face data from that of the cell from which it comes
  this->OutData->CopyData(this->InData, inId, outId);
}

//------------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::SetRenderer(vtkRenderer* ren)
{
  if (ren != this->Renderer)
  {
    this->Renderer = ren;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkAdaptiveDataSetSurfaceFilter::GetMTime()
{
  // Check for minimal changes
  if (this->Renderer)
  {
    vtkCamera* cam = this->Renderer->GetActiveCamera();
    if (cam)
    {
      // Check & Update renderer size
      const int* sz = this->Renderer->GetSize();
      if (this->LastRendererSize[0] != sz[0] || this->LastRendererSize[1] != sz[1])
      {
        this->LastRendererSize[0] = sz[0];
        this->LastRendererSize[1] = sz[1];
        this->Modified();
      }
    } // if ( cam )
  }   // if ( this->Renderer )
  return this->Superclass::GetMTime();
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_5_0
void vtkAdaptiveDataSetSurfaceFilter::SetCircleSelection(bool vtkNotUsed(_arg))
{
  vtkWarningMacro("CircleSelection has been removed. Do not use.");
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_5_0
bool vtkAdaptiveDataSetSurfaceFilter::GetCircleSelection()
{
  vtkWarningMacro("CircleSelection has been removed. Do not use.");
  return true;
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_5_0
void vtkAdaptiveDataSetSurfaceFilter::SetBBSelection(bool vtkNotUsed(_arg))
{
  vtkWarningMacro("BBSelection has been removed. Do not use.");
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_5_0
bool vtkAdaptiveDataSetSurfaceFilter::GetBBSelection()
{
  vtkWarningMacro("BBSelection has been removed. Do not use.");
  return true;
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_5_0
void vtkAdaptiveDataSetSurfaceFilter::SetDynamicDecimateLevelMax(int vtkNotUsed(_arg))
{
  vtkWarningMacro("DynamicDecimateLevelMax has been removed. Do not use.");
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_5_0
int vtkAdaptiveDataSetSurfaceFilter::GetDynamicDecimateLevelMax()
{
  vtkWarningMacro("DynamicDecimateLevelMax has been removed. Do not use.");
  return 0;
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_5_0
void vtkAdaptiveDataSetSurfaceFilter::SetScale(double vtkNotUsed(_arg))
{
  vtkWarningMacro("Scale has been removed. Do not use.");
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_5_0
int vtkAdaptiveDataSetSurfaceFilter::GetScale()
{
  vtkWarningMacro("Scale has been removed. Do not use.");
  return 0;
}

VTK_ABI_NAMESPACE_END
