// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 1993-2007 NVIDIA Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov
// .SECTION Thanks
// This file is part of the generalized Youngs material interface reconstruction algorithm
// contributed by CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br> BP12,
// F-91297 Arpajon, France. <br> Implementation by Thierry Carrard (CEA) and Philippe Pebay (Kitware
// SAS)

#include "vtkYoungsMaterialInterface.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkConvexPointSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkEmptyCell.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#ifndef DBG_ASSERT
#define DBG_ASSERT(c) (void)0
#endif

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <cassert>
#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
class vtkYoungsMaterialInterfaceCellCut
{
public:
  enum
  {
    MAX_CELL_POINTS = 128,
    MAX_CELL_TETRAS = 128
  };

  static void cellInterface3D(

    // Inputs
    int ncoords, double coords[][3], int nedge, int cellEdges[][2], int ntetra,
    int tetraPointIds[][4], double fraction, double normal[3], bool useFractionAsDistance,

    // Outputs
    int& np, int eids[], double weights[], int& nInside, int inPoints[], int& nOutside,
    int outPoints[]);

  static double findTetraSetCuttingPlane(const double normal[3], double fraction, int vertexCount,
    const double vertices[][3], int tetraCount, const int tetras[][4]);

  static bool cellInterfaceD(

    // Inputs
    double points[][3], int nPoints, int triangles[][3], int nTriangles, double fraction,
    double normal[3], bool axisSymetric, bool useFractionAsDistance,

    // Outputs
    int eids[4], double weights[2], int& polygonPoints, int polygonIds[], int& nRemPoints,
    int remPoints[]);

  static double findTriangleSetCuttingPlane(const double normal[3], double fraction,
    int vertexCount, const double vertices[][3], int triangleCount, const int triangles[][3],
    bool axisSymetric = false);
};

class vtkYoungsMaterialInterfaceInternals
{
public:
  struct MaterialDescription
  {
  private:
    std::string Volume, Normal, NormalX, NormalY, NormalZ, Ordering;

  public:
    std::set<int> blocks;
    void setVolume(const std::string& str) { this->Volume = str; }
    void setNormal(const std::string& str) { this->Normal = str; }
    void setNormalX(const std::string& str) { this->NormalX = str; }
    void setNormalY(const std::string& str) { this->NormalY = str; }
    void setNormalZ(const std::string& str) { this->NormalZ = str; }
    void setOrdering(const std::string& str) { this->Ordering = str; }

    const std::string& volume() const { return this->Volume; }

    const std::string& normal(const vtkYoungsMaterialInterfaceInternals& storage) const
    {
      if (this->Normal.empty() && this->NormalX.empty() && this->NormalY.empty() &&
        this->NormalZ.empty() &&
        storage.NormalArrayMap.find(this->Volume) != storage.NormalArrayMap.end())
      {
        return storage.NormalArrayMap.find(this->Volume)->second;
      }
      return this->Normal;
    }

    const std::string& ordering(const vtkYoungsMaterialInterfaceInternals& storage) const
    {
      if (this->Ordering.empty() &&
        storage.OrderingArrayMap.find(this->Volume) != storage.OrderingArrayMap.end())
      {
        return storage.OrderingArrayMap.find(this->Volume)->second;
      }
      return this->Ordering;
    }

    const std::string& normalX() const { return this->NormalX; }

    const std::string& normalY() const { return this->NormalY; }

    const std::string& normalZ() const { return this->NormalZ; }
  };

  std::vector<MaterialDescription> Materials;

  //  original implementation uses index to save all normal and ordering array
  //  associations. To make it easier for ParaView, we needed to add an API to
  //  associate normal and ordering arrays using the volume fraction array names
  //  and hence we've added these two maps. These are only used if
  //  MaterialDescription has empty values for normal and ordering.
  //  Eventually, we may want to consolidate these data-structures.
  std::map<std::string, std::string> NormalArrayMap;
  std::map<std::string, std::string> OrderingArrayMap;
};

// standard constructors and factory
vtkStandardNewMacro(vtkYoungsMaterialInterface);

/*!
  The default constructor
  \sa ~vtkYoungsMaterialInterface()
*/
vtkYoungsMaterialInterface::vtkYoungsMaterialInterface()
{
  this->FillMaterial = 0;
  this->InverseNormal = 0;
  this->AxisSymetric = 0;
  this->OnionPeel = 0;
  this->ReverseMaterialOrder = 0;
  this->UseFractionAsDistance = 0;
  this->VolumeFractionRange[0] = 0.01;
  this->VolumeFractionRange[1] = 0.99;
  this->NumberOfDomains = -1;
  this->Internals = new vtkYoungsMaterialInterfaceInternals;
  this->MaterialBlockMapping = vtkSmartPointer<vtkIntArray>::New();
  this->UseAllBlocks = true;

  vtkDebugMacro(<< "vtkYoungsMaterialInterface::vtkYoungsMaterialInterface() ok\n");
}

/*!
  The destructor
  \sa vtkYoungsMaterialInterface()
*/
vtkYoungsMaterialInterface::~vtkYoungsMaterialInterface()
{
  delete this->Internals;
}

void vtkYoungsMaterialInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FillMaterial: " << this->FillMaterial << "\n";
  os << indent << "InverseNormal: " << this->InverseNormal << "\n";
  os << indent << "AxisSymetric: " << this->AxisSymetric << "\n";
  os << indent << "OnionPeel: " << this->OnionPeel << "\n";
  os << indent << "ReverseMaterialOrder: " << this->ReverseMaterialOrder << "\n";
  os << indent << "UseFractionAsDistance: " << this->UseFractionAsDistance << "\n";
  os << indent << "VolumeFractionRange: [" << this->VolumeFractionRange[0] << ";"
     << this->VolumeFractionRange[1] << "]\n";
  os << indent << "NumberOfDomains" << this->NumberOfDomains << "\n";
  os << indent << "UseAllBlocks:" << this->UseAllBlocks << "\n";
}

void vtkYoungsMaterialInterface::SetNumberOfMaterials(int n)
{
  //   vtkDebugMacro(<<"Resize Materials to "<<n<<"\n");
  this->NumberOfDomains = -1;
  this->Internals->Materials.resize(n);
  this->Modified();
}

int vtkYoungsMaterialInterface::GetNumberOfMaterials()
{
  return static_cast<int>(this->Internals->Materials.size());
}

void vtkYoungsMaterialInterface::SetMaterialVolumeFractionArray(int M, const char* volume)
{
  vtkDebugMacro(<< "SetMaterialVolumeFractionArray " << M << " : " << volume << "\n");
  this->NumberOfDomains = -1;
  if (M < 0)
  {
    vtkErrorMacro(<< "Bad material index " << M << "\n");
    return;
  }
  else if (M >= this->GetNumberOfMaterials())
  {
    this->SetNumberOfMaterials(M + 1);
  }

  this->Internals->Materials[M].setVolume(volume);
  this->Modified();
}

void vtkYoungsMaterialInterface::SetMaterialNormalArray(int M, const char* normal)
{
  vtkDebugMacro(<< "SetMaterialNormalArray " << M << " : " << normal << "\n");
  this->NumberOfDomains = -1;
  if (M < 0)
  {
    vtkErrorMacro(<< "Bad material index " << M << "\n");
    return;
  }
  else if (M >= this->GetNumberOfMaterials())
  {
    this->SetNumberOfMaterials(M + 1);
  }

  std::string n = normal;
  std::string::size_type s = n.find(' ');
  if (s == std::string::npos)
  {
    this->Internals->Materials[M].setNormal(n);
    this->Internals->Materials[M].setNormalX("");
    this->Internals->Materials[M].setNormalY("");
    this->Internals->Materials[M].setNormalZ("");
  }
  else
  {
    std::string::size_type s2 = n.rfind(' ');
    this->Internals->Materials[M].setNormal("");
    this->Internals->Materials[M].setNormalX(n.substr(0, s));
    this->Internals->Materials[M].setNormalY(n.substr(s + 1, s2 - s - 1));
    this->Internals->Materials[M].setNormalZ(n.substr(s2 + 1));
  }
  this->Modified();
}

void vtkYoungsMaterialInterface::SetMaterialOrderingArray(int M, const char* ordering)
{
  vtkDebugMacro(<< "SetMaterialOrderingArray " << M << " : " << ordering << "\n");
  this->NumberOfDomains = -1;
  if (M < 0)
  {
    vtkErrorMacro(<< "Bad material index " << M << "\n");
    return;
  }
  else if (M >= this->GetNumberOfMaterials())
  {
    this->SetNumberOfMaterials(M + 1);
  }
  this->Internals->Materials[M].setOrdering(ordering);
  this->Modified();
}

void vtkYoungsMaterialInterface::SetMaterialArrays(
  int M, const char* volume, const char* normal, const char* ordering)
{
  this->NumberOfDomains = -1;
  if (M < 0)
  {
    vtkErrorMacro(<< "Bad material index " << M << "\n");
    return;
  }
  else if (M >= this->GetNumberOfMaterials())
  {
    this->SetNumberOfMaterials(M + 1);
  }
  vtkDebugMacro(<< "Set Material " << M << " : " << volume << "," << normal << "," << ordering
                << "\n");
  vtkYoungsMaterialInterfaceInternals::MaterialDescription md;
  md.setVolume(volume);
  md.setNormal(normal);
  md.setNormalX("");
  md.setNormalY("");
  md.setNormalZ("");
  md.setOrdering(ordering);
  this->Internals->Materials[M] = md;
  this->Modified();
}

void vtkYoungsMaterialInterface::SetMaterialArrays(int M, const char* volume, const char* normalX,
  const char* normalY, const char* normalZ, const char* ordering)
{
  this->NumberOfDomains = -1;
  if (M < 0)
  {
    vtkErrorMacro(<< "Bad material index " << M << "\n");
    return;
  }
  else if (M >= this->GetNumberOfMaterials())
  {
    this->SetNumberOfMaterials(M + 1);
  }
  vtkDebugMacro(<< "Set Material " << M << " : " << volume << "," << normalX << "," << normalY
                << "," << normalZ << "," << ordering << "\n");
  vtkYoungsMaterialInterfaceInternals::MaterialDescription md;
  md.setVolume(volume);
  md.setNormal("");
  md.setNormalX(normalX);
  md.setNormalY(normalY);
  md.setNormalZ(normalZ);
  md.setOrdering(ordering);
  this->Internals->Materials[M] = md;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkYoungsMaterialInterface::SetMaterialNormalArray(const char* volume, const char* normal)
{
  // not sure why this is done, but all SetMaterialNormalArray(int,..) variants
  // do it, and hence ...
  this->NumberOfDomains = -1;
  if (volume && normal && this->Internals->NormalArrayMap[volume] != normal)
  {
    this->Internals->NormalArrayMap[volume] = normal;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkYoungsMaterialInterface::SetMaterialOrderingArray(const char* volume, const char* ordering)
{
  // not sure why this is done, but all SetMaterialOrderingArray(int,..) variants
  // do it, and hence ...
  this->NumberOfDomains = -1;
  if (volume && ordering && this->Internals->OrderingArrayMap[volume] != ordering)
  {
    this->Internals->OrderingArrayMap[volume] = ordering;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkYoungsMaterialInterface::RemoveAllMaterials()
{
  this->NumberOfDomains = -1;
  vtkDebugMacro(<< "Remove All Materials\n");
  this->Internals->NormalArrayMap.clear();
  this->Internals->OrderingArrayMap.clear();
  this->SetNumberOfMaterials(0);
}

int vtkYoungsMaterialInterface::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  // info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  return 1;
}

// internal classes
struct vtkYoungsMaterialInterface_IndexedValue
{
  double value;
  int index;
  bool operator<(const vtkYoungsMaterialInterface_IndexedValue& iv) const
  {
    return value < iv.value;
  }
};

struct vtkYoungsMaterialInterface_Mat
{
  // input
  vtkDataArray* fractionArray;
  vtkDataArray* normalArray;
  vtkDataArray* normalXArray;
  vtkDataArray* normalYArray;
  vtkDataArray* normalZArray;
  vtkDataArray* orderingArray;

  // temporary
  vtkIdType numberOfCells;
  vtkIdType numberOfPoints;
  vtkIdType cellCount;
  vtkIdType cellArrayCount;
  vtkIdType pointCount;
  vtkIdType* pointMap;

  // output
  std::vector<unsigned char> cellTypes;
  std::vector<vtkIdType> cells;
  vtkDataArray** outCellArrays;
  vtkDataArray** outPointArrays; // last point array is point coords
};

static inline void vtkYoungsMaterialInterface_GetPointData(int nPointData,
  vtkDataArray** inPointArrays, vtkDataSet* input,
  std::vector<std::pair<int, vtkIdType>>& prevPointsMap, int vtkNotUsed(nmat),
  vtkYoungsMaterialInterface_Mat* Mats, int a, vtkIdType i, double* t)
{
  if ((i) >= 0)
  {
    if (a < (nPointData - 1))
    {
      DBG_ASSERT(i < inPointArrays[a]->GetNumberOfTuples());
      inPointArrays[a]->GetTuple(i, t);
    }
    else
    {
      DBG_ASSERT(a == (nPointData - 1));
      DBG_ASSERT(i < input->GetNumberOfPoints());
      input->GetPoint(i, t);
    }
  }
  else
  {
    int j = -i - 1;
    DBG_ASSERT(j >= 0 && j < prevPointsMap.size());
    int prev_m = prevPointsMap[j].first;
    DBG_ASSERT(prev_m >= 0);
    vtkIdType prev_i = (prevPointsMap[j].second);
    DBG_ASSERT(prev_i >= 0 && prev_i < Mats[prev_m].outPointArrays[a]->GetNumberOfTuples());
    Mats[prev_m].outPointArrays[a]->GetTuple(prev_i, t);
  }
}

#define GET_POINT_DATA(a, i, t)                                                                    \
  vtkYoungsMaterialInterface_GetPointData(                                                         \
    nPointData, inPointArrays, input, prevPointsMap, nmat, Mats, a, i, t)

struct CellInfo
{
  double points[vtkYoungsMaterialInterface::MAX_CELL_POINTS][3];
  vtkIdType pointIds[vtkYoungsMaterialInterface::MAX_CELL_POINTS];
  int triangulation[vtkYoungsMaterialInterface::MAX_CELL_POINTS * 4];
  int edges[vtkYoungsMaterialInterface::MAX_CELL_POINTS][2];

  int dim;
  int np;
  int nf;
  int ntri;
  int type;
  int nEdges;

  bool triangulationOk;
  bool needTriangulation;

  CellInfo()
    : dim(2)
    , np(0)
    , nf(0)
    , ntri(0)
    , type(VTK_EMPTY_CELL)
    , nEdges(0)
    , triangulationOk(false)
    , needTriangulation(false)
  {
  }
};

int vtkYoungsMaterialInterface::CellProduceInterface(
  int dim, int np, double fraction, double minFrac, double maxFrac)
{
  return ((dim == 3 && np >= 4) || (dim == 2 && np >= 3)) &&
    (this->UseFractionAsDistance ||
      ((fraction > minFrac) && (fraction < maxFrac || this->FillMaterial)));
}

void vtkYoungsMaterialInterface::RemoveAllMaterialBlockMappings()
{
  vtkDebugMacro(<< "RemoveAllMaterialBlockMappings\n");
  this->MaterialBlockMapping->Reset();
}

void vtkYoungsMaterialInterface::AddMaterialBlockMapping(int b)
{
  vtkDebugMacro(<< "AddMaterialBlockMapping " << b << "\n");
  this->MaterialBlockMapping->InsertNextValue(b);
}

void vtkYoungsMaterialInterface::UpdateBlockMapping()
{
  int n = this->MaterialBlockMapping->GetNumberOfTuples();
  int curmat = -1;
  for (int i = 0; i < n; ++i)
  {
    int b = this->MaterialBlockMapping->GetValue(i);
    vtkDebugMacro(<< "MaterialBlockMapping " << b << "\n");
    if (b < 0)
      curmat = (-b) - 1;
    else
    {
      vtkDebugMacro(<< "Material " << curmat << ": Adding block " << b << "\n");
      this->Internals->Materials[curmat].blocks.insert(b);
    }
  }
}

//------------------------------------------------------------------------------
void vtkYoungsMaterialInterface::Aggregate(int nmat, int* inputsPerMaterial)
{
  // Calculate number of domains
  this->NumberOfDomains = 0;
  for (int m = 0; m < nmat; ++m)
  {
    // Sum all counts from all processes
    int inputsPerMaterialSum = inputsPerMaterial[m];
    this->NumberOfDomains = std::max(inputsPerMaterialSum, this->NumberOfDomains);

    // Reset array
    inputsPerMaterial[m] = 0;
  }
}

//------------------------------------------------------------------------------
int vtkYoungsMaterialInterface::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  this->UpdateBlockMapping();

  this->NumberOfDomains = -1;

  // get composite input
  vtkCompositeDataSet* compositeInput =
    vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // get typed output
  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (compositeInput == nullptr || output == nullptr)
  {
    vtkErrorMacro(<< "Invalid algorithm connection\n");
    return 0;
  }

  // debug statistics
  vtkIdType debugStats_PrimaryTriangulationfailed = 0;
  vtkIdType debugStats_Triangulationfailed = 0;
  vtkIdType debugStats_NullNormal = 0;
  vtkIdType debugStats_NoInterfaceFound = 0;

  // Initialize number of materials
  int nmat = static_cast<int>(this->Internals->Materials.size());
  if (nmat <= 0)
  {
    vtkErrorMacro(<< "Invalid materials size\n");
    return 0;
  }

  // allocate composite iterator
  vtkSmartPointer<vtkCompositeDataIterator> inputIterator;
  inputIterator.TakeReference(compositeInput->NewIterator());
  inputIterator->SkipEmptyNodesOn();
  inputIterator->InitTraversal();
  inputIterator->GoToFirstItem();

  // first compute number of domains
  int* inputsPerMaterial = new int[nmat];
  for (int i = 0; i < nmat; ++i)
  {
    inputsPerMaterial[i] = 0;
  }

  while (!inputIterator->IsDoneWithTraversal())
  {
    if (this->CheckAbort())
    {
      break;
    }
    vtkDataSet* input = vtkDataSet::SafeDownCast(inputIterator->GetCurrentDataObject());
    // Composite indices begin at 1 (0 is the root)
    int composite_index = inputIterator->GetCurrentFlatIndex();
    inputIterator->GoToNextItem();

    if (input && input->GetNumberOfCells() > 0)
    {
      int m = 0;
      for (std::vector<vtkYoungsMaterialInterfaceInternals::MaterialDescription>::iterator it =
             this->Internals->Materials.begin();
           it != this->Internals->Materials.end(); ++it, ++m)
      {
        double range[2];
        bool materialHasBlock = ((*it).blocks.find(composite_index) != (*it).blocks.end());
        if ((this->UseAllBlocks || materialHasBlock) &&
          input->GetCellData()->GetRange(it->volume().c_str(), range) &&
          range[1] > this->VolumeFractionRange[0])
        {
          ++inputsPerMaterial[m];
        }
      }
    }
  }

  // Perform parallel aggregation when needed (nothing in serial)
  if (!this->CheckAbort())
  {
    this->Aggregate(nmat, inputsPerMaterial);
  }

  // map containing output blocks
  std::map<int, vtkSmartPointer<vtkUnstructuredGrid>> outputBlocks;

  // iterate over input blocks
  inputIterator->InitTraversal();
  inputIterator->GoToFirstItem();
  while (inputIterator->IsDoneWithTraversal() == 0)
  {
    if (this->CheckAbort())
    {
      break;
    }
    vtkDataSet* input = vtkDataSet::SafeDownCast(inputIterator->GetCurrentDataObject());

    // Composite indices begin at 1 (0 is the root)
    int composite_index = inputIterator->GetCurrentFlatIndex();
    inputIterator->GoToNextItem();

    // make some variables visible by the debugger
    int nCellData = input->GetCellData()->GetNumberOfArrays();
    int nPointData = input->GetPointData()->GetNumberOfArrays();
    vtkIdType nCells = input->GetNumberOfCells();
    vtkIdType nPoints = input->GetNumberOfPoints();

    // -------------- temporary data initialization -------------------
    vtkDataArray** inCellArrays = new vtkDataArray*[nCellData];
    for (int i = 0; i < nCellData; i++)
    {
      inCellArrays[i] = input->GetCellData()->GetArray(i);
    }

    vtkDataArray** inPointArrays =
      new vtkDataArray*[nPointData + 1]; // last point array is point coords
    int* pointArrayOffset = new int[nPointData + 1];
    int pointDataComponents = 0;
    for (int i = 0; i < nPointData; i++)
    {
      inPointArrays[i] = input->GetPointData()->GetArray(i);
      pointArrayOffset[i] = pointDataComponents;
      pointDataComponents += inPointArrays[i]->GetNumberOfComponents();
    }
    // we add another data array for point coords
    pointArrayOffset[nPointData] = pointDataComponents;
    pointDataComponents += 3;
    nPointData++;

    vtkYoungsMaterialInterface_Mat* Mats = new vtkYoungsMaterialInterface_Mat[nmat];
    {
      int m = 0;
      for (std::vector<vtkYoungsMaterialInterfaceInternals::MaterialDescription>::iterator it =
             this->Internals->Materials.begin();
           it != this->Internals->Materials.end(); ++it, ++m)
      {
        Mats[m].fractionArray = input->GetCellData()->GetArray((*it).volume().c_str());
        Mats[m].normalArray =
          input->GetCellData()->GetArray((*it).normal(*this->Internals).c_str());
        Mats[m].normalXArray = input->GetCellData()->GetArray((*it).normalX().c_str());
        Mats[m].normalYArray = input->GetCellData()->GetArray((*it).normalY().c_str());
        Mats[m].normalZArray = input->GetCellData()->GetArray((*it).normalZ().c_str());
        Mats[m].orderingArray =
          input->GetCellData()->GetArray((*it).ordering(*this->Internals).c_str());

        if (!Mats[m].fractionArray)
        {
          vtkDebugMacro(<< "Material " << m << ": volume fraction array '" << (*it).volume()
                        << "' not found\n");
        }
        if (!Mats[m].orderingArray)
        {
          vtkDebugMacro(<< "Material " << m << " material ordering array '"
                        << (*it).ordering(*this->Internals) << "' not found\n");
        }
        if (!Mats[m].normalArray && !Mats[m].normalXArray && !Mats[m].normalYArray &&
          !Mats[m].normalZArray)
        {
          vtkDebugMacro(<< "Material " << m << " normal  array '" << (*it).normal(*this->Internals)
                        << "' not found\n");
        }

        bool materialHasBlock = ((*it).blocks.find(composite_index) != (*it).blocks.end());
        if (!this->UseAllBlocks && !materialHasBlock)
        {
          Mats[m].fractionArray =
            nullptr; // TODO: we certainly can do better to avoid material calculations
        }

        Mats[m].numberOfCells = 0;
        Mats[m].cellCount = 0;
        Mats[m].cellArrayCount = 0;

        Mats[m].outCellArrays = new vtkDataArray*[nCellData];
        for (int i = 0; i < nCellData; ++i)
        {
          Mats[m].outCellArrays[i] = vtkDataArray::CreateDataArray(inCellArrays[i]->GetDataType());
          Mats[m].outCellArrays[i]->SetName(inCellArrays[i]->GetName());
          Mats[m].outCellArrays[i]->SetNumberOfComponents(inCellArrays[i]->GetNumberOfComponents());
        }

        Mats[m].numberOfPoints = 0;
        Mats[m].pointCount = 0;
        Mats[m].outPointArrays = new vtkDataArray*[nPointData];

        for (int i = 0; i < (nPointData - 1); i++)
        {
          Mats[m].outPointArrays[i] =
            vtkDataArray::CreateDataArray(inPointArrays[i]->GetDataType());
          Mats[m].outPointArrays[i]->SetName(inPointArrays[i]->GetName());
          Mats[m].outPointArrays[i]->SetNumberOfComponents(
            inPointArrays[i]->GetNumberOfComponents());
        }
        Mats[m].outPointArrays[nPointData - 1] = vtkDoubleArray::New();
        Mats[m].outPointArrays[nPointData - 1]->SetName("Points");
        Mats[m].outPointArrays[nPointData - 1]->SetNumberOfComponents(3);
      }
    }

    // --------------- per material number of interfaces estimation ------------
    for (vtkIdType c = 0; c < nCells; c++)
    {
      vtkCell* vtkcell = input->GetCell(c);
      int cellDim = vtkcell->GetCellDimension();
      int np = vtkcell->GetNumberOfPoints();
      int nf = vtkcell->GetNumberOfFaces();

      for (int m = 0; m < nmat; m++)
      {
        double fraction =
          (Mats[m].fractionArray != nullptr) ? Mats[m].fractionArray->GetTuple1(c) : 0;
        if (this->CellProduceInterface(
              cellDim, np, fraction, this->VolumeFractionRange[0], this->VolumeFractionRange[1]))
        {
          if (cellDim == 2)
          {
            Mats[m].numberOfPoints += 2;
          }
          else
          {
            Mats[m].numberOfPoints += nf;
          }
          if (this->FillMaterial)
          {
            Mats[m].numberOfPoints += np - 1;
          }
          Mats[m].numberOfCells++;
        }
      }
    }

    // allocation of output arrays
    for (int m = 0; m < nmat; m++)
    {
      vtkDebugMacro(<< "Mat #" << m << " : cells=" << Mats[m].numberOfCells << ", points="
                    << Mats[m].numberOfPoints << ", FillMaterial=" << this->FillMaterial << "\n");
      for (int i = 0; i < nCellData; i++)
      {
        Mats[m].outCellArrays[i]->Allocate(
          Mats[m].numberOfCells * Mats[m].outCellArrays[i]->GetNumberOfComponents());
      }
      for (int i = 0; i < nPointData; i++)
      {
        Mats[m].outPointArrays[i]->Allocate(
          Mats[m].numberOfPoints * Mats[m].outPointArrays[i]->GetNumberOfComponents());
      }
      Mats[m].cellTypes.reserve(Mats[m].numberOfCells);
      Mats[m].cells.reserve(Mats[m].numberOfCells + Mats[m].numberOfPoints);
      Mats[m].pointMap = new vtkIdType[nPoints];
      for (vtkIdType i = 0; i < nPoints; i++)
      {
        Mats[m].pointMap[i] = -1;
      }
    }

    // --------------------------- core computation --------------------------
    vtkIdList* ptIds = vtkIdList::New();
    vtkConvexPointSet* cpsCell = vtkConvexPointSet::New();

    double* interpolatedValues = new double[MAX_CELL_POINTS * pointDataComponents];
    vtkYoungsMaterialInterface_IndexedValue* matOrdering =
      new vtkYoungsMaterialInterface_IndexedValue[nmat];

    std::vector<std::pair<int, vtkIdType>> prevPointsMap;
    prevPointsMap.reserve(MAX_CELL_POINTS * nmat);

    for (vtkIdType ci = 0; ci < nCells; ci++)
    {
      if (this->CheckAbort())
      {
        break;
      }
      int interfaceEdges[MAX_CELL_POINTS * 2];
      double interfaceWeights[MAX_CELL_POINTS];
      int nInterfaceEdges;

      int insidePointIds[MAX_CELL_POINTS];
      int nInsidePoints;

      int outsidePointIds[MAX_CELL_POINTS];
      int nOutsidePoints;

      int outCellPointIds[MAX_CELL_POINTS];
      int nOutCellPoints;

      double referenceVolume = 1.0;
      double normal[3];
      bool normaleNulle = false;

      prevPointsMap.clear();

      // sort materials
      int nEffectiveMat = 0;
      for (int mi = 0; mi < nmat; mi++)
      {
        matOrdering[mi].index = mi;
        matOrdering[mi].value =
          (Mats[mi].orderingArray != nullptr) ? Mats[mi].orderingArray->GetTuple1(ci) : 0.0;

        double fraction =
          (Mats[mi].fractionArray != nullptr) ? Mats[mi].fractionArray->GetTuple1(ci) : 0;
        if (this->UseFractionAsDistance || fraction > this->VolumeFractionRange[0])
          nEffectiveMat++;
      }
      std::stable_sort(matOrdering, matOrdering + nmat);

      // read cell information for the first iteration
      // a temporary cell will then be generated after each iteration for the next one.
      vtkCell* vtkcell = input->GetCell(ci);
      CellInfo cell;
      cell.dim = vtkcell->GetCellDimension();
      cell.np = vtkcell->GetNumberOfPoints();
      cell.nf = vtkcell->GetNumberOfFaces();
      cell.type = vtkcell->GetCellType();

      /* copy points and point ids to lacal arrays.
         IMPORTANT NOTE : A negative point id refers to a point in the previous material.
         the material number and real point id can be found through the prevPointsMap. */
      for (int p = 0; p < cell.np; p++)
      {
        cell.pointIds[p] = vtkcell->GetPointId(p);
        DBG_ASSERT(cell.pointIds[p] >= 0 && cell.pointIds[p] < nPoints);
        vtkcell->GetPoints()->GetPoint(p, cell.points[p]);
      }

      /* Triangulate cell.
         IMPORTANT NOTE: triangulation is given with mesh point ids (not local cell ids)
         and are translated to cell local point ids. */
      cell.needTriangulation = false;
      cell.triangulationOk = (vtkcell->TriangulateIds(ci, ptIds) != 0);
      cell.ntri = 0;
      if (cell.triangulationOk)
      {
        cell.ntri = ptIds->GetNumberOfIds() / (cell.dim + 1);
        for (int i = 0; i < (cell.ntri * (cell.dim + 1)); i++)
        {
          vtkIdType j =
            std::find(cell.pointIds, cell.pointIds + cell.np, ptIds->GetId(i)) - cell.pointIds;
          DBG_ASSERT(j >= 0 && j < cell.np);
          cell.triangulation[i] = j;
        }
      }
      else
      {
        debugStats_PrimaryTriangulationfailed++;
        vtkWarningMacro(<< "Triangulation failed on primary cell\n");
      }

      // get 3D cell edges.
      if (cell.dim == 3)
      {
        vtkCell3D* cell3D = vtkCell3D::SafeDownCast(vtkcell);
        cell.nEdges = vtkcell->GetNumberOfEdges();
        for (int i = 0; i < cell.nEdges; i++)
        {
          const vtkIdType* edgePoints;
          cell3D->GetEdgePoints(i, edgePoints);
          cell.edges[i][0] = edgePoints[0];
          DBG_ASSERT(cell.edges[i][0] >= 0 && cell.edges[i][0] < cell.np);
          cell.edges[i][1] = edgePoints[1];
          DBG_ASSERT(cell.edges[i][1] >= 0 && cell.edges[i][1] < cell.np);
        }
      }

      // For debugging : ensure that we don't read anything from cell, but only from previously
      // filled arrays
      vtkcell = nullptr;

      int processedEfectiveMat = 0;

      // Loop for each material. Current cell is iteratively cut.
      for (int mi = 0; mi < nmat; mi++)
      {
        int m =
          this->ReverseMaterialOrder ? matOrdering[nmat - 1 - mi].index : matOrdering[mi].index;

        // Get volume fraction and interface plane normal from input arrays
        double fraction =
          (Mats[m].fractionArray != nullptr) ? Mats[m].fractionArray->GetTuple1(ci) : 0;

        // Normalize remaining volume fraction
        fraction = (referenceVolume > 0) ? (fraction / referenceVolume) : 0.0;

        if (this->CellProduceInterface(cell.dim, cell.np, fraction, this->VolumeFractionRange[0],
              this->VolumeFractionRange[1]))
        {
          CellInfo nextCell; // empty cell by default
          int interfaceCellType = VTK_EMPTY_CELL;

          if ((!mi) || (!this->OnionPeel))
          {
            normal[0] = 0;
            normal[1] = 0;
            normal[2] = 0;

            if (Mats[m].normalArray != nullptr)
              Mats[m].normalArray->GetTuple(ci, normal);
            if (Mats[m].normalXArray != nullptr)
              normal[0] = Mats[m].normalXArray->GetTuple1(ci);
            if (Mats[m].normalYArray != nullptr)
              normal[1] = Mats[m].normalYArray->GetTuple1(ci);
            if (Mats[m].normalZArray != nullptr)
              normal[2] = Mats[m].normalZArray->GetTuple1(ci);

            // work-around for degenerated normals
            if (vtkMath::Norm(normal) == 0.0) // should it be <EPSILON ?
            {
              debugStats_NullNormal++;
              normaleNulle = true;
              normal[0] = 1.0;
              normal[1] = 0.0;
              normal[2] = 0.0;
            }
            else
            {
              vtkMath::Normalize(normal);
            }
            if (this->InverseNormal)
            {
              normal[0] = -normal[0];
              normal[1] = -normal[1];
              normal[2] = -normal[2];
            }
          }

          // count how many materials we've processed so far
          if (fraction > this->VolumeFractionRange[0])
          {
            processedEfectiveMat++;
          }

          // -= case where the entire input cell is passed through =-
          if ((!this->UseFractionAsDistance && fraction > this->VolumeFractionRange[1] &&
                this->FillMaterial) ||
            (this->UseFractionAsDistance && normaleNulle))
          {
            interfaceCellType = cell.type;
            // Mats[m].cellTypes.push_back( cell.type );
            nOutCellPoints = nInsidePoints = cell.np;
            nInterfaceEdges = 0;
            nOutsidePoints = 0;
            for (int p = 0; p < cell.np; p++)
            {
              outCellPointIds[p] = insidePointIds[p] = p;
            }
            // remaining volume is an empty cell (nextCell is left as is)
          }

          // -= case where the entire cell is ignored =-

          else if (!this->UseFractionAsDistance &&
            (fraction < this->VolumeFractionRange[0] ||
              (fraction > this->VolumeFractionRange[1] && !this->FillMaterial) ||
              !cell.triangulationOk))
          {
            interfaceCellType = VTK_EMPTY_CELL;
            // Mats[m].cellTypes.push_back( VTK_EMPTY_CELL );

            nOutCellPoints = 0;
            nInterfaceEdges = 0;
            nInsidePoints = 0;
            nOutsidePoints = 0;

            // remaining volume is the same cell
            nextCell = cell;

            if (!cell.triangulationOk)
            {
              debugStats_Triangulationfailed++;
              vtkWarningMacro(<< "Cell triangulation failed\n");
            }
          }

          // -= 2D case =-
          else if (cell.dim == 2)
          {
            int nRemCellPoints;
            int remCellPointIds[MAX_CELL_POINTS];

            int triangles[MAX_CELL_POINTS][3];
            for (int i = 0; i < cell.ntri; i++)
              for (int j = 0; j < 3; j++)
              {
                triangles[i][j] = cell.triangulation[i * 3 + j];
                DBG_ASSERT(triangles[i][j] >= 0 && triangles[i][j] < cell.np);
              }

            bool interfaceFound = vtkYoungsMaterialInterfaceCellCut::cellInterfaceD(cell.points,
              cell.np, triangles, cell.ntri, fraction, normal, this->AxisSymetric != 0,
              this->UseFractionAsDistance != 0, interfaceEdges, interfaceWeights, nOutCellPoints,
              outCellPointIds, nRemCellPoints, remCellPointIds);

            if (interfaceFound)
            {
              nInterfaceEdges = 2;
              interfaceCellType = this->FillMaterial ? VTK_POLYGON : VTK_LINE;
              // Mats[m].cellTypes.push_back( this->FillMaterial ? VTK_POLYGON : VTK_LINE );

              // remaining volume is a polygon
              nextCell.dim = 2;
              nextCell.np = nRemCellPoints;
              nextCell.nf = nRemCellPoints;
              nextCell.type = VTK_POLYGON;

              // build polygon triangulation for next iteration
              nextCell.ntri = nextCell.np - 2;
              for (int i = 0; i < nextCell.ntri; i++)
              {
                nextCell.triangulation[i * 3 + 0] = 0;
                nextCell.triangulation[i * 3 + 1] = i + 1;
                nextCell.triangulation[i * 3 + 2] = i + 2;
              }
              nextCell.triangulationOk = true;
              nextCell.needTriangulation = false;

              // populate prevPointsMap and next iteration cell point ids
              int ni = 0;
              for (int i = 0; i < nRemCellPoints; i++)
              {
                vtkIdType id = remCellPointIds[i];
                if (id < 0)
                {
                  id = -(int)(prevPointsMap.size() + 1);
                  DBG_ASSERT((-id - 1) == prevPointsMap.size());
                  prevPointsMap.emplace_back(
                    m, Mats[m].pointCount + ni); // intersection points will be added first
                  ni++;
                }
                else
                {
                  DBG_ASSERT(id >= 0 && id < cell.np);
                  id = cell.pointIds[id];
                }
                nextCell.pointIds[i] = id;
              }
              DBG_ASSERT(ni == nInterfaceEdges);

              // filter out points inside material volume
              nInsidePoints = 0;
              for (int i = 0; i < nOutCellPoints; i++)
              {
                if (outCellPointIds[i] >= 0)
                  insidePointIds[nInsidePoints++] = outCellPointIds[i];
              }

              if (!this->FillMaterial) // keep only interface points

              {
                int n = 0;
                for (int i = 0; i < nOutCellPoints; i++)
                {
                  if (outCellPointIds[i] < 0)
                    outCellPointIds[n++] = outCellPointIds[i];
                }
                nOutCellPoints = n;
              }
            }
            else
            {
              vtkWarningMacro(<< "no interface found for cell " << ci << ", mi=" << mi
                              << ", m=" << m << ", frac=" << fraction << "\n");
              nInterfaceEdges = 0;
              nOutCellPoints = 0;
              nInsidePoints = 0;
              nOutsidePoints = 0;
              interfaceCellType = VTK_EMPTY_CELL;
              // Mats[m].cellTypes.push_back( VTK_EMPTY_CELL );
              // remaining volume is the original cell left unmodified
              nextCell = cell;
            }
          }

          // -= 3D case =-

          else
          {
            int tetras[MAX_CELL_POINTS][4];
            for (int i = 0; i < cell.ntri; i++)
              for (int j = 0; j < 4; j++)
              {
                tetras[i][j] = cell.triangulation[i * 4 + j];
              }

            // compute interface polygon
            vtkYoungsMaterialInterfaceCellCut::cellInterface3D(cell.np, cell.points, cell.nEdges,
              cell.edges, cell.ntri, tetras, fraction, normal, this->UseFractionAsDistance != 0,
              nInterfaceEdges, interfaceEdges, interfaceWeights, nInsidePoints, insidePointIds,
              nOutsidePoints, outsidePointIds);

            if (nInterfaceEdges > cell.nf ||
              nInterfaceEdges < 3) // degenerated case, considered as null interface
            {
              debugStats_NoInterfaceFound++;
              vtkDebugMacro(<< "no interface found for cell " << ci << ", mi=" << mi << ", m=" << m
                            << ", frac=" << fraction << "\n");
              nInterfaceEdges = 0;
              nOutCellPoints = 0;
              nInsidePoints = 0;
              nOutsidePoints = 0;
              interfaceCellType = VTK_EMPTY_CELL;
              // Mats[m].cellTypes.push_back( VTK_EMPTY_CELL );

              // in this case, next iteration cell is the same
              nextCell = cell;
            }
            else
            {
              nOutCellPoints = 0;

              for (int e = 0; e < nInterfaceEdges; e++)
              {
                outCellPointIds[nOutCellPoints++] = -e - 1;
              }

              if (this->FillMaterial)
              {
                interfaceCellType = VTK_CONVEX_POINT_SET;
                // Mats[m].cellTypes.push_back( VTK_CONVEX_POINT_SET );
                for (int p = 0; p < nInsidePoints; p++)
                {
                  outCellPointIds[nOutCellPoints++] = insidePointIds[p];
                }
              }
              else
              {
                interfaceCellType = VTK_POLYGON;
                // Mats[m].cellTypes.push_back( VTK_POLYGON );
              }

              // NB: Remaining volume is a convex point set
              // IMPORTANT NOTE: next iteration cell cannot be entirely built right now.
              // in this particular case we'll finish it at the end of the material loop.
              // If no other material remains to be processed, then skip this step.
              if (mi < (nmat - 1) && processedEfectiveMat < nEffectiveMat)
              {
                nextCell.type = VTK_CONVEX_POINT_SET;
                nextCell.np = nInterfaceEdges + nOutsidePoints;
                vtkcell = cpsCell;
                vtkcell->Points->Reset();
                vtkcell->PointIds->Reset();
                vtkcell->Points->SetNumberOfPoints(nextCell.np);
                vtkcell->PointIds->SetNumberOfIds(nextCell.np);
                for (int i = 0; i < nextCell.np; i++)
                {
                  vtkcell->PointIds->SetId(i, i);
                }
                // nf, ntri and triangulation have to be computed later on, when point coords are
                // computed
                nextCell.needTriangulation = true;
              }

              for (int i = 0; i < nInterfaceEdges; i++)
              {
                vtkIdType id = -(int)(prevPointsMap.size() + 1);
                DBG_ASSERT((-id - 1) == prevPointsMap.size());
                // Interpolated points will be added consecutively
                prevPointsMap.emplace_back(m, Mats[m].pointCount + i);
                nextCell.pointIds[i] = id;
              }
              for (int i = 0; i < nOutsidePoints; i++)
              {
                nextCell.pointIds[nInterfaceEdges + i] = cell.pointIds[outsidePointIds[i]];
              }
            }

            // check correctness of next cell's point ids
            for (int i = 0; i < nextCell.np; i++)
            {
              DBG_ASSERT(
                (nextCell.pointIds[i] < 0 && (-nextCell.pointIds[i] - 1) < prevPointsMap.size()) ||
                (nextCell.pointIds[i] >= 0 && nextCell.pointIds[i] < nPoints));
            }
          } // End 3D case

          //  create output cell
          if (interfaceCellType != VTK_EMPTY_CELL)
          {

            // set type of cell
            Mats[m].cellTypes.push_back(interfaceCellType);

            // interpolate point values for cut edges
            for (int e = 0; e < nInterfaceEdges; e++)
            {
              double t = interfaceWeights[e];
              for (int p = 0; p < nPointData; p++)
              {
                double v0[16];
                double v1[16];
                int nc = Mats[m].outPointArrays[p]->GetNumberOfComponents();
                int ep0 = cell.pointIds[interfaceEdges[e * 2 + 0]];
                int ep1 = cell.pointIds[interfaceEdges[e * 2 + 1]];
                GET_POINT_DATA(p, ep0, v0);
                GET_POINT_DATA(p, ep1, v1);
                for (int c = 0; c < nc; c++)
                {
                  interpolatedValues[e * pointDataComponents + pointArrayOffset[p] + c] =
                    v0[c] + t * (v1[c] - v0[c]);
                }
              }
            }

            // copy point values
            for (int e = 0; e < nInterfaceEdges; e++)
            {
              for (int a = 0; a < nPointData; a++)
              {
                DBG_ASSERT(nptId == Mats[m].outPointArrays[a]->GetNumberOfTuples());
                Mats[m].outPointArrays[a]->InsertNextTuple(
                  interpolatedValues + e * pointDataComponents + pointArrayOffset[a]);
              }
            }
            int pointsCopied = 0;
            int prevMatInterfToBeAdded = 0;
            if (this->FillMaterial)
            {
              for (int p = 0; p < nInsidePoints; p++)
              {
                vtkIdType ptId = cell.pointIds[insidePointIds[p]];
                if (ptId >= 0)
                {
                  if (Mats[m].pointMap[ptId] == -1)
                  {
                    vtkIdType nptId = Mats[m].pointCount + nInterfaceEdges + pointsCopied;
                    Mats[m].pointMap[ptId] = nptId;
                    pointsCopied++;
                    for (int a = 0; a < nPointData; a++)
                    {
                      DBG_ASSERT(nptId == Mats[m].outPointArrays[a]->GetNumberOfTuples());
                      double tuple[16];
                      GET_POINT_DATA(a, ptId, tuple);
                      Mats[m].outPointArrays[a]->InsertNextTuple(tuple);
                    }
                  }
                }
                else
                {
                  prevMatInterfToBeAdded++;
                }
              }
            }

            // Populate connectivity array and add extra points from previous
            // edge intersections that are used but not inserted yet
            int prevMatInterfAdded = 0;
            Mats[m].cells.push_back(nOutCellPoints);
            Mats[m].cellArrayCount++;
            for (int p = 0; p < nOutCellPoints; ++p)
            {
              int nptId;
              int pointIndex = outCellPointIds[p];
              if (pointIndex >= 0)
              {
                // An original point is encountered (not an edge intersection)
                DBG_ASSERT(pointIndex >= 0 && pointIndex < cell.np);
                vtkIdType ptId = cell.pointIds[pointIndex];
                if (ptId >= 0)
                {
                  // Interface from a previous iteration
                  DBG_ASSERT(ptId >= 0 && ptId < nPoints);
                  nptId = Mats[m].pointMap[ptId];
                }
                else
                {
                  nptId = Mats[m].pointCount + nInterfaceEdges + pointsCopied + prevMatInterfAdded;
                  prevMatInterfAdded++;
                  for (int a = 0; a < nPointData; a++)
                  {
                    DBG_ASSERT(nptId == Mats[m].outPointArrays[a]->GetNumberOfTuples());
                    double tuple[16];
                    GET_POINT_DATA(a, ptId, tuple);
                    Mats[m].outPointArrays[a]->InsertNextTuple(tuple);
                  }
                }
              }
              else
              {
                int interfaceIndex = -pointIndex - 1;
                DBG_ASSERT(interfaceIndex >= 0 && interfaceIndex < nInterfaceEdges);
                nptId = Mats[m].pointCount + interfaceIndex;
              }
              DBG_ASSERT(nptId >= 0 &&
                nptId <
                  (Mats[m].pointCount + nInterfaceEdges + pointsCopied + prevMatInterfToBeAdded));
              Mats[m].cells.push_back(nptId);
              Mats[m].cellArrayCount++;
            }
            (void)prevMatInterfToBeAdded;

            Mats[m].pointCount += nInterfaceEdges + pointsCopied + prevMatInterfAdded;

            // Copy cell arrays
            for (int a = 0; a < nCellData; a++)
            {
              Mats[m].outCellArrays[a]->InsertNextTuple(inCellArrays[a]->GetTuple(ci));
            }
            Mats[m].cellCount++;

            // Check for equivalence between counters and container sizes
            DBG_ASSERT(Mats[m].cellCount == Mats[m].cellTypes.size());
            DBG_ASSERT(Mats[m].cellArrayCount == Mats[m].cells.size());

            // Populate next iteration cell point coordinates
            for (int i = 0; i < nextCell.np; i++)
            {
              DBG_ASSERT(
                (nextCell.pointIds[i] < 0 && (-nextCell.pointIds[i] - 1) < prevPointsMap.size()) ||
                (nextCell.pointIds[i] >= 0 && nextCell.pointIds[i] < nPoints));
              GET_POINT_DATA((nPointData - 1), nextCell.pointIds[i], nextCell.points[i]);
            }

            // for the convex point set, we need to first compute point coords before triangulation
            // (no fixed topology)
            if (nextCell.needTriangulation && mi < (nmat - 1) &&
              processedEfectiveMat < nEffectiveMat)
            {
              //                       for(int myi = 0;myi<nextCell.np;myi++)
              //                       {
              //                                cerr<<"p["<<myi<<"]=("<<nextCell.points[myi][0]<<','<<nextCell.points[myi][1]<<','<<nextCell.points[myi][2]<<")
              //                                ";
              //                       }
              //                       cerr<<endl;

              vtkcell->Initialize();
              nextCell.nf = vtkcell->GetNumberOfFaces();
              if (nextCell.dim == 3)
              {
                vtkCell3D* cell3D = vtkCell3D::SafeDownCast(vtkcell);
                nextCell.nEdges = vtkcell->GetNumberOfEdges();
                for (int i = 0; i < nextCell.nEdges; i++)
                {
                  const vtkIdType* edgePoints;
                  cell3D->GetEdgePoints(i, edgePoints);
                  nextCell.edges[i][0] = edgePoints[0];
                  DBG_ASSERT(nextCell.edges[i][0] >= 0 && nextCell.edges[i][0] < nextCell.np);
                  nextCell.edges[i][1] = edgePoints[1];
                  DBG_ASSERT(nextCell.edges[i][1] >= 0 && nextCell.edges[i][1] < nextCell.np);
                }
              }
              nextCell.triangulationOk = (vtkcell->TriangulateIds(ci, ptIds) != 0);
              nextCell.ntri = 0;
              if (nextCell.triangulationOk)
              {
                nextCell.ntri = ptIds->GetNumberOfIds() / (nextCell.dim + 1);
                for (int i = 0; i < (nextCell.ntri * (nextCell.dim + 1)); i++)
                {
                  vtkIdType j = ptIds->GetId(i); // cell ids have been set with local ids
                  DBG_ASSERT(j >= 0 && j < nextCell.np);
                  nextCell.triangulation[i] = j;
                }
              }
              else
              {
                debugStats_Triangulationfailed++;
                vtkWarningMacro(<< "Triangulation failed. Info: cell " << ci << ", material " << mi
                                << ", np=" << nextCell.np << ", nf=" << nextCell.nf
                                << ", ne=" << nextCell.nEdges << "\n");
              }
              nextCell.needTriangulation = false;
              vtkcell = nullptr;
            }

            // switch to next cell
            cell = nextCell;

          } // end of 'interface was found'

          else
          {
            vtkcell = nullptr;
          }

        } // end of 'cell is ok'

        //                      else // cell is ignored
        //                      {
        //                              //vtkWarningMacro(<<"ignoring cell #"<<ci<<", m="<<m<<",
        //                              mi="<<mi<<", frac="<<fraction<<"\n");
        //                      }

        // update reference volume
        referenceVolume -= fraction;

      } // for materials

    } // for cells
    delete[] pointArrayOffset;
    delete[] inPointArrays;
    delete[] inCellArrays;

    ptIds->Delete();
    cpsCell->Delete();
    delete[] interpolatedValues;
    delete[] matOrdering;

    // finish output creation
    //       output->SetNumberOfBlocks( nmat );
    for (int m = 0; m < nmat; m++)
    {
      if (Mats[m].cellCount > 0 && Mats[m].pointCount > 0)
      {
        vtkDebugMacro(<< "Mat #" << m << " : cellCount=" << Mats[m].cellCount << ", numberOfCells="
                      << Mats[m].numberOfCells << ", pointCount=" << Mats[m].pointCount
                      << ", numberOfPoints=" << Mats[m].numberOfPoints << "\n");
      }

      delete[] Mats[m].pointMap;

      vtkSmartPointer<vtkUnstructuredGrid> ugOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();

      // set points
      Mats[m].outPointArrays[nPointData - 1]->Squeeze();
      vtkPoints* points = vtkPoints::New();
      points->SetDataTypeToDouble();
      points->SetNumberOfPoints(Mats[m].pointCount);
      points->SetData(Mats[m].outPointArrays[nPointData - 1]);
      Mats[m].outPointArrays[nPointData - 1]->Delete();
      ugOutput->SetPoints(points);
      points->Delete();

      // set cell connectivity
      vtkIdTypeArray* cellArrayData = vtkIdTypeArray::New();
      cellArrayData->SetNumberOfValues(Mats[m].cellArrayCount);
      vtkIdType* cellArrayDataPtr = cellArrayData->WritePointer(0, Mats[m].cellArrayCount);
      for (vtkIdType i = 0; i < Mats[m].cellArrayCount; i++)
        cellArrayDataPtr[i] = Mats[m].cells[i];

      vtkCellArray* cellArray = vtkCellArray::New();
      cellArray->AllocateExact(Mats[m].cellCount, Mats[m].cellArrayCount - Mats[m].cellCount);
      cellArray->ImportLegacyFormat(cellArrayData);
      cellArrayData->Delete();

      // set cell types
      vtkUnsignedCharArray* cellTypes = vtkUnsignedCharArray::New();
      cellTypes->SetNumberOfValues(Mats[m].cellCount);
      unsigned char* cellTypesPtr = cellTypes->WritePointer(0, Mats[m].cellCount);
      for (vtkIdType i = 0; i < Mats[m].cellCount; i++)
        cellTypesPtr[i] = Mats[m].cellTypes[i];

      // attach connectivity arrays to data set
      ugOutput->SetCells(cellTypes, cellArray);
      cellArray->Delete();
      cellTypes->Delete();

      // attach point arrays
      for (int i = 0; i < nPointData - 1; i++)
      {
        Mats[m].outPointArrays[i]->Squeeze();
        ugOutput->GetPointData()->AddArray(Mats[m].outPointArrays[i]);
        Mats[m].outPointArrays[i]->Delete();
      }

      // attach cell arrays
      for (int i = 0; i < nCellData; i++)
      {
        Mats[m].outCellArrays[i]->Squeeze();
        ugOutput->GetCellData()->AddArray(Mats[m].outCellArrays[i]);
        Mats[m].outCellArrays[i]->Delete();
      }

      delete[] Mats[m].outCellArrays;
      delete[] Mats[m].outPointArrays;

      // activate attributes similarly to input
      for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
      {
        vtkDataArray* attr = input->GetCellData()->GetAttribute(i);
        if (attr != nullptr)
        {
          ugOutput->GetCellData()->SetActiveAttribute(attr->GetName(), i);
        }
      }
      for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
      {
        vtkDataArray* attr = input->GetPointData()->GetAttribute(i);
        if (attr != nullptr)
        {
          ugOutput->GetPointData()->SetActiveAttribute(attr->GetName(), i);
        }
      }

      // add material data set to multiblock output
      if (ugOutput && ugOutput->GetNumberOfCells() > 0)
      {
        int domain = inputsPerMaterial[m];
        outputBlocks[domain * nmat + m] = ugOutput;
        ++inputsPerMaterial[m];
      }
    }
    delete[] Mats;
  } // Iterate over input blocks

  delete[] inputsPerMaterial;

  if (debugStats_PrimaryTriangulationfailed)
  {
    vtkDebugMacro(<< "PrimaryTriangulationfailed " << debugStats_PrimaryTriangulationfailed
                  << "\n");
  }
  if (debugStats_Triangulationfailed)
  {
    vtkDebugMacro(<< "Triangulationfailed " << debugStats_Triangulationfailed << "\n");
  }
  if (debugStats_NullNormal)
  {
    vtkDebugMacro(<< "NullNormal " << debugStats_NullNormal << "\n");
  }
  if (debugStats_NoInterfaceFound)
  {
    vtkDebugMacro(<< "NoInterfaceFound " << debugStats_NoInterfaceFound << "\n");
  }
  // Build final composite output. also tagging blocks with their associated Id
  vtkDebugMacro(<< this->NumberOfDomains << " Domains, " << nmat << " Materials\n");

  output->SetNumberOfBlocks(0);
  output->SetNumberOfBlocks(nmat);

  for (int m = 0; m < nmat && !this->CheckAbort(); ++m)
  {
    vtkMultiBlockDataSet* matBlock = vtkMultiBlockDataSet::New();
    matBlock->SetNumberOfBlocks(this->NumberOfDomains);
    output->SetBlock(m, matBlock);
    matBlock->Delete();
  }

  int blockIndex = 0;
  for (std::map<int, vtkSmartPointer<vtkUnstructuredGrid>>::iterator it = outputBlocks.begin();
       it != outputBlocks.end() && !this->CheckAbort(); ++it, ++blockIndex)
  {
    if (it->second->GetNumberOfCells() > 0)
    {
      int mat = it->first % nmat;
      int dom = it->first / nmat;
      vtkMultiBlockDataSet* matBlock = vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(mat));
      matBlock->SetBlock(dom, it->second);
    }
  }

  return 1;
}

#undef GET_POINT_DATA

VTK_ABI_NAMESPACE_END
/* ------------------------------------------------------------------------------------------
   --- Low level computations including interface placement and intersection line/polygon ---
   ------------------------------------------------------------------------------------------ */

// here after the low-level functions that compute placement of the interface given a normal vector
// and a set of simplices
namespace vtkYoungsMaterialInterfaceCellCutInternals
{
VTK_ABI_NAMESPACE_BEGIN
// define base vector types and operators or use those provided by CUDA

struct double2
{
  double x, y;
};
struct uchar4
{
  unsigned char x, y, z, w;
};
struct uchar3
{
  unsigned char x, y, z;
};

/* -------------------------------------------------------- */
/* ----------- DOUBLE ------------------------------------- */
/* -------------------------------------------------------- */
struct double3
{
  double x, y, z;
};
struct double4
{
  double x, y, z, w;
};

static inline double3 operator*(double f, double3 v)
{
  return double3{ v.x * f, v.y * f, v.z * f };
}

static inline double2 operator*(double f, double2 v)
{
  return double2{ v.x * f, v.y * f };
}

static inline double3 operator+(double3 a, double3 b)
{
  return double3{ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline double2 operator+(double2 a, double2 b)
{
  return double2{ a.x + b.x, a.y + b.y };
}

static inline void operator+=(double3& b, double3 a)
{
  b.x += a.x;
  b.y += a.y;
  b.z += a.z;
}
static inline void operator+=(double2& b, double2 a)
{
  b.x += a.x;
  b.y += a.y;
}

static inline double3 operator-(double3 a, double3 b)
{
  return double3{ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline double2 operator-(double2 a, double2 b)
{
  return double2{ a.x - b.x, a.y - b.y };
}

static inline void operator/=(double2& b, double f)
{
  b.x /= f;
  b.y /= f;
}

static inline void operator/=(double3& b, double f)
{
  b.x /= f;
  b.y /= f;
  b.z /= f;
}

static inline double dot(double2 a, double2 b)
{
  return a.x * b.x + a.y * b.y;
}

static inline double dot(double3 a, double3 b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline double3 cross(double3 A, double3 B)
{
  return double3{ A.y * B.z - A.z * B.y, A.z * B.x - A.x * B.z, A.x * B.y - A.y * B.x };
}

/**************************************
 *** Precision dependent constants   ***
 ***************************************/

// double ( default )
#define NEWTON_NITER 32

/**************************************
 ***       Debugging                 ***
 ***************************************/
#define DBG_MESG(m) (void)0

/**************************************
 ***          Macros                 ***
 ***************************************/

// local arrays allocation
#if defined(__GNUC__) // Warning, this is a gcc extension, not all compiler accept it
#define ALLOC_LOCAL_ARRAY(name, type, n) type name[(n)]
#define FREE_LOCAL_ARRAY(name, type, n)
#else
#include <malloc.h>
#define ALLOC_LOCAL_ARRAY(name, type, n) type* name = (type*)malloc(sizeof(type) * (n))
#define FREE_LOCAL_ARRAY(name, type, n) free(name)
#endif

#ifdef __GNUC__
#define LOCAL_ARRAY_SIZE(n) n
#else
#define LOCAL_ARRAY_SIZE(n) 128
#endif

/*********************
 *** Triangle area ***
 *********************/
/*
  Formula from VTK in vtkTriangle.cxx, method TriangleArea
*/
static inline double triangleSurf(double3 p1, double3 p2, double3 p3)
{
  const double3 e1 = p2 - p1;
  const double3 e2 = p3 - p2;
  const double3 e3 = p1 - p3;

  const double a = dot(e1, e1);
  const double b = dot(e2, e2);
  const double c = dot(e3, e3);

  return 0.25 * sqrt(fabs(4 * a * c - (a - b + c) * (a - b + c)));
}

/*************************
 *** Tetrahedra volume ***
 *************************/

static inline double tetraVolume(double3 p0, double3 p1, double3 p2, double3 p3)
{
  double3 A = p1 - p0;
  double3 B = p2 - p0;
  double3 C = p3 - p0;
  double3 BC = cross(B, C);
  return fabs(dot(A, BC) / 6.0);
}

/*******************************************
 *** Evaluation of a polynomial function ***
 *******************************************/
static inline double evalPolynomialFunc(double2 F, double x)
{
  return F.x * x + F.y;
}

static inline double evalPolynomialFunc(double3 F, double x)
{
  double y = (F.x * x + F.y) * x;
  return y + F.z;
}

static inline double evalPolynomialFunc(double4 F, double x)
{
  double y = ((F.x * x + F.y) * x + F.z) * x;
  return y + F.w; // this increases numerical stability when compiled with -ffloat-store
}

/*****************************************
 *** Integral of a polynomial function ***
 *****************************************/
static inline double3 integratePolynomialFunc(double2 linearFunc)
{
  return double3{ linearFunc.x / 2, linearFunc.y, 0 };
}

static inline double4 integratePolynomialFunc(double3 quadFunc)
{
  return double4{ quadFunc.x / 3, quadFunc.y / 2, quadFunc.z, 0 };
}

/****************************
 *** Linear interpolation ***
 ****************************/
static inline double3 linearInterp(double t0, double3 x0, double t1, double3 x1, double t)
{
  double f = (t1 != t0) ? (t - t0) / (t1 - t0) : 0;
  return x0 + f * (x1 - x0);
}

static inline double2 linearInterp(double t0, double2 x0, double t1, double2 x1, double t)
{
  double f = (t1 != t0) ? (t - t0) / (t1 - t0) : 0.0;
  return x0 + f * (x1 - x0);
}

/****************************************
 *** Quadratic interpolation function ***
 ****************************************/
static inline double3 quadraticInterpFunc(
  double x0, double y0, double x1, double y1, double x2, double y2)
{
  // Formula from the book 'Maillages', page 409

  // non-degenerated case (really a quadratic function)
  if (x1 > x0 && x2 > x1)
  {
    // denominators
    const double d0 = (x0 - x1) * (x0 - x2);
    const double d1 = (x1 - x0) * (x1 - x2);
    const double d2 = (x2 - x0) * (x2 - x1);

    // coefficients for the quadratic interpolation of (x0,y0) , (x1,y1) and p2(x2,y2)
    return double3{ (y0 / d0) + (y1 / d1) + (y2 / d2),                          // x^2 term
      (y0 * (-x1 - x2) / d0) + (y1 * (-x0 - x2) / d1) + (y2 * (-x0 - x1) / d2), // x term
      (y0 * (x1 * x2) / d0) + (y1 * (x0 * x2) / d1) + (y2 * (x0 * x1) / d2) };  // constant term
  }

  // linear case : 2 out of the 3 points are the same
  else if (x2 > x0)
  {
    return double3{ 0,       // x^2 term
      (y2 - y0) / (x2 - x0), // x term
      y0 };                  // constant term
  }

  // degenerated case
  return double3{ 0, 0, 0 };
}

/****************************
 *** Newton search method ***
 ****************************/
static inline double newtonSearchPolynomialFunc(
  double3 F, double2 dF, double value, double xmin, double xmax)
{
  // translate F, because newton searches for the 0 of the derivative
  F.z -= value;

  // start with x, the closest of xmin, xmean and xmax
  const double ymin = evalPolynomialFunc(F, xmin);
  const double ymax = evalPolynomialFunc(F, xmax);

  double x = (xmin + xmax) * 0.5;
  double y = evalPolynomialFunc(F, x);

  // search x where F(x) = 0
  for (int i = 0; i < NEWTON_NITER; i++)
  {
    DBG_MESG("F(" << x << ")=" << y);
    // Xi+1 = Xi - F'(x)/F''(x)
    double d = evalPolynomialFunc(dF, x);
    if (d == 0)
    {
      d = 1;
      y = 0;
    }
    x = x - (y / d);
    y = evalPolynomialFunc(F, x);
  }

  // check that the solution is not worse than the 2 bounds
  DBG_MESG("F(" << xmin << ")=" << ymin << ", "
                << "F(" << x << ")=" << y << ", "
                << "F(" << xmax << ")=" << ymax);
  y = fabs(y);
  if (fabs(ymin) < y)
  {
    x = xmin;
  }
  if (fabs(ymax) < y)
  {
    x = xmax;
  }

  DBG_MESG("F(" << x << ")=" << y);
  return x;
}

static inline double newtonSearchPolynomialFunc(
  double4 F, double3 dF, double value, double xmin, double xmax)
{
  // translate F, because newton searches for the 0 of the derivative
  F.w -= value;

  // start with x, the closest of xmin, xmean and xmax
  const double ymin = evalPolynomialFunc(F, xmin);
  const double ymax = evalPolynomialFunc(F, xmax);

  double x = (xmin + xmax) * 0.5;
  double y = evalPolynomialFunc(F, x);

  // search x where F(x) = 0
  for (int i = 0; i < NEWTON_NITER; i++)
  {
    DBG_MESG("F(" << x << ")=" << y);
    // Xi+1 = Xi - F'(x)/F''(x)
    double d = evalPolynomialFunc(dF, x);
    if (d == 0)
    {
      d = 1;
      y = 0;
    }
    x = x - (y / d);
    y = evalPolynomialFunc(F, x);
  }

  // check that the solution is not worse than taking one of the 2 bounds
  DBG_MESG("F(" << xmin << ")=" << ymin << ", "
                << "F(" << x << ")=" << y << ", "
                << "F(" << xmax << ")=" << ymax);
  y = fabs(y);
  if (fabs(ymin) < y)
  {
    x = xmin;
  }
  if (fabs(ymax) < y)
  {
    x = xmax;
  }

  DBG_MESG("F(" << x << ")=" << y);
  return x;
}

/***********************
 *** Sorting methods ***
 ***********************/
static inline uchar3 sortTriangle(uchar3 t, unsigned char* i)
{
  if (i[t.y] < i[t.x])
    std::swap(t.x, t.y);
  if (i[t.z] < i[t.y])
    std::swap(t.y, t.z);
  if (i[t.y] < i[t.x])
    std::swap(t.x, t.y);
  return t;
}

typedef unsigned char IntType;
/***********************
 *** Sorting methods ***
 ***********************/
static inline void sortVertices(int n, const double3* vertices, double3 normal, IntType* indices)
{
  // insertion sort : slow but symmetrical across all instances
  for (int i = 0; i < n; i++)
  {
    int imin = i;
    double dmin = dot(vertices[indices[i]], normal);
    for (int j = i + 1; j < n; j++)
    {
      double d = dot(vertices[indices[j]], normal);
      imin = (d < dmin) ? j : imin;
      dmin = std::min(dmin, d);
    }
    std::swap(i, imin);
  }
}

static inline void sortVertices(int n, const double2* vertices, double2 normal, IntType* indices)
{
  // insertion sort : slow but symmetrical across all instances
  for (int i = 0; i < n; i++)
  {
    int imin = i;
    double dmin = dot(vertices[indices[i]], normal);
    for (int j = i + 1; j < n; j++)
    {
      double d = dot(vertices[indices[j]], normal);
      imin = (d < dmin) ? j : imin;
      dmin = std::min(dmin, d);
    }
    std::swap(i, imin);
  }
}

static inline uchar4 sortTetra(uchar4 t, IntType* i)
{
  if (i[t.y] < i[t.x])
    std::swap(t.x, t.y);
  if (i[t.w] < i[t.z])
    std::swap(t.z, t.w);
  if (i[t.z] < i[t.y])
    std::swap(t.y, t.z);
  if (i[t.y] < i[t.x])
    std::swap(t.x, t.y);
  if (i[t.w] < i[t.z])
    std::swap(t.z, t.w);
  if (i[t.z] < i[t.y])
    std::swap(t.y, t.z);
  return t;
}

static inline double makeTriangleSurfaceFunctions(
  uchar3 triangle, const double3* vertices, double3 normal, double2 func[2])
{

  // 1. load the data
  const double3 v0 = vertices[triangle.x];
  const double3 v1 = vertices[triangle.y];
  const double3 v2 = vertices[triangle.z];

  const double d0 = dot(v0, normal);
  const double d1 = dot(v1, normal);
  const double d2 = dot(v2, normal);

  DBG_MESG("v0 = " << v0.x << ',' << v0.y << " d0=" << d0);
  DBG_MESG("v1 = " << v1.x << ',' << v1.y << " d1=" << d1);
  DBG_MESG("v2 = " << v2.x << ',' << v2.y << " d2=" << d2);

  // 2. compute

  // compute vector from point on v0-v2 that has distance d1 from Plane0
  double3 I = linearInterp(d0, v0, d2, v2, d1);
  DBG_MESG("I = " << I.x << ',' << I.y);
  double3 vec = v1 - I;
  double length = sqrt(dot(vec, vec));
  DBG_MESG("length = " << length);

  // side length function = (x-d0) * length / (d1-d0) = (length/(d1-d0)) * x - length * d0 / (d1-d0)
  double2 linearFunc01 = double2{ length / (d1 - d0), -length * d0 / (d1 - d0) };
  // surface function = integral of distance function starting at d0
  func[0] = double2{ 0, 0 };
  if (d1 > d0)
  {
    func[0] = linearFunc01;
  }

  // side length function = (d2-x) * length / (d2-d1) = (-length/(d2-d1)) * x + d2*length / (d2-d1)
  double2 linearFunc12 = double2{ -length / (d2 - d1), d2 * length / (d2 - d1) };
  // surface function = integral of distance function starting at d1
  func[1] = double2{ 0, 0 };
  if (d2 > d1)
  {
    func[1] = linearFunc12;
  }

  return triangleSurf(v0, v1, v2);
}

static inline double findTriangleSetCuttingPlane(double3 normal, // IN  , normal vector
  double fraction,                                               // IN  , volume fraction
  int nv,                                                        // IN  , number of vertices
  int nt,                                                        // IN  , number of triangles
  const uchar3* tv,       // IN  , triangles connectivity, size=nt
  const double3* vertices // IN  , vertex coordinates, size=nv
)
{
  // only need nv-1 derivs but allocate nv as gcc freaks out
  // with nv-1 as an argument
  ALLOC_LOCAL_ARRAY(derivatives, double2, nv);
  ALLOC_LOCAL_ARRAY(index, unsigned char, nv);
  ALLOC_LOCAL_ARRAY(rindex, unsigned char, nv);

  // initialization
  for (int i = 0; i < nv; i++)
  {
    index[i] = i;
  }

  for (int i = 0; i < (nv - 1); i++)
  {
    derivatives[i] = double2{ 0, 0 };
  }

  // sort vertices in the normal vector direction
  sortVertices(nv, vertices, normal, index);

  // reverse indirection table
  for (int i = 0; i < nv; i++)
  {
    rindex[index[i]] = i;
  }

  // total area
  double surface = 0;

  // construction of the truncated volume piecewise cubic function
  for (int i = 0; i < nt; i++)
  {
    // area of the interface-tetra intersection at points P1 and P2
    uchar3 triangle = sortTriangle(tv[i], rindex);
    DBG_MESG("\ntriangle " << i << " : " << tv[i].x << ',' << tv[i].y << ',' << tv[i].z << " -> "
                           << triangle.x << ',' << triangle.y << ',' << triangle.z);

    // compute the volume function derivative pieces
    double2 triangleSurfFunc[2];
    surface += makeTriangleSurfaceFunctions(triangle, vertices, normal, triangleSurfFunc);

#ifdef DEBUG
    for (int k = 0; k < 2; k++)
    {
      DBG_MESG("surf'[" << k << "] = " << triangleSurfFunc[k].x << ',' << triangleSurfFunc[k].y);
    }
#endif

    // surface function bounds
    unsigned int i0 = rindex[triangle.x];
    unsigned int i1 = rindex[triangle.y];
    unsigned int i2 = rindex[triangle.z];

    DBG_MESG("surf(x) steps = " << i0 << ',' << i1 << ',' << i2);

    DBG_MESG("Adding surfFunc onto [" << i0 << ';' << i1 << "]");
    for (unsigned int j = i0; j < i1; j++)
    {
      derivatives[j] += triangleSurfFunc[0];
    }

    DBG_MESG("Adding surfFunc onto [" << i1 << ';' << i2 << "]");
    for (unsigned int j = i1; j < i2; j++)
    {
      derivatives[j] += triangleSurfFunc[1];
    }
  }

  // target volume fraction we're looking for
  double y = surface * fraction;
  DBG_MESG("surface = " << surface << ", surface*fraction = " << y);

  // integrate area function pieces to obtain volume function pieces
  double sum = 0;
  double3 surfaceFunction = double3{ 0, 0, 0 };
  double xmin = 0;
  double xmax = dot(vertices[index[0]], normal);
  int s = -1;
  while (sum < y && s < (nv - 2))
  {
    xmin = xmax;
    y -= sum;
    ++s;
    double3 F = integratePolynomialFunc(derivatives[s]);
    F.z = -evalPolynomialFunc(F, xmin);
    surfaceFunction = F;
    xmax = dot(vertices[index[s + 1]], normal);
    sum = evalPolynomialFunc(F, xmax);
  }
  s = std::max(s, 0);

  DBG_MESG("step=" << s << ", x in [" << xmin << ';' << xmax << ']');
  DBG_MESG("surface reminder = " << y);

  // newton search
  double x = newtonSearchPolynomialFunc(surfaceFunction, derivatives[s], y, xmin, xmax);

  DBG_MESG("final x = " << x);

  FREE_LOCAL_ARRAY(derivatives, double2, nv - 1);
  FREE_LOCAL_ARRAY(index, unsigned char, nv);
  FREE_LOCAL_ARRAY(rindex, unsigned char, nv);

  return x;
}

/*
  compute the derivatives of the piecewise cubic function of the volume behind the cutting cone
  (axis symmetric 2D plane)
*/
static inline void makeConeVolumeDerivatives(
  uchar3 triangle, const double2* vertices, double2 normal, double3 deriv[2])
{

  // 1. load the data
  const double2 v0 = vertices[triangle.x];
  const double2 v1 = vertices[triangle.y];
  const double2 v2 = vertices[triangle.z];

  // 2. compute
  const double d0 = dot(v0, normal);
  const double d1 = dot(v1, normal);
  const double d2 = dot(v2, normal);

  DBG_MESG("v0 = " << v0.x << ',' << v0.y << " d0=" << d0);
  DBG_MESG("v1 = " << v1.x << ',' << v1.y << " d1=" << d1);
  DBG_MESG("v2 = " << v2.x << ',' << v2.y << " d2=" << d2);

  // compute vector from point on v0-v2 that has distance d1 from Plane0
  double2 I = linearInterp(d0, v0, d2, v2, d1);
  DBG_MESG("I = " << I.x << ',' << I.y);
  double2 vec = v1 - I;
  double length = sqrt(dot(vec, vec));
  DBG_MESG("length = " << length);

  // compute truncated cone surface at d1
  double Isurf = vtkMath::Pi() * fabs(I.y + v1.y) *
    length; // 2 * vtkMath::Pi() * ( (I.y+v1.y) * 0.5 ) * length ;
  double coef;

  // build cubic volume functions derivatives
  coef = (d1 > d0) ? (Isurf / ((d1 - d0) * (d1 - d0))) : 0.0;
  deriv[0] = coef * double3{ 1, -2 * d0, d0 * d0 };

  coef = (d2 > d1) ? (Isurf / ((d2 - d1) * (d2 - d1))) : 0.0;
  deriv[1] = coef * double3{ 1, -2 * d2, d2 * d2 };
}

static inline double findTriangleSetCuttingCone(double2 normal, // IN  , normal vector
  double fraction,                                              // IN  , volume fraction
  int nv,                                                       // IN  , number of vertices
  int nt,                                                       // IN  , number of triangles
  const uchar3* tv,       // IN  , triangles connectivity, size=nt
  const double2* vertices // IN  , vertex coordinates, size=nv
)
{
  ALLOC_LOCAL_ARRAY(derivatives, double3, nv - 1);
  ALLOC_LOCAL_ARRAY(index, unsigned char, nv);
  ALLOC_LOCAL_ARRAY(rindex, unsigned char, nv);

  // initialization
  for (int i = 0; i < nv; i++)
  {
    index[i] = i;
  }

  for (int i = 0; i < (nv - 1); i++)
  {
    derivatives[i] = double3{ 0, 0, 0 };
  }

  // sort vertices along normal vector
  sortVertices(nv, vertices, normal, index);

  // reverse indirection table
  for (int i = 0; i < nv; i++)
  {
    rindex[index[i]] = i;
  }

  // construction of the truncated volume piecewise cubic function
  for (int i = 0; i < nt; i++)
  {
    // area of the interface-tetra intersection at points P1 and P2
    uchar3 triangle = sortTriangle(tv[i], rindex);
    DBG_MESG("\ntriangle " << i << " : " << tv[i].x << ',' << tv[i].y << ',' << tv[i].z << " -> "
                           << triangle.x << ',' << triangle.y << ',' << triangle.z);

    // compute the volume function derivatives pieces
    double3 coneVolDeriv[2];
    makeConeVolumeDerivatives(triangle, vertices, normal, coneVolDeriv);

    // area function bounds
    unsigned int i0 = rindex[triangle.x];
    unsigned int i1 = rindex[triangle.y];
    unsigned int i2 = rindex[triangle.z];

    DBG_MESG("surf(x) steps = " << i0 << ',' << i1 << ',' << i2);

    DBG_MESG("Adding surfFunc onto [" << i0 << ';' << i1 << "]");
    for (unsigned int j = i0; j < i1; j++)
    {
      derivatives[j] += coneVolDeriv[0];
    }

    DBG_MESG("Adding surfFunc onto [" << i1 << ';' << i2 << "]");
    for (unsigned int j = i1; j < i2; j++)
    {
      derivatives[j] += coneVolDeriv[1];
    }
  }

  double surface = 0;
  double xmin = 0;
  double xmax = dot(vertices[index[0]], normal);
  for (int i = 0; i < (nv - 1); i++)
  {
    xmin = xmax;
    double4 F = integratePolynomialFunc(derivatives[i]);
    F.w = -evalPolynomialFunc(F, xmin);
    xmax = dot(vertices[index[i + 1]], normal);
    surface += evalPolynomialFunc(F, xmax);
  }

  double y = surface * fraction;
  DBG_MESG("surface = " << surface << ", surface*fraction = " << y);

  // integrate area function pieces to obtain volume function pieces
  double sum = 0;
  double4 volumeFunction = double4{ 0, 0, 0, 0 };
  xmax = dot(vertices[index[0]], normal);
  int s = -1;
  while (sum < y && s < (nv - 2))
  {
    xmin = xmax;
    y -= sum;
    ++s;
    double4 F = integratePolynomialFunc(derivatives[s]);
    F.w = -evalPolynomialFunc(F, xmin);
    volumeFunction = F;
    xmax = dot(vertices[index[s + 1]], normal);
    sum = evalPolynomialFunc(F, xmax);
  }
  s = std::max(s, 0);

  // look for the function piece that contain the target volume
  DBG_MESG("step=" << s << ", x in [" << xmin << ';' << xmax << ']');
  DBG_MESG("surface reminder = " << y);

  // newton search method
  double x = newtonSearchPolynomialFunc(volumeFunction, derivatives[s], y, xmin, xmax);

  DBG_MESG("final x = " << x);

  FREE_LOCAL_ARRAY(derivatives, double3, nv - 1);
  FREE_LOCAL_ARRAY(index, unsigned char, nv);
  FREE_LOCAL_ARRAY(rindex, unsigned char, nv);

  return x;
}

/*
  Computes the area of the intersection between the plane, orthogonal to the 'normal' vector,
  that passes through P1 (resp. P2), and the given tetrahedron.
  the resulting area function, is a function of the intersection area given the distance of the
  cutting plane to the origin.
*/
static inline double tetraPlaneSurfFunc(
  uchar4 tetra, const double3* vertices, double3 normal, double3 func[3])
{
  // 1. load the data

  const double3 v0 = vertices[tetra.x];
  const double3 v1 = vertices[tetra.y];
  const double3 v2 = vertices[tetra.z];
  const double3 v3 = vertices[tetra.w];

  const double d0 = dot(v0, normal);
  const double d1 = dot(v1, normal);
  const double d2 = dot(v2, normal);
  const double d3 = dot(v3, normal);

#ifdef DEBUG
  bool ok = (d0 <= d1 && d1 <= d2 && d2 <= d3);
  if (!ok)
  {
    DBG_MESG("d0=" << d0 << ", d1=" << d1 << ", d2=" << d2 << ", d3=" << d3);
  }
  assert(d0 <= d1 && d1 <= d2 && d2 <= d3);
#endif

  // 2. compute

  // Intersection surface in p1
  const double surf1 =
    triangleSurf(v1, linearInterp(d0, v0, d2, v2, d1), linearInterp(d0, v0, d3, v3, d1));

  // Compute the intersection surfice in the middle of p1 and p2.
  // The intersection is a quadric of a,b,c,d
  const double d12 = (d1 + d2) * 0.5;
  const double3 a = linearInterp(d0, v0, d2, v2, d12);
  const double3 b = linearInterp(d0, v0, d3, v3, d12);
  const double3 c = linearInterp(d1, v1, d3, v3, d12);
  const double3 d = linearInterp(d1, v1, d2, v2, d12);

  const double surf12 = triangleSurf(a, b, d) + triangleSurf(b, c, d);

  // intersection  surface in p2
  const double surf2 =
    triangleSurf(v2, linearInterp(d0, v0, d3, v3, d2), linearInterp(d1, v1, d3, v3, d2));

  // Construct the surface functions
  double coef;

  // Search S0(x) = coef * (x-d0)^2
  coef = (d1 > d0) ? (surf1 / ((d1 - d0) * (d1 - d0))) : 0.0;
  func[0] = coef * double3{ 1, -2 * d0, d0 * d0 };

  // Search S1(x) = quadric interpolation of surf1, surf12, surf2 at the points d1, d12, d2
  func[1] = quadraticInterpFunc(d1, surf1, d12, surf12, d2, surf2);

  // S(x) = coef * (d3-x)^2
  coef = (d3 > d2) ? (surf2 / ((d3 - d2) * (d3 - d2))) : 0.0;
  func[2] = coef * double3{ 1, -2 * d3, d3 * d3 };

  return tetraVolume(v0, v1, v2, v3);
}

static inline double findTetraSetCuttingPlane(double3 normal, // IN  , normal vector
  double fraction,                                            // IN  , volume fraction
  int nv,                                                     // IN  , number of vertices
  int nt,                                                     // IN  , number of tetras
  const uchar4* tv,                                           // IN  , tetras connectivity, size=nt
  const double3* vertices                                     // IN  , vertex coordinates, size=nv
)
{
  ALLOC_LOCAL_ARRAY(rindex, unsigned char, nv);
  ALLOC_LOCAL_ARRAY(index, unsigned char, nv);
  ALLOC_LOCAL_ARRAY(derivatives, double3, nv - 1);

  // initialization
  for (int i = 0; i < nv; i++)
  {
    index[i] = i;
  }

  // sort vertices in the normal vector direction
  sortVertices(nv, vertices, normal, index);

  // reverse indirection table
  for (int i = 0; i < nv; i++)
  {
    rindex[index[i]] = i;
  }

#ifdef DEBUG
  for (int i = 0; i < nv; i++)
  {
    DBG_MESG("index[" << i << "]=" << index[i] << ", rindex[" << i << "]=" << rindex[i]);
  }
#endif

  for (int i = 0; i < (nv - 1); i++)
  {
    derivatives[i] = double3{ 0, 0, 0 };
  }

  double volume = 0;

  // construction of the truncated volume piecewise cubic function
  for (int i = 0; i < nt; i++)
  {
    // area of the interface-tetra intersection at points P1 and P2
    uchar4 tetra = sortTetra(tv[i], rindex);
    DBG_MESG("\ntetra " << i << " : " << tv[i].x << ',' << tv[i].y << ',' << tv[i].z << ','
                        << tv[i].w << " -> " << tetra.x << ',' << tetra.y << ',' << tetra.z << ','
                        << tetra.w);

    // compute the volume function derivative pieces
    double3 tetraSurfFunc[3];
    volume += tetraPlaneSurfFunc(tetra, vertices, normal, tetraSurfFunc);

#ifdef DEBUG
    for (int k = 0; k < 3; k++)
    {
      DBG_MESG("surf[" << k << "] = " << tetraSurfFunc[k].x << ',' << tetraSurfFunc[k].y << ','
                       << tetraSurfFunc[k].z);
    }
#endif

    // surface function bounds
    unsigned int i0 = rindex[tetra.x];
    unsigned int i1 = rindex[tetra.y];
    unsigned int i2 = rindex[tetra.z];
    unsigned int i3 = rindex[tetra.w];

    DBG_MESG("surf(x) steps = " << i0 << ',' << i1 << ',' << i2 << ',' << i3);

    DBG_MESG("Adding surfFunc onto [" << i0 << ';' << i1 << "]");
    for (unsigned int j = i0; j < i1; j++)
      derivatives[j] += tetraSurfFunc[0];

    DBG_MESG("Adding surfFunc onto [" << i1 << ';' << i2 << "]");
    for (unsigned int j = i1; j < i2; j++)
      derivatives[j] += tetraSurfFunc[1];

    DBG_MESG("Adding surfFunc onto [" << i2 << ';' << i3 << "]");
    for (unsigned int j = i2; j < i3; j++)
      derivatives[j] += tetraSurfFunc[2];
  }

  // target volume fraction we're looking for
  double y = volume * fraction;
  DBG_MESG("volume = " << volume << ", volume*fraction = " << y);

  // integrate area function pieces to obtain volume function pieces
  double sum = 0;
  double4 volumeFunction = double4{ 0, 0, 0, 0 };
  double xmin = 0;
  double xmax = dot(vertices[index[0]], normal);
  int s = -1;
  while (sum < y && s < (nv - 2))
  {
    xmin = xmax;
    y -= sum;
    ++s;
    double4 F = integratePolynomialFunc(derivatives[s]);
    F.w = -evalPolynomialFunc(F, xmin);
    volumeFunction = F;
    xmax = dot(vertices[index[s + 1]], normal);
    sum = evalPolynomialFunc(F, xmax);
  }
  s = std::max(s, 0);
  // F, F' : free derivatives

  // search the function range that contains the value
  DBG_MESG("step=" << s << ", x in [" << xmin << ';' << xmax << ']');

  /* each function pieces start from 0,
     compute the volume in this function piece.
  */
  // y -= sum;
  DBG_MESG("volume reminder = " << y);

  // search by newton
  double x = newtonSearchPolynomialFunc(volumeFunction, derivatives[s], y, xmin, xmax);

  DBG_MESG("final x = " << x);

  FREE_LOCAL_ARRAY(rindex, unsigned char, nv);
  FREE_LOCAL_ARRAY(index, unsigned char, nv);
  FREE_LOCAL_ARRAY(derivatives, double3, nv - 1);

  return x;
}

typedef double Real;
typedef double2 Real2;
typedef double3 Real3;
typedef double4 Real4;

struct VertexInfo
{
  double coord[3];
  double weight;
  int eid[2];
};

struct CWVertex
{
  double angle;
  double coord[3];
  double weight;
  int eid[2];
  bool operator<(const CWVertex& v) const { return angle < v.angle; }
};
VTK_ABI_NAMESPACE_END
} /* namespace vtkYoungsMaterialInterfaceCellCutInternals */

VTK_ABI_NAMESPACE_BEGIN
// ------------------------------------
//         ####     ####
//             #    #   #
//          ###     #   #
//             #    #   #
//         ####     ####
// ------------------------------------
void vtkYoungsMaterialInterfaceCellCut::cellInterface3D(int ncoords, double coords[][3], int nedge,
  int cellEdges[][2], int ntetra, int tetraPointIds[][4], double fraction, double normal[3],
  bool useFractionAsDistance, int& np, int eids[], double weights[], int& nInside, int inPoints[],
  int& nOutside, int outPoints[])
{
  // normalize the normal vector if the norm >0
  double nlen2 = normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2];
  if (nlen2 > 0)
  {
    double nlen = sqrt(nlen2);
    normal[0] /= nlen;
    normal[1] /= nlen;
    normal[2] /= nlen;
  }
  else
  {
    normal[0] = 1;
    normal[1] = 0;
    normal[2] = 0;
  }

  double dmin, dmax;
  dmin = dmax = coords[0][0] * normal[0] + coords[0][1] * normal[1] + coords[0][2] * normal[2];
  for (int i = 0; i < ncoords; i++)
  {
    double d = coords[i][0] * normal[0] + coords[i][1] * normal[1] + coords[i][2] * normal[2];
    if (d < dmin)
      dmin = d;
    else if (d > dmax)
      dmax = d;
  }

  // compute plane's offset ( D parameter in Ax+By+Cz+D=0 )
  double d = useFractionAsDistance
    ? fraction
    : findTetraSetCuttingPlane(normal, fraction, ncoords, coords, ntetra, tetraPointIds);

  // compute vertex distances to interface plane
  double dist[MAX_CELL_POINTS];
  for (int i = 0; i < ncoords; i++)
  {
    dist[i] = coords[i][0] * normal[0] + coords[i][1] * normal[1] + coords[i][2] * normal[2] + d;
  }

  // get in/out points
  nInside = 0;
  nOutside = 0;
  for (int i = 0; i < ncoords; i++)
  {
    if (dist[i] <= 0.0)
    {
      inPoints[nInside++] = i;
    }
    else
    {
      outPoints[nOutside++] = i;
    }
  }

  double center[3] = { 0, 0, 0 };
  double polygon[MAX_CELL_POINTS][3];

  // compute intersections between edges and interface plane
  np = 0;
  for (int i = 0; i < nedge; i++)
  {
    int e0 = cellEdges[i][0];
    int e1 = cellEdges[i][1];
    if (dist[e0] * dist[e1] < 0)
    {
      double edist = dist[e1] - dist[e0];
      double t;
      if (edist != 0)
      {
        t = (0 - dist[e0]) / edist;
        t = vtkMath::ClampValue(t, 0.0, 1.0);
      }
      else
      {
        t = 0;
      }

      for (int c = 0; c < 3; c++)
      {
        polygon[np][c] = coords[e0][c] + t * (coords[e1][c] - coords[e0][c]);
        center[c] += polygon[np][c];
      }
      eids[np * 2 + 0] = e0;
      eids[np * 2 + 1] = e1;
      weights[np] = t;
      np++;
    }
  }

  // sort points
  if (np > 3)
  {
    // compute the center of the polygon
    for (int comp = 0; comp < 3; comp++)
    {
      center[comp] /= np;
    }

    // compute the main direction to be in a 2D case
    int maxDim = 0;
    if (fabs(normal[1]) > fabs(normal[maxDim]))
      maxDim = 1;
    if (fabs(normal[2]) > fabs(normal[maxDim]))
      maxDim = 2;
    int xd = 0, yd = 1;
    switch (maxDim)
    {
      case 0:
        xd = 1;
        yd = 2;
        break;
      case 1:
        xd = 0;
        yd = 2;
        break;
      case 2:
        xd = 0;
        yd = 1;
        break;
    }

    // compute the angles of the polygon vertices
    vtkYoungsMaterialInterfaceCellCutInternals::CWVertex pts[MAX_CELL_POINTS];
    for (int i = 0; i < np; i++)
    {
      double vec[3];
      for (int comp = 0; comp < 3; comp++)
      {
        pts[i].coord[comp] = polygon[i][comp];
        vec[comp] = polygon[i][comp] - center[comp];
      }

      pts[i].weight = weights[i];
      pts[i].eid[0] = eids[i * 2 + 0];
      pts[i].eid[1] = eids[i * 2 + 1];
      pts[i].angle = atan2(vec[yd], vec[xd]);
    }
    std::sort(pts, pts + np);
    for (int i = 0; i < np; i++)
    {
      weights[i] = pts[i].weight;
      eids[i * 2 + 0] = pts[i].eid[0];
      eids[i * 2 + 1] = pts[i].eid[1];
    }
  }
}

double vtkYoungsMaterialInterfaceCellCut::findTetraSetCuttingPlane(const double normal[3],
  double fraction, int vertexCount, const double vertices[][3], int tetraCount,
  const int tetras[][4])
{
  vtkYoungsMaterialInterfaceCellCutInternals::Real3 N = { normal[0], normal[1], normal[2] };
  vtkYoungsMaterialInterfaceCellCutInternals::Real3 V[LOCAL_ARRAY_SIZE(vertexCount)];
  vtkYoungsMaterialInterfaceCellCutInternals::uchar4 tet[LOCAL_ARRAY_SIZE(tetraCount)];

  for (int i = 0; i < vertexCount; i++)
  {
    V[i].x = vertices[i][0] - vertices[0][0];
    V[i].y = vertices[i][1] - vertices[0][1];
    V[i].z = vertices[i][2] - vertices[0][2];
  }

  vtkYoungsMaterialInterfaceCellCutInternals::Real3 vmin, vmax;
  vtkYoungsMaterialInterfaceCellCutInternals::Real scale;
  vmin = vmax = V[0];
  for (int i = 1; i < vertexCount; i++)
  {
    vmin.x = std::min(V[i].x, vmin.x);
    vmax.x = std::max(V[i].x, vmax.x);
    vmin.y = std::min(V[i].y, vmin.y);
    vmax.y = std::max(V[i].y, vmax.y);
    vmin.z = std::min(V[i].z, vmin.z);
    vmax.z = std::max(V[i].z, vmax.z);
  }
  scale = vmax.x - vmin.x;
  scale = std::max(vmax.y - vmin.y, scale);
  scale = std::max(vmax.z - vmin.z, scale);
  for (int i = 0; i < vertexCount; i++)
    V[i] /= scale;

  for (int i = 0; i < tetraCount; i++)
  {
    tet[i].x = tetras[i][0];
    tet[i].y = tetras[i][1];
    tet[i].z = tetras[i][2];
    tet[i].w = tetras[i][3];
  }

  double dist0 =
    vertices[0][0] * normal[0] + vertices[0][1] * normal[1] + vertices[0][2] * normal[2];
  double d = dist0 +
    vtkYoungsMaterialInterfaceCellCutInternals::findTetraSetCuttingPlane(
      N, fraction, vertexCount, tetraCount, tet, V) *
      scale;

  return -d;
}

// ------------------------------------
//         ####     ####
//             #    #   #
//          ###     #   #
//         #        #   #
//        #####     ####
// ------------------------------------

bool vtkYoungsMaterialInterfaceCellCut::cellInterfaceD(double points[][3], int nPoints,
  int triangles[][3], // TODO: int [] pour plus d'integration au niveau du dessus
  int nTriangles, double fraction, double normal[3], bool axisSymetric, bool useFractionAsDistance,
  int eids[4], double weights[2], int& polygonPoints, int polygonIds[], int& nRemPoints,
  int remPoints[])
{
  double d = useFractionAsDistance ? fraction
                                   : findTriangleSetCuttingPlane(normal, fraction, nPoints, points,
                                       nTriangles, triangles, axisSymetric);

  // compute vertex distances to interface plane
  double dist[LOCAL_ARRAY_SIZE(nPoints)];
  for (int i = 0; i < nPoints; i++)
  {
    dist[i] = points[i][0] * normal[0] + points[i][1] * normal[1] + points[i][2] * normal[2] + d;
  }

  // compute intersections between edges and interface line
  int np = 0;
  nRemPoints = 0;
  polygonPoints = 0;
  for (int i = 0; i < nPoints; i++)
  {
    int edge[2];
    edge[0] = i;
    edge[1] = (i + 1) % nPoints;
    if (dist[i] <= 0.0)
    {
      polygonIds[polygonPoints++] = i;
    }
    else
    {
      remPoints[nRemPoints++] = i;
    }
    if (np < 2)
    {
      if (dist[edge[0]] * dist[edge[1]] < 0.0)
      {
        double t = (0 - dist[edge[0]]) / (dist[edge[1]] - dist[edge[0]]);
        t = vtkMath::ClampValue(t, 0.0, 1.0);
        eids[np * 2 + 0] = edge[0];
        eids[np * 2 + 1] = edge[1];
        weights[np] = t;
        np++;
        polygonIds[polygonPoints++] = -np;
        remPoints[nRemPoints++] = -np;
      }
    }
  }

  return (np == 2);
}

double vtkYoungsMaterialInterfaceCellCut::findTriangleSetCuttingPlane(const double normal[3],
  double fraction, int vertexCount, const double vertices[][3], int triangleCount,
  const int triangles[][3], bool axisSymetric)
{
  double d;

  vtkYoungsMaterialInterfaceCellCutInternals::uchar3 tri[LOCAL_ARRAY_SIZE(triangleCount)];
  for (int i = 0; i < triangleCount; i++)
  {
    tri[i].x = triangles[i][0];
    tri[i].y = triangles[i][1];
    tri[i].z = triangles[i][2];
  }

  if (axisSymetric)
  {
    vtkYoungsMaterialInterfaceCellCutInternals::Real2 N = { normal[0], normal[1] };
    vtkYoungsMaterialInterfaceCellCutInternals::Real2 V[LOCAL_ARRAY_SIZE(vertexCount)];
    for (int i = 0; i < vertexCount; i++)
    {
      V[i].x = vertices[i][0] - vertices[0][0];
      V[i].y = vertices[i][1] - vertices[0][1];
    }
    vtkYoungsMaterialInterfaceCellCutInternals::Real2 vmin, vmax;
    vtkYoungsMaterialInterfaceCellCutInternals::Real scale;
    vmin = vmax = V[0];
    for (int i = 1; i < vertexCount; i++)
    {
      vmin.x = std::min(V[i].x, vmin.x);
      vmax.x = std::max(V[i].x, vmax.x);
      vmin.y = std::min(V[i].y, vmin.y);
      vmax.y = std::max(V[i].y, vmax.y);
    }
    scale = vmax.x - vmin.x;
    scale = std::max(vmax.y - vmin.y, scale);
    for (int i = 0; i < vertexCount; i++)
      V[i] /= scale;
    double dist0 = vertices[0][0] * normal[0] + vertices[0][1] * normal[1];
    d = dist0 +
      vtkYoungsMaterialInterfaceCellCutInternals::findTriangleSetCuttingCone(
        N, fraction, vertexCount, triangleCount, tri, V) *
        scale;
  }
  else
  {
    vtkYoungsMaterialInterfaceCellCutInternals::Real3 N = { normal[0], normal[1], normal[2] };
    vtkYoungsMaterialInterfaceCellCutInternals::Real3 V[LOCAL_ARRAY_SIZE(vertexCount)];
    for (int i = 0; i < vertexCount; i++)
    {
      V[i].x = vertices[i][0] - vertices[0][0];
      V[i].y = vertices[i][1] - vertices[0][1];
      V[i].z = vertices[i][2] - vertices[0][2];
    }
    vtkYoungsMaterialInterfaceCellCutInternals::Real3 vmin, vmax;
    vtkYoungsMaterialInterfaceCellCutInternals::Real scale;
    vmin = vmax = V[0];
    for (int i = 1; i < vertexCount; i++)
    {
      vmin.x = std::min(V[i].x, vmin.x);
      vmax.x = std::max(V[i].x, vmax.x);
      vmin.y = std::min(V[i].y, vmin.y);
      vmax.y = std::max(V[i].y, vmax.y);
      vmin.z = std::min(V[i].z, vmin.z);
      vmax.z = std::max(V[i].z, vmax.z);
    }
    scale = vmax.x - vmin.x;
    scale = std::max(vmax.y - vmin.y, scale);
    scale = std::max(vmax.z - vmin.z, scale);
    for (int i = 0; i < vertexCount; i++)
      V[i] /= scale;
    double dist0 =
      vertices[0][0] * normal[0] + vertices[0][1] * normal[1] + vertices[0][2] * normal[2];
    d = dist0 +
      vtkYoungsMaterialInterfaceCellCutInternals::findTriangleSetCuttingPlane(
        N, fraction, vertexCount, triangleCount, tri, V) *
        scale;
  }

  return -d;
}
VTK_ABI_NAMESPACE_END
