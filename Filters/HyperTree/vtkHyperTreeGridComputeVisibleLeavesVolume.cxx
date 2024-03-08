/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGenerateMaskLeavesCells.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridComputeVisibleLeavesVolume.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayMeta.h" // for GetAPIType
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkGenericDataArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkImplicitArray.h"
#include "vtkIndent.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkType.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm> // for max
#include <memory>
#include <set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridComputeVisibleLeavesVolume)

  namespace
{
  /**
   * Implicit array implementation unpacking a bool array to an unsigned char array,
   * reducing the memory footprint of the array by a factor of 8,
   * while still guaranteeing fast element access using implicit arrays static dispatch.
   */
  template <typename ValueType>
  struct vtkScalarBooleanImplicitBackend final
  {
    /**
     * Build the implicit array using a bit vector to be unpacked.
     *
     * @param values Lookup vector to use
     */
    vtkScalarBooleanImplicitBackend(const std::vector<bool>& values)
      : Values(values)
    {
    }

    /**
     * Templated method called for element access
     *
     * @param _index: Array element id
     * \return Array element in the templated type
     */
    ValueType operator()(const int _index) const { return static_cast<ValueType>(Values[_index]); }

    const std::vector<bool> Values;
  };

  template <typename T>
  using vtkScalarBooleanArray = vtkImplicitArray<vtkScalarBooleanImplicitBackend<T>>;
}

//------------------------------------------------------------------------------
class vtkHyperTreeGridComputeVisibleLeavesVolume::vtkInternal
{
public:
  vtkInternal() = default;
  ~vtkInternal() = default;

  //------------------------------------------------------------------------------------------------
  /**
   * @brief Initialize.
   *
   * Cette methode dimensionne le tableau de boolean en le reinitialisant à 0.
   * D'autres informations relatives à la source sont mise à jour comme
   * le nombre de cellules filles lors d'un raffinement, l'existence d'un
   * masquage sur les cellules ou de la définition de cellules fantômes.
   *
   * @param _input: input mesh
   */
  void Initialize(vtkHyperTreeGrid* inputHTG)
  {
    this->packedValidCellArray.clear();
    this->packedValidCellArray.resize(inputHTG->GetNumberOfCells(), 0);

    this->m_with_discretes_values = true;
    this->m_discretes_values.clear();
    this->m_volume_double.clear();
    this->m_volume_double.resize(inputHTG->GetNumberOfCells(), 0);

    this->m_number_of_children = inputHTG->GetNumberOfChildren();
    this->maskArray = inputHTG->HasMask() ? inputHTG->GetMask() : nullptr;
    this->ghostArray = inputHTG->GetGhostCells();
  }

  //------------------------------------------------------------------------------------------------
  /**
   * @brief GetAndFinalizeValidMaskArray.
   *
   * Cette methode construit le tableau de boolean suivant VTK décrivant si
   * une cellule est valide ou non.
   *
   * @return le tableau VTK décrivant un masque sur les cellules valides (valeur
   * True) face aux cellules non valides (valeur False, les cellules coarses,
   * masquées ou fantômes).
   */
  vtkDataArray* GetAndFinalizeValidMaskArray()
  {
    this->m_vtkvalidcell->ConstructBackend(this->packedValidCellArray);
    this->m_vtkvalidcell->SetName("vtkValidCell");
    this->m_vtkvalidcell->SetNumberOfComponents(1);
    this->m_vtkvalidcell->SetNumberOfTuples(this->packedValidCellArray.size());
    for (vtkIdType iCell = 0; iCell < (vtkIdType)(this->packedValidCellArray.size()); ++iCell)
    {
      this->m_vtkvalidcell->SetTuple1(iCell, this->packedValidCellArray[iCell]);
    }
    this->packedValidCellArray.clear();
    return this->m_vtkvalidcell;
  }

  //------------------------------------------------------------------------------------------------
  /**
   * @brief GetAndFinalizeVolumArray.
   *
   * Cette methode construit le tableau de double suivant VTK décrivant un champ
   * Volume.
   *
   * @return le tableau VTK décrivant un volume.
   */
  vtkDataArray* GetAndFinalizeVolumArray()
  {
    // généralement les valeurs prises par le volume des cellules décrit un
    // ensemble de valeurs discrétes :
    // - dans le cas uniforme, une valeur de volume par niveau ;
    // - dans le cas générale, une valeur de volume par niveau par nombre de
    // cellules par axe.
    if (!this->m_with_discretes_values)
    {
      // Implicit array by discrete double values
      // return ... ;
    }
    // Classic array by double
    this->m_volume->SetName("vtkVolume");
    this->m_volume->SetNumberOfComponents(1);
    this->m_volume->SetNumberOfTuples(this->m_volume_double.size());
    for (vtkIdType iCell = 0; iCell < (vtkIdType)(this->m_volume_double.size()); ++iCell)
    {
      this->m_volume->SetTuple1(iCell, this->m_volume_double[iCell]);
    }
    this->m_with_discretes_values = true;
    this->m_discretes_values.clear();
    this->m_volume_double.clear();
    return this->m_volume;
  }

  //------------------------------------------------------------------------------------------------
  /**
   * @brief ComputeVolume.
   *
   * Cette methode calcule le volume de la cellule pointé par le cursor.
   *
   * @param _idC: index de la cellule
   */
  void ComputeVolume(vtkHyperTreeGridNonOrientedGeometryCursor* _cursor)
  {
    vtkIdType idC = _cursor->GetGlobalNodeIndex();
    bool checkVolume = false;
    double cellVolume{ 1 };
    double* ptr{ _cursor->GetSize() };
    for (unsigned int iDim = 0; iDim < 3; ++iDim, ++ptr)
    {
      if (*ptr != 0)
      {
        cellVolume *= *ptr;
        checkVolume = true;
      }
    }
    if (!checkVolume)
    {
      cellVolume = 0;
    }
    if (this->m_with_discretes_values)
    {
      this->m_discretes_values.insert(cellVolume);
      if (this->m_discretes_values.size() > 256)
      {
        this->m_with_discretes_values = false;
        this->m_discretes_values.clear();
      }
    }
    this->m_volume_double[idC] = cellVolume;
  }

  //------------------------------------------------------------------------------------------------
  /**
   * @brief SetValidMaskArray.
   *
   * Cette methode positionne la valeur du masque à true à l'index passé en
   * paramètre.
   *
   * @param _idC: index de la cellule
   */
  void SetValidMaskArray(const vtkIdType _idC) { this->packedValidCellArray[_idC] = true; }

  //------------------------------------------------------------------------------------------------
  /**
   * @brief GetNumberOfChildren.
   *
   * Cette methode permet de récupère le nombre de cellules filles (more fine)
   * lors du raffinement d'une cellule (mère, coarse).
   *
   * @return the value
   */
  unsigned int GetNumberOfChildren() const { return this->m_number_of_children; }

  //------------------------------------------------------------------------------------------------
  /**
   * @brief GetInputIsValidCell.
   *
   * Cette methode permet de récupère si une cellule est valide (c'est une
   * cellule feuille, non masquée et non fantome) par rapport aux informations
   * recuperees au niveau de l'input. ATTENTION La notion de cellule feuille est
   * relatif à l'application ou non du filtre vtkDepthLimiter.
   *
   * @param _idC: index de la cellule
   * @return the value
   */
  bool GetInputIsValidCell(const vtkIdType _idC) const
  {
    // This cell is valid:
    // - which means not masked;
    if (this->maskArray != nullptr && this->maskArray->GetTuple1(_idC) != 0)
    {
      return false;
    }
    // - which means not ghosted.
    if (this->ghostArray != nullptr && this->ghostArray->GetTuple1(_idC) != 0)
    {
      return false;
    }
    return true;
  }

private:
  // Data input
  unsigned int m_number_of_children{ 0 };
  vtkBitArray* maskArray = nullptr;
  vtkUnsignedCharArray* ghostArray = nullptr;

  // Data intermediate
  std::vector<bool> packedValidCellArray;

  bool m_with_discretes_values{ true };
  std::set<double> m_discretes_values;
  std::vector<double> m_volume_double;
  // Data output
  using SourceT = vtk::GetAPIType<vtkDoubleArray>;
  vtkNew<::vtkScalarBooleanArray<SourceT>> m_vtkvalidcell;

  vtkNew<vtkDoubleArray> m_volume;
};

//------------------------------------------------------------------------------
vtkHyperTreeGridComputeVisibleLeavesVolume::vtkHyperTreeGridComputeVisibleLeavesVolume()
{
  this->Internal = std::unique_ptr<vtkInternal>(new vtkInternal());
};

//------------------------------------------------------------------------------
void vtkHyperTreeGridComputeVisibleLeavesVolume::PrintSelf(ostream& ost, vtkIndent indent)
{
  this->Superclass::PrintSelf(ost, indent);
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridComputeVisibleLeavesVolume::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridComputeVisibleLeavesVolume::ProcessTrees(
  vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to hypertree grid
  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (outputHTG == nullptr)
  {
    vtkErrorMacro(
      "Incorrect type of output: " << outputDO->GetClassName() << ". Expected vtkHyperTreeGrid");
    return 0;
  }

  outputHTG->ShallowCopy(input);

  this->Internal->Initialize(input);

  // Iterate over all input and output hyper trees
  vtkIdType index = -1;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator iterator;
  outputHTG->InitializeTreeIterator(iterator);
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> outCursor;
  while (iterator.GetNextTree(index))
  {
    if (this->CheckAbort())
    {
      break;
    }

    // Place cursor at root of current output tree
    outputHTG->InitializeNonOrientedGeometryCursor(outCursor, index);
    this->ProcessNode(outCursor);
  }

  // Append both volume and cell validity array to the output
  outputHTG->GetCellData()->AddArray(this->Internal->GetAndFinalizeValidMaskArray());
  outputHTG->GetCellData()->AddArray(this->Internal->GetAndFinalizeVolumArray());

  this->UpdateProgress(1.);
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridComputeVisibleLeavesVolume::ProcessNode(
  vtkHyperTreeGridNonOrientedGeometryCursor* outCursor)
{
  vtkIdType currentId = outCursor->GetGlobalNodeIndex();
  this->Internal->ComputeVolume(outCursor);

  if (outCursor->IsLeaf())
  {
    if (this->Internal->GetInputIsValidCell(currentId))
    {
      this->Internal->SetValidMaskArray(currentId);
    }
    return;
  }

  for (unsigned int childId = 0; childId < this->Internal->GetNumberOfChildren(); ++childId)
  {
    outCursor->ToChild(childId);
    this->ProcessNode(outCursor);
    outCursor->ToParent();
  }
}

VTK_ABI_NAMESPACE_END
