// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAdaptiveDataSetSurfaceFilter.h"

#include "vtkBitArray.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"

#include "vtkIncrementalPointLocator.h"
#include "vtkMergePoints.h"

#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight.h"

VTK_ABI_NAMESPACE_BEGIN
static const unsigned int VonNeumannCursors3D[] = { 0, 1, 2, 4, 5, 6 };
static const unsigned int VonNeumannOrientations3D[] = { 2, 1, 0, 0, 1, 2 };
static const unsigned int VonNeumannOffsets3D[] = { 0, 0, 0, 1, 1, 1 };

vtkStandardNewMacro(vtkAdaptiveDataSetSurfaceFilter);

//------------------------------------------------------------------------------
vtkAdaptiveDataSetSurfaceFilter::vtkAdaptiveDataSetSurfaceFilter()
{
  this->InData = nullptr;
  this->OutData = nullptr;
  this->Points = nullptr;
  this->Cells = nullptr;

  // Default dimension is 0
  this->Dimension = 0;

  // Default orientation is 0
  this->Orientation = 0;

  this->Renderer = nullptr;

  this->LevelMax = -1;

  this->ViewPointDepend = true;

  this->ParallelProjection = false;
  this->LastRendererSize[0] = 0;
  this->LastRendererSize[1] = 0;
  this->LastCameraFocalPoint[0] = 0.0;
  this->LastCameraFocalPoint[1] = 0.0;
  this->LastCameraFocalPoint[2] = 0.0;
  this->LastCameraParallelScale = 0;

  this->Scale = 1;

  this->CircleSelection = true;
  this->BBSelection = false;
  this->FixedLevelMax = -1;
  this->DynamicDecimateLevelMax = 0;

  // Default Locator is 0
  this->Merging = false;

  // vtkGeometryFilter allows an optional 2nd input. Need to
  // disable this.
  this->vtkAlgorithm::SetNumberOfInputPorts(1);
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

  os << indent << "Dimension: " << this->Dimension << endl;
  os << indent << "Orientation: " << this->Orientation << endl;
  os << indent << "Axis1: " << this->Axis1 << endl;
  os << indent << "Axis2: " << this->Axis2 << endl;
  os << indent << "Radius: " << this->Radius << endl;
  os << indent << "LevelMax: " << this->LevelMax << endl;
  os << indent << "ViewPointDepend: " << this->ViewPointDepend << endl;
  os << indent << "ParallelProjection: " << this->ParallelProjection << endl;
  os << indent << "Scale: " << this->Scale << endl;
  os << indent << "FixedLevelMax: " << this->FixedLevelMax << endl;
  os << indent << "DynamicDecimateLevelMax: " << this->DynamicDecimateLevelMax << endl;
  os << indent << "LastCameraParallelScale: " << this->LastCameraParallelScale << endl;
  os << indent << "LastRendererSize: " << this->LastRendererSize[0] << ", "
     << this->LastRendererSize[1] << endl;
  os << indent << "LastCameraFocalPoint: " << this->LastCameraFocalPoint[0] << ", "
     << this->LastCameraFocalPoint[1] << ", " << this->LastCameraFocalPoint[2] << endl;
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
int vtkAdaptiveDataSetSurfaceFilter::ComputeMaxLevel(vtkHyperTreeGrid* input)
{
  int levelMax;

  vtkCamera* cam = this->Renderer->GetActiveCamera();

  unsigned int gridSize[3];
  input->GetCellDims(gridSize);

  double bounds[6];
  input->GetBounds(bounds);

  int f = input->GetBranchFactor();

  // Compute the depth-first-search depth for display
  if (this->Dimension == 2)
  {
    // Average size of a level 0 cell in real coordinates along each direction.
    double worldCellAverageScaleAxis1 = (bounds[(2 * this->Axis1) + 1] - bounds[2 * this->Axis1]) /
      static_cast<double>(gridSize[this->Axis1]) / this->Scale;
    double worldCellAverageScaleAxis2 = (bounds[(2 * this->Axis2) + 1] - bounds[2 * this->Axis2]) /
      static_cast<double>(gridSize[this->Axis2]) / this->Scale;

    // Window size in real coordinates (GetParallelScale) along each direction
    double worldWindScaleAxis1 = cam->GetParallelScale() * this->LastRendererSize[0] /
      static_cast<double>(this->LastRendererSize[1]);
    double worldWindScaleAxis2 = cam->GetParallelScale();

    // Window size in screen pixels along each direction
    double windScaleAxis1 = this->LastRendererSize[0];
    double windScaleAxis2 = this->LastRendererSize[1];

    // Compute how many levels of the tree we should process by direction
    // 1) Application of Thales' theorem; the ratio of the size of a level 0 cell
    // to the window size is identical whether the calculation is done in real
    // coordinates or in screen (pixel) coordinates.
    // 2) The size of a level L cell is equal to the size of a level 0 cell
    // divided by the refinement factor raised to the power of L.
    // 3) Ultimately, the following calculation aims to determine when a cell
    // will correspond to a single pixel.

    double levelMaxiAxis1 =
      (log(windScaleAxis1) + log(worldCellAverageScaleAxis1) - log(worldWindScaleAxis1)) / log(f);
    double levelMaxiAxis2 =
      (log(windScaleAxis2) + log(worldCellAverageScaleAxis2) - log(worldWindScaleAxis2)) / log(f);

    // Select highest level
    levelMax = std::ceil(std::max(levelMaxiAxis1, levelMaxiAxis2));
  }
  else
  {
    // In 3D, by default, select all levels.
    levelMax = 65536;
  }

  // The selected max level can be reduced. It can be useful for different LODs
  levelMax -= this->DynamicDecimateLevelMax;
  levelMax = std::max(levelMax, 0);

  return levelMax;
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

  // Retrieve useful grid parameters for speed of access
  this->Dimension = input->GetDimension();
  this->Orientation = input->GetOrientation();

  // Initialize output cell data
  this->InData = static_cast<vtkDataSetAttributes*>(input->GetCellData());
  this->OutData = static_cast<vtkDataSetAttributes*>(output->GetCellData());
  this->OutData->CopyAllocate(this->InData);

  // Init renderer information
  if (this->ViewPointDepend && this->ParallelProjection && this->Renderer)
  {
    if (this->Dimension == 2)
    {
      input->Get2DAxes(this->Axis1, this->Axis2);
    }

    // The selected max level can be forced.
    if (this->FixedLevelMax >= 0)
    {
      this->LevelMax = this->FixedLevelMax;
    }
    else
    {
      this->LevelMax = this->ComputeMaxLevel(input);
    }

    vtkCamera* cam = this->Renderer->GetActiveCamera();

    // The following calculation aims to determine the radius of the circle in real
    // coordinates, including the projection of the window. Activating CircleSelection
    // will ensure that only the cells intersecting this circle, centered at the
    // camera focal point, are produced.
    // LastCameraFocalPoint returns the center of the screen in real coordinates.
    double ratio = this->LastRendererSize[0] / static_cast<double>(this->LastRendererSize[1]);
    this->Radius = cam->GetParallelScale() * sqrt(1 + (ratio * ratio));

    // The following calculation aims to determine the bounding box in real
    // coordinates (without considering a rotated viewpoint), including the projection
    // of the window. Activating BBSelection will ensure that only the cells
    // intersecting this bounding box are produced.
    this->WindowBounds[0] = this->LastCameraFocalPoint[0] - cam->GetParallelScale() * ratio;
    this->WindowBounds[1] = this->LastCameraFocalPoint[0] + cam->GetParallelScale() * ratio;
    this->WindowBounds[2] = this->LastCameraFocalPoint[1] - cam->GetParallelScale();
    this->WindowBounds[3] = this->LastCameraFocalPoint[1] + cam->GetParallelScale();
  }
  else
  {
    // Recurse all the tree
    this->LevelMax = -1;
  }

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

  // Iterate over all hyper trees
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
      this->RecursivelyProcessTree1DAnd2D(cursor, 0);
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
void vtkAdaptiveDataSetSurfaceFilter::RecursivelyProcessTree1DAnd2D(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor, int level)
{
  bool insideBB = (this->LevelMax == -1);
  if (!insideBB && (this->CircleSelection || this->BBSelection))
  {
    double originAxis1 = cursor->GetOrigin()[this->Axis1];
    double originAxis2 = cursor->GetOrigin()[this->Axis2];
    double halfAxis1 = cursor->GetSize()[this->Axis1] / 2;
    double halfAxis2 = cursor->GetSize()[this->Axis2] / 2;
    if (this->CircleSelection)
    {
      // We determine if the cell corresponding to the current node of the tree
      // is going to be rendered.
      // To do this, we make an initial approximation by considering the square cell
      // that encloses it, keeping the same origin and setting its half-width
      // to the maximum value between the half-width and half-length.
      double half = std::max(halfAxis1, halfAxis2);

      // This cell must be rendered if the center of this cell is within a distance of
      // Radius + half * sqrt(2) from the camera focal point. Radius is the minimal radius
      // of the circle centered at the camera focal point that covers the rendering window.
      // The center of the cell is located at Origin + half, in each direction.
      // The comparison is made on squared distances to avoid the costly calculation
      // of square roots.
      insideBB = pow(originAxis1 + half - this->LastCameraFocalPoint[this->Axis1], 2) +
          pow(originAxis2 + half - this->LastCameraFocalPoint[this->Axis2], 2) <
        pow(this->Radius + (half * 1.414213562), 2);
    }
    else if (this->BBSelection)
    {
      // We determine if the cell corresponding to the current node of the tree
      // is going to be rendered.
      // To do this, we check if the cell is within a bounding box corresponding to the
      // projection of the screen into the mesh world.
      insideBB = originAxis1 + 2 * halfAxis1 >= this->WindowBounds[0] &&
        originAxis1 <= this->WindowBounds[1] &&
        originAxis2 + 2 * halfAxis2 >= this->WindowBounds[2] &&
        originAxis2 <= this->WindowBounds[3];
    }
  }

  // We only process those nodes than are going to be rendered
  if (!insideBB)
  {
    return;
  }

  if (cursor->IsLeaf() || (this->LevelMax != -1 && level >= this->LevelMax))
  {
    if (this->Dimension == 2)
    {
      this->ProcessLeaf2D(cursor);
    }
    else
    {
      this->ProcessLeaf1D(cursor);
    }
  }
  else
  {
    // Cursor is not at leaf, recurse to all children
    int numChildren = cursor->GetNumberOfChildren();
    for (int ichild = 0; ichild < numChildren; ++ichild)
    {
      if (this->CheckAbort())
      {
        break;
      }
      cursor->ToChild(ichild);
      this->RecursivelyProcessTree1DAnd2D(cursor, level + 1);
      cursor->ToParent();
    }
  }
}

//------------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::ProcessLeaf1D(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  // In 1D the geometry is composed of edges, create storage for endpoint IDs
  vtkIdType id[2];

  // First endpoint is at origin of cursor
  const double* origin = cursor->GetOrigin();
  id[0] = this->Points->InsertNextPoint(origin);

  // Second endpoint is at origin of cursor plus its length
  double pt[3];
  memcpy(pt, origin, 3 * sizeof(double));
  switch (this->Orientation)
  {
    case 3: // 1 + 2
      pt[2] += cursor->GetSize()[2];
      break;
    case 5: // 1 + 4
      pt[1] += cursor->GetSize()[1];
      break;
    case 6: // 2 + 4
      pt[0] += cursor->GetSize()[0];
      break;
    default:
      break;
  }
  id[1] = this->Points->InsertNextPoint(pt);

  // Insert edge into 1D geometry
  this->Cells->InsertNextCell(2, id);
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
void vtkAdaptiveDataSetSurfaceFilter::RecursivelyProcessTree3D(
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* cursor, int level)
{
  // Create geometry output if cursor is at leaf
  if (cursor->IsLeaf())
  {
    this->ProcessLeaf3D(cursor);
  }
  else
  {
    // Cursor is not at leaf, recurse to all children
    int numChildren = cursor->GetNumberOfChildren();
    for (int ichild = 0; ichild < numChildren; ++ichild)
    {
      if (this->CheckAbort())
      {
        break;
      }
      cursor->ToChild(ichild);
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
  vtkIdType idcenter = superCursor->GetGlobalNodeIndex();
  unsigned level = superCursor->GetLevel();
  int masked = this->Mask ? this->Mask->GetValue(idcenter) : 0;

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

    int maskedN = 1;
    if (treeN)
    {
      maskedN = this->Mask ? this->Mask->GetValue(idN) : 0;
    }

    // In 3D masked and unmasked cells are handled differently:
    // - If cell is unmasked, and face neighbor is a masked leaf, or no such neighbor
    //   exists, then generate face.
    // - If cell is masked, and face neighbor exists and is an unmasked leaf, then
    //   generate face, breaking ties at same level. This ensures that faces between
    //   unmasked and masked cells will be generated once and only once.
    if ((!masked && (!treeN || (leafN && maskedN))) ||
      (masked && treeN && leafN && levelN < level && !maskedN))
    {
      // Generate face with corresponding normal and offset
      this->AddFace(idcenter, superCursor->GetOrigin(), superCursor->GetSize(),
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
      // Check & Update parallel projection
      bool para = (cam->GetParallelProjection() != 0);

      if (this->ParallelProjection != para)
      {
        this->ParallelProjection = para;
        this->Modified();
      }

      // Check & Update renderer size
      const int* sz = this->Renderer->GetSize();
      if (this->LastRendererSize[0] != sz[0] || this->LastRendererSize[1] != sz[1])
      {
        this->LastRendererSize[0] = sz[0];
        this->LastRendererSize[1] = sz[1];
        this->Modified();
      }

      // Check & Update camera focal point
      double* fp = cam->GetFocalPoint();
      if (this->LastCameraFocalPoint[0] != fp[0] || this->LastCameraFocalPoint[1] != fp[1] ||
        this->LastCameraFocalPoint[2] != fp[2])
      {
        this->LastCameraFocalPoint[0] = fp[0];
        this->LastCameraFocalPoint[1] = fp[1];
        this->LastCameraFocalPoint[2] = fp[2];
        this->Modified();
      }

      // Check & Update camera scale
      double scale = cam->GetParallelScale();
      if (this->LastCameraParallelScale != scale)
      {
        this->LastCameraParallelScale = scale;
        this->Modified();
      }
    } // if ( cam )
  }   // if ( this->Renderer )
  return this->Superclass::GetMTime();
}
VTK_ABI_NAMESPACE_END
