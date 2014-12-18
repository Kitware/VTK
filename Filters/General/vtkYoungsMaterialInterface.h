/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkYoungsMaterialInterface.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkYoungsMaterialInterface - reconstructs material interfaces
//
// .SECTION Description
// Reconstructs material interfaces from a mesh containing mixed cells (where several materials are mixed)
// this implementation is based on the youngs algorithm, generalized to arbitrary cell types and works
// on both 2D and 3D meshes. the main advantage of the youngs algorithm is it guarantees the material volume correctness.
// for 2D meshes, the AxisSymetric flag allows to switch between a pure 2D (planar) algorithm and an axis symetric 2D algorithm
// handling volumes of revolution.
//
// .SECTION Thanks
// This file is part of the generalized Youngs material interface reconstruction algorithm contributed by <br>
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>
// Implementation by Thierry Carrard (thierry.carrard@cea.fr)
// Modification by Philippe Pebay (philippe.pebay@kitware.com)

#ifndef vtkYoungsMaterialInterface_h
#define vtkYoungsMaterialInterface_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

#include "vtkSmartPointer.h" // For SP ivars

class vtkIntArray;
class vtkInformation;
class vtkInformationVector;
class vtkYoungsMaterialInterfaceInternals;

class VTKFILTERSGENERAL_EXPORT vtkYoungsMaterialInterface : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkYoungsMaterialInterface* New();
  vtkTypeMacro(vtkYoungsMaterialInterface,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get wether the normal vector has to be flipped.
  vtkSetMacro(InverseNormal,int);
  vtkGetMacro(InverseNormal,int);
  vtkBooleanMacro(InverseNormal,int);

  // Description:
  // If this flag is on, material order in reversed.
  // Otherwise, materials are sorted in ascending order depending on the given ordering array.
  vtkSetMacro(ReverseMaterialOrder,int);
  vtkGetMacro(ReverseMaterialOrder,int);
  vtkBooleanMacro(ReverseMaterialOrder,int);

  // Description:
  // Set/Get OnionPeel flag. if this flag is on, the normal vector of the first
  // material (which depends on material ordering) is used for all materials.
  vtkSetMacro(OnionPeel,int);
  vtkGetMacro(OnionPeel,int);
  vtkBooleanMacro(OnionPeel,int);

  // Description:
  // Turns on/off AxisSymetric computation of 2D interfaces.
  // in axis symetric mode, 2D meshes are understood as volumes of revolution.
  vtkSetMacro(AxisSymetric,int);
  vtkGetMacro(AxisSymetric,int);
  vtkBooleanMacro(AxisSymetric,int);

  // Description:
  // when UseFractionAsDistance is true, the volume fraction is interpreted as the distance
  // of the cutting plane from the origin.
  // in axis symetric mode, 2D meshes are understood as volumes of revolution.
  vtkSetMacro(UseFractionAsDistance,int);
  vtkGetMacro(UseFractionAsDistance,int);
  vtkBooleanMacro(UseFractionAsDistance,int);

  // Description:
  // When FillMaterial is set to 1, the volume containing material is output and not only the interface surface.
  vtkSetMacro(FillMaterial,int);
  vtkGetMacro(FillMaterial,int);
  vtkBooleanMacro(FillMaterial,int);

  // Description:
  // Set/Get minimum and maximum volume fraction value. if a material fills a volume above the minimum value, the material is considered to be void. if a material fills a volume fraction beyond the maximum value it is considered as filling the whole volume.
  vtkSetVector2Macro(VolumeFractionRange,double);
  vtkGetVectorMacro(VolumeFractionRange,double,2);

  // Description:
  // Sets/Gets the number of materials.
  virtual void SetNumberOfMaterials(int n);
  virtual int GetNumberOfMaterials();

  // Description:
  // Set/Get whether all material blocks should be used, irrespective of the material block mapping.
  vtkSetMacro(UseAllBlocks,bool);
  vtkGetMacro(UseAllBlocks,bool);
  vtkBooleanMacro(UseAllBlocks,bool);

  // Description:
  // Only meaningfull for LOVE software. returns the maximum number of blocks conatining the same material
  vtkGetMacro(NumberOfDomains,int);

  // Description:
  // Set ith Material arrays to be used as volume fraction, interface normal and material ordering. Each parameter name a cell array.
  virtual void SetMaterialArrays( int i, const char* volume, const char* normalX, const char* normalY, const char* normalZ, const char* ordering );
  virtual void SetMaterialArrays( int i, const char* volume, const char* normal, const char* ordering );
  virtual void SetMaterialVolumeFractionArray( int i, const char* volume );
  virtual void SetMaterialNormalArray( int i, const char* normal );
  virtual void SetMaterialOrderingArray( int i, const char* ordering );

  // Description:
  // Removes all meterials previously added.
  virtual void RemoveAllMaterials();

  // Description:
  // Alternative API for associating Normal and Ordering arrays to materials
  // identified by its volume-fraction array.
  // Note that these mappings are cleared by a call to RemoveAllMaterials() but
  // not by SetNumberOfMaterials().
  // If one uses the SetMaterial*Array(int, ...) API to set the normal or
  // ordering arrays, then that supersedes the values set using this API.
  virtual void SetMaterialNormalArray(const char* volume, const char* normal);
  virtual void SetMaterialOrderingArray(const char* volume, const char* ordering);

  // Description:
  // select blocks to be processed for each described material.
  virtual void RemoveAllMaterialBlockMappings();
  virtual void AddMaterialBlockMapping(int b);


//BTX
  enum
  {
    MAX_CELL_POINTS=256
  };
//ETX

protected:
  vtkYoungsMaterialInterface ();
  virtual ~vtkYoungsMaterialInterface ();

  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int RequestData(vtkInformation *request,
             vtkInformationVector **inputVector,
             vtkInformationVector *outputVector);

  // Description:
  // Serial implementation of the material aggregation.
  virtual void Aggregate ( int, int* );

  void UpdateBlockMapping();

  int CellProduceInterface( int dim, int np, double fraction, double minFrac, double maxFrac );

  // Description:
  // Read-Write Properties
  int FillMaterial;
  int InverseNormal;
  int AxisSymetric;
  int OnionPeel;
  int ReverseMaterialOrder;
  int UseFractionAsDistance;
  double VolumeFractionRange[2];
//BTX
  vtkSmartPointer<vtkIntArray> MaterialBlockMapping;
//ETX
  bool UseAllBlocks;

  // Description:
  // Read only properties
  int NumberOfDomains;

  // Desctiption:
  // Internal data structures
  vtkYoungsMaterialInterfaceInternals* Internals;

private:
  vtkYoungsMaterialInterface(const vtkYoungsMaterialInterface&); // Not implemented
  void operator=(const vtkYoungsMaterialInterface&); // Not implemented
};

#endif /* VTK_YOUNGS_MATERIAL_INTERFACE_H */

