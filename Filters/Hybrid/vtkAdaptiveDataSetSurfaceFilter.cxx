/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdaptiveDataSetSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAdaptiveDataSetSurfaceFilter.h"

#include "vtkBitArray.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkUnsignedCharArray.h"

#include "vtkIncrementalPointLocator.h"
#include "vtkMergePoints.h"

#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight.h"

static const unsigned int VonNeumannCursors3D[] = { 0, 1, 2, 4, 5, 6 };
static const unsigned int VonNeumannOrientations3D[] = { 2, 1, 0, 0, 1, 2 };
static const unsigned int VonNeumannOffsets3D[] = { 0, 0, 0, 1, 1, 1 };

vtkStandardNewMacro(vtkAdaptiveDataSetSurfaceFilter);

//-----------------------------------------------------------------------------
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
}

//-----------------------------------------------------------------------------
vtkAdaptiveDataSetSurfaceFilter::~vtkAdaptiveDataSetSurfaceFilter() {}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkAdaptiveDataSetSurfaceFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  vtkDataObject* input = vtkDataObject::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int MeshType = input->GetDataObjectType();
  if (MeshType != VTK_HYPER_TREE_GRID)
  {
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  return this->DataSetExecute(input, output);
}

//----------------------------------------------------------------------------
int vtkAdaptiveDataSetSurfaceFilter::DataSetExecute(vtkDataObject* inputDS, vtkPolyData* output)
{
  // Retrieve input grid
  vtkHyperTreeGrid* input = vtkHyperTreeGrid::SafeDownCast(inputDS);
  if (!input)
  {
    // DDM&&JB Nous perdons cette facilite d'appeler ce service par defaut qui nous
    // redirige ensuite vers un service plus adapte si pas HTG
    // return vtkDataSetSurfaceFilter::DataSetExecute( inputDS, output );
    vtkErrorMacro("pre: input_not_HyperTreeGrid: " << inputDS->GetClassName());
    return 0;
  }

  // Retrieve useful grid parameters for speed of access
  this->Dimension = input->GetDimension();
  this->Orientation = input->GetOrientation();

  // Initialize output cell data
  this->InData = static_cast<vtkDataSetAttributes*>(input->GetPointData());
  this->OutData = static_cast<vtkDataSetAttributes*>(output->GetCellData());
  this->OutData->CopyAllocate(this->InData);

  // DDM&&JB Nous perdons aussi cette fonctionnalite sous cette nouvelle forme
  /*
  if ( this->PassThroughCellIds )
  {
    this->OriginalCellIds = vtkIdTypeArray::New();
    this->OriginalCellIds->SetName( this->GetOriginalCellIdsName() );
    this->OriginalCellIds->SetNumberOfComponents( 1 );
    this->OutData->AddArray( this->OriginalCellIds );
  }
  */

  // Init renderer information
  if (this->ViewPointDepend && this->ParallelProjection && this->Renderer)
  {
    // Generate planes XY, XZ o YZ
    unsigned int gridSize[3];
    input->GetCellDims(gridSize);

    bool isInit = false;
    if (this->Dimension == 2) // JB A verifier
    {
      input->Get2DAxes(this->Axis1, this->Axis2);
      isInit = true;
    }

    // Compute the zoom of the camera
    vtkCamera* cam = this->Renderer->GetActiveCamera();

    // Compute the bounding box
    double bounds[6];
    input->GetBounds(bounds);

    // JB Recupere le branch factor
    int f = input->GetBranchFactor();

    // JB Le calcul qui suit a pour objet de determiner le niveau de parcours en profondeur utile
    // pour l'affichage
    // JB en fonction de la distance
    if (isInit)
    {
      // JB Taille Moyenne d'une maille du niveau 0 dans les coordonnees reelles suivant chaque
      // direction
      double worldCellAverageScaleAxis1 = (bounds[2 * this->Axis1 + 1] - bounds[2 * this->Axis1]) /
        (double)(gridSize[this->Axis1]) / this->Scale;
      double worldCellAverageScaleAxis2 = (bounds[2 * this->Axis2 + 1] - bounds[2 * this->Axis2]) /
        (double)(gridSize[this->Axis2]) / this->Scale;

      // JB Taille de la fenetre dans les coordonnees reelles (GetParallelScale) suivant chaque
      // direction
      double worldWindScaleAxis1 =
        cam->GetParallelScale() * this->LastRendererSize[0] / (double)(this->LastRendererSize[1]);
      double worldWindScaleAxis2 = cam->GetParallelScale();

      // JB Taille de la fenetre en pixel, ecran, suivant chaque direction
      double windScaleAxis1 = this->LastRendererSize[0];
      double windScaleAxis2 = this->LastRendererSize[1];

      // JB Compute how many levels of the tree we should process by direction
      // JB 1) application du theoreme de Thales ; le rapport taille d'une maille de niveau 0 par la
      // taille de la fenetre est identique si le
      // JB calcul se fait en coordonnees reelles ou en coordonnees ecran (pixel)
      // JB 2) la taille d'une maille de niveau L vaut la taille d'une maille de niveau 0 divisee
      // par le facteur de raffinement eleve a la puissance L
      // JB 3) au final, le calcul qui suit a pour objet de determiner quand une maille fera un
      // pixel
      double levelMaxiAxis1 =
        (log(windScaleAxis1) + log(worldCellAverageScaleAxis1) - log(worldWindScaleAxis1)) / log(f);
      double levelMaxiAxis2 =
        (log(windScaleAxis2) + log(worldCellAverageScaleAxis2) - log(worldWindScaleAxis2)) / log(f);

      // JB On opte pour le niveau le plus eleve
      this->LevelMax = std::ceil(std::max(levelMaxiAxis1, levelMaxiAxis2));
    }
    else
    {
      // JB En 3D, par defaut, on prend tous les niveaux
      this->LevelMax = 65536;
    }

    // JB Par option, on peut reduire cette valeur... tres utile pour avoir un LOD leger.
    this->LevelMax -= this->DynamicDecimateLevelMax;
    if (this->LevelMax < 0)
    {
      this->LevelMax = 0;
    }

    // JB Par option, on peut fixer le niveau max independemment du calcul dynamique realise
    // precedemment
    if (this->FixedLevelMax >= 0)
    {
      this->LevelMax = this->FixedLevelMax;
    }

    // JB Le calcul qui suit a pour objet de determiner le rayon du cercle dans les coordonnees
    // reelles incluant la projection
    // JB de la fenetre. L'activation de CircleSelection permettra de ne produire que les mailles
    // intersectant ce cercle centre
    // JB au camera focal point.
    // JB LastCameraFocalPoint retourne le centre de l'ecran dans les coordonnees reelles
    double ratio = this->LastRendererSize[0] / (double)(this->LastRendererSize[1]);
    this->Radius = cam->GetParallelScale() * sqrt(1 + pow(ratio, 2));

    // JB Le calcul qui suit a pour objet de determiner la boite englobante dans les coordonnees
    // reelles (et sans tenir compte
    // JB d'un point de vue qui aurait tourne) incluant la projection de la fenetre. L'activation de
    // BBSelection permettra de ne
    // JB produire que les mailles intersectant cette boite englobante.
    this->WindowBounds[0] = this->LastCameraFocalPoint[0] - cam->GetParallelScale() * ratio;
    this->WindowBounds[1] = this->LastCameraFocalPoint[0] + cam->GetParallelScale() * ratio;
    this->WindowBounds[2] = this->LastCameraFocalPoint[1] - cam->GetParallelScale();
    this->WindowBounds[3] = this->LastCameraFocalPoint[1] + cam->GetParallelScale();

#ifndef NDEBUG
    this->NbRejectByCircle = 0;
    this->NbRejectByBB = 0;

    std::cerr << "LevelMax        " << this->LevelMax << std::endl;
    std::cerr << "CircleSelection " << this->CircleSelection << std::endl;
    std::cerr << "Circle R        " << this->Radius << std::endl;
    std::cerr << "       CX       " << this->LastCameraFocalPoint[this->Axis1] << std::endl;
    std::cerr << "       CY       " << this->LastCameraFocalPoint[this->Axis2] << std::endl;
    std::cerr << "BBSelection     " << this->BBSelection << std::endl;
    std::cerr << "Bounds X        " << this->WindowBounds[0] << " : " << this->WindowBounds[1]
              << std::endl;
    std::cerr << "       Y        " << this->WindowBounds[2] << " : " << this->WindowBounds[3]
              << std::endl;
#endif
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

//----------------------------------------------------------------------------
int vtkAdaptiveDataSetSurfaceFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::ProcessTrees(vtkHyperTreeGrid* input, vtkPolyData* output)
{
  if (this->Points)
  {
    this->Points->Delete();
  }
  // Create storage for corners of leaf cells
  this->Points = vtkPoints::New();

  // Create storage for untructured leaf cells
  if (this->Cells)
  {
    this->Cells->Delete();
  }
  this->Cells = vtkCellArray::New();

  // JB Initialize a Locator
  if (this->Merging)
  {
    this->Locator = vtkMergePoints::New();
    this->Locator->InitPointInsertion(this->Points, input->GetBounds());
  }

  // Retrieve material mask
  this->Mask = input->HasMask() ? input->GetMask() : nullptr;

  //
  vtkUnsignedCharArray* ghost = nullptr; // DDM input->GetPointGhostArray();
  if (ghost)
  {
    this->OutData->CopyFieldOff(vtkDataSetAttributes::GhostArrayName());
  }

  // Iterate over all hyper trees
  if (this->Dimension == 3)
  {
    vtkIdType index;
    vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
    input->InitializeTreeIterator(it);
    vtkNew<vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight> cursor;
    while (it.GetNextTree(index))
    {
      // In 3 dimensions, von Neumann neighborhood information is needed
      input->InitializeNonOrientedVonNeumannSuperCursorLight(cursor, index);
      // If this is not a ghost tree
      if (!ghost || !ghost->GetTuple1(cursor->GetGlobalNodeIndex()))
      {
        // Build geometry recursively
        this->RecursivelyProcessTree3D(cursor, 0);
      }
    } // it
  }   // if ( this->Dimension == 3 )
  else
  {
    vtkIdType index;
    vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
    input->InitializeTreeIterator(it);
    vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
    while (it.GetNextTree(index))
    {
      // Otherwise, geometric properties of the cells suffice
      input->InitializeNonOrientedGeometryCursor(cursor, index);
      // If this is not a ghost tree
      if (!ghost || !ghost->GetTuple1(cursor->GetGlobalNodeIndex()))
      {
        // Build geometry recursively
        this->RecursivelyProcessTreeNot3D(cursor, 0);
      }
    } // it
  }   // else

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

#ifndef NDEBUG
  std::cerr << "vtkAdaptiveDataSetSurfaceFilter #Points            "
            << this->Points->GetNumberOfPoints() << std::endl;
  std::cerr << "                                #Cells             "
            << this->Cells->GetNumberOfCells() << std::endl;
  std::cerr << "                                #Type&Connectivity "
            << this->Cells->GetNumberOfConnectivityIds() << std::endl;
  std::cerr << "                          Cells #NbRejectByBB      " << this->NbRejectByBB
            << std::endl;
  std::cerr << "                                #NbRejectByCircle  " << this->NbRejectByCircle
            << std::endl;
#endif
  std::cerr << "vtkAdaptiveDataSetSurfaceFilter #Points            "
            << this->Points->GetNumberOfPoints() << std::endl;
  std::cerr << "                                #Cells             "
            << this->Cells->GetNumberOfCells() << std::endl;
  std::cerr << "                                #Type&Connectivity "
            << this->Cells->GetNumberOfConnectivityIds() << std::endl;

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

//----------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::RecursivelyProcessTreeNot3D(
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
      // JB On determine si la maille correspondant au current node of the tree
      // JB is going to be rendered.
      // JB Pour cela, on fait une premiere approximation en considerant la maille
      // JB carre qui l'englobe en conservant la meme origine et en fixant sa demi-largeur
      // JB a la valeur maximale entre les valeurs de demi-largeur et demi-longueur.
      double half = std::max(halfAxis1, halfAxis2);
      // JB This cell must be rendered si le centre de cette maille se trouve a moins de
      // JB Radius + half * sqrt(2) du camera focal point. Radius est le rayon minimal du cercle
      // JB centre sur la camera focal point couvrant la fenetre de rendu.
      // JB Le centre de la maille se trouve a Origin + half, par direction.
      // JB La comparaison se fait sur les distances au carre afin d'eviter le calcul
      // JB couteux de racines carres.
      insideBB = (pow(originAxis1 + half - this->LastCameraFocalPoint[this->Axis1], 2) +
                   pow(originAxis2 + half - this->LastCameraFocalPoint[this->Axis2], 2)) <
        // pow( this->Radius + half * sqrt(2.), 2 );
        pow(this->Radius + half * 1.414213562, 2);
    }
    else
    {
      insideBB = true;
    }

    if (insideBB && this->BBSelection)
    {
      // JB On determine si la maille correspondant au current node of the tree
      // JB is going to be rendered.
      // JB Pour cela, on verifie si la maille est dans une boite englobante correspondant a la
      // JB projection de l'ecran dans le monde du maillage.
      insideBB = ((originAxis1 + 2 * halfAxis1 >= this->WindowBounds[0]) &&
        (originAxis1 <= this->WindowBounds[1]) &&
        (originAxis2 + 2 * halfAxis2 >= this->WindowBounds[2]) &&
        (originAxis2 <= this->WindowBounds[3]));
#ifndef NDEBUG
      if (!insideBB)
      {
        this->NbRejectByBB++;
      }
    }
    else
    {
      this->NbRejectByCircle++;
#endif
    }
  }
  if (insideBB)
  {
    // We only process those nodes than are going to be rendered
    if (cursor->IsLeaf() || (this->LevelMax != -1 && level >= this->LevelMax))
    {
      if (this->Dimension == 2)
      {
        this->ProcessLeaf2D(cursor);
      }
      else
      {
        this->ProcessLeaf1D(cursor);
      } // else
    }   // if ( cursor->IsLeaf() || ( this->LevelMax!=-1 && level >= this->LevelMax ) )
    else
    {
      // Cursor is not at leaf, recurse to all children
      int numChildren = cursor->GetNumberOfChildren();
      for (int ichild = 0; ichild < numChildren; ++ichild)
      {
        cursor->ToChild(ichild);
        // Recurse
        this->RecursivelyProcessTreeNot3D(cursor, level + 1);
        cursor->ToParent();
      } // ichild
    }   // else
  }     // if( insideBB )
}

//----------------------------------------------------------------------------
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
  } // switch
  id[1] = this->Points->InsertNextPoint(pt);

  // Insert edge into 1D geometry
  this->Cells->InsertNextCell(2, id);
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::RecursivelyProcessTree3D(
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* cursor, int level)
{
  // Create geometry output if cursor is at leaf
  if (cursor->IsLeaf())
  {
    this->ProcessLeaf3D(cursor);
  } // if ( cursor->IsLeaf() )
  else
  {
    // Cursor is not at leaf, recurse to all children
    int numChildren = cursor->GetNumberOfChildren();
    for (int ichild = 0; ichild < numChildren; ++ichild)
    {
      cursor->ToChild(ichild);
      // Recurse
      this->RecursivelyProcessTree3D(cursor, level + 1);
      cursor->ToParent();
    } // child
  }   // else
}

//----------------------------------------------------------------------------
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
    // . If cell is unmasked, and face neighbor is a masked leaf, or no such neighbor
    //   exists, then generate face.
    // . If cell is masked, and face neighbor exists and is an unmasked leaf, then
    //   generate face, breaking ties at same level. This ensures that faces between
    //   unmasked and masked cells will be generated once and only once.
    if ((!masked && (!treeN || (leafN && maskedN))) ||
      (masked && treeN && leafN && levelN < level && !maskedN))
    {
      // Generate face with corresponding normal and offset
      this->AddFace(idcenter, superCursor->GetOrigin(), superCursor->GetSize(),
        VonNeumannOffsets3D[c], VonNeumannOrientations3D[c]);
    }
  } // c
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::SetRenderer(vtkRenderer* ren)
{
  if (ren != this->Renderer)
  {
    this->Renderer = ren;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
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
