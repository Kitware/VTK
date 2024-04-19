// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkYoungsMaterialInterface
 * @brief   reconstructs material interfaces
 *
 *
 * Reconstructs material interfaces from a mesh containing mixed cells (where several materials are
 * mixed) this implementation is based on the youngs algorithm, generalized to arbitrary cell types
 * and works on both 2D and 3D meshes. the main advantage of the youngs algorithm is it guarantees
 * the material volume correctness. for 2D meshes, the AxisSymetric flag allows to switch between a
 * pure 2D (planar) algorithm and an axis symmetric 2D algorithm handling volumes of revolution.
 *
 * @par Thanks:
 * This file is part of the generalized Youngs material interface reconstruction algorithm
 * contributed by <br> CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
 * BP12, F-91297 Arpajon, France. <br>
 * Implementation by Thierry Carrard (thierry.carrard@cea.fr)
 * Modification by Philippe Pebay (philippe.pebay@kitware.com)
 */

#ifndef vtkYoungsMaterialInterface_h
#define vtkYoungsMaterialInterface_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

#include "vtkSmartPointer.h" // For SP ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkIntArray;
class vtkInformation;
class vtkInformationVector;
class vtkYoungsMaterialInterfaceInternals;

class VTKFILTERSGENERAL_EXPORT vtkYoungsMaterialInterface : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkYoungsMaterialInterface* New();
  vtkTypeMacro(vtkYoungsMaterialInterface, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get whether the normal vector has to be flipped.
   */
  vtkSetMacro(InverseNormal, vtkTypeBool);
  vtkGetMacro(InverseNormal, vtkTypeBool);
  vtkBooleanMacro(InverseNormal, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If this flag is on, material order in reversed.
   * Otherwise, materials are sorted in ascending order depending on the given ordering array.
   */
  vtkSetMacro(ReverseMaterialOrder, vtkTypeBool);
  vtkGetMacro(ReverseMaterialOrder, vtkTypeBool);
  vtkBooleanMacro(ReverseMaterialOrder, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get OnionPeel flag. if this flag is on, the normal vector of the first
   * material (which depends on material ordering) is used for all materials.
   */
  vtkSetMacro(OnionPeel, vtkTypeBool);
  vtkGetMacro(OnionPeel, vtkTypeBool);
  vtkBooleanMacro(OnionPeel, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turns on/off AxisSymetric computation of 2D interfaces.
   * in axis symmetric mode, 2D meshes are understood as volumes of revolution.
   */
  vtkSetMacro(AxisSymetric, vtkTypeBool);
  vtkGetMacro(AxisSymetric, vtkTypeBool);
  vtkBooleanMacro(AxisSymetric, vtkTypeBool);
  ///@}

  ///@{
  /**
   * when UseFractionAsDistance is true, the volume fraction is interpreted as the distance
   * of the cutting plane from the origin.
   * in axis symmetric mode, 2D meshes are understood as volumes of revolution.
   */
  vtkSetMacro(UseFractionAsDistance, vtkTypeBool);
  vtkGetMacro(UseFractionAsDistance, vtkTypeBool);
  vtkBooleanMacro(UseFractionAsDistance, vtkTypeBool);
  ///@}

  ///@{
  /**
   * When FillMaterial is set to 1, the volume containing material is output and not only the
   * interface surface.
   */
  vtkSetMacro(FillMaterial, vtkTypeBool);
  vtkGetMacro(FillMaterial, vtkTypeBool);
  vtkBooleanMacro(FillMaterial, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get minimum and maximum volume fraction value. if a material fills a volume above the
   * minimum value, the material is considered to be void. if a material fills a volume fraction
   * beyond the maximum value it is considered as filling the whole volume.
   */
  vtkSetVector2Macro(VolumeFractionRange, double);
  vtkGetVectorMacro(VolumeFractionRange, double, 2);
  ///@}

  ///@{
  /**
   * Sets/Gets the number of materials.
   */
  virtual void SetNumberOfMaterials(int n);
  virtual int GetNumberOfMaterials();
  ///@}

  ///@{
  /**
   * Set/Get whether all material blocks should be used, irrespective of the material block mapping.
   */
  vtkSetMacro(UseAllBlocks, bool);
  vtkGetMacro(UseAllBlocks, bool);
  vtkBooleanMacro(UseAllBlocks, bool);
  ///@}

  ///@{
  /**
   * Only meaningful for LOVE software. returns the maximum number of blocks containing the same
   * material
   */
  vtkGetMacro(NumberOfDomains, int);
  ///@}

  ///@{
  /**
   * Set ith Material arrays to be used as volume fraction, interface normal and material ordering.
   * Each parameter name a cell array.
   */
  virtual void SetMaterialArrays(int i, const char* volume, const char* normalX,
    const char* normalY, const char* normalZ, const char* ordering);
  virtual void SetMaterialArrays(
    int i, const char* volume, const char* normal, const char* ordering);
  virtual void SetMaterialVolumeFractionArray(int i, const char* volume);
  virtual void SetMaterialNormalArray(int i, const char* normal);
  virtual void SetMaterialOrderingArray(int i, const char* ordering);
  ///@}

  /**
   * Removes all materials previously added.
   */
  virtual void RemoveAllMaterials();

  ///@{
  /**
   * Alternative API for associating Normal and Ordering arrays to materials
   * identified by its volume-fraction array.
   * Note that these mappings are cleared by a call to RemoveAllMaterials() but
   * not by SetNumberOfMaterials().
   * If one uses the SetMaterial*Array(int, ...) API to set the normal or
   * ordering arrays, then that supersedes the values set using this API.
   */
  virtual void SetMaterialNormalArray(const char* volume, const char* normal);
  virtual void SetMaterialOrderingArray(const char* volume, const char* ordering);
  ///@}

  ///@{
  /**
   * select blocks to be processed for each described material.
   */
  virtual void RemoveAllMaterialBlockMappings();
  virtual void AddMaterialBlockMapping(int b);
  ///@}

  enum
  {
    MAX_CELL_POINTS = 256
  };

protected:
  vtkYoungsMaterialInterface();
  ~vtkYoungsMaterialInterface() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Serial implementation of the material aggregation.
   */
  virtual void Aggregate(int, int*);

  void UpdateBlockMapping();

  int CellProduceInterface(int dim, int np, double fraction, double minFrac, double maxFrac);

  ///@{
  /**
   * Read-Write Properties
   */
  vtkTypeBool FillMaterial;
  vtkTypeBool InverseNormal;
  vtkTypeBool AxisSymetric;
  vtkTypeBool OnionPeel;
  vtkTypeBool ReverseMaterialOrder;
  vtkTypeBool UseFractionAsDistance;
  double VolumeFractionRange[2];
  ///@}

  vtkSmartPointer<vtkIntArray> MaterialBlockMapping;

  bool UseAllBlocks;

  /**
   * Read only properties
   */
  int NumberOfDomains;

  // Description:
  // Internal data structures
  vtkYoungsMaterialInterfaceInternals* Internals;

private:
  vtkYoungsMaterialInterface(const vtkYoungsMaterialInterface&) = delete;
  void operator=(const vtkYoungsMaterialInterface&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* VTK_YOUNGS_MATERIAL_INTERFACE_H */
