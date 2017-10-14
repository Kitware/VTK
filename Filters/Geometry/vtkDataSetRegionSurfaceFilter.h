//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
/**
 * @class   vtkDataSetRegionSurfaceFilter
 * @brief   Extract surface of materials.
 *
 * This filter extracts surfaces of materials such that a surface
 * could have a material on each side of it. It also stores a
 * mapping of the original cells and their sides back to the original grid
 * so that we can output boundary information for those cells given
 * only surfaces.
*/

#ifndef vtkDataSetRegionSurfaceFilter_h
#define vtkDataSetRegionSurfaceFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro

#include "vtkDataSetSurfaceFilter.h"

class vtkCharArray;

class VTKFILTERSGEOMETRY_EXPORT vtkDataSetRegionSurfaceFilter : public vtkDataSetSurfaceFilter
{
public:
  static vtkDataSetRegionSurfaceFilter* New();
  vtkTypeMacro(vtkDataSetRegionSurfaceFilter, vtkDataSetSurfaceFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The name of the cell based array that we use to extract interfaces from
   * Default is "Regions"
   */
  vtkSetStringMacro(RegionArrayName);
  vtkGetStringMacro(RegionArrayName);
  //@}

  int UnstructuredGridExecute(vtkDataSet *input,
                                      vtkPolyData *output) override;

  //make it clear we want all the recordOrigCellId signatures from our parent
  using vtkDataSetSurfaceFilter::RecordOrigCellId;

  //override one of the signatures
  void RecordOrigCellId(vtkIdType newIndex, vtkFastGeomQuad *quad) override;

  //@{
  /**
   * Whether to return single sided material interfaces or double sided
   * Default is single
   */
  vtkSetMacro(SingleSided, bool);
  vtkGetMacro(SingleSided, bool);
  //@}

  //@{
  /**
   * The name of the field array that has characteristics of each material
   * Default is "material_properties"
   */
  vtkSetStringMacro(MaterialPropertiesName);
  vtkGetStringMacro(MaterialPropertiesName);
  //@}

  //@{
  /**
   * The name of the field array that has material type identifiers in it
   * Default is "material_ids"
   */
  vtkSetStringMacro(MaterialIDsName);
  vtkGetStringMacro(MaterialIDsName);
  //@}

  //@{
  /**
   * The name of the output field array that records parent materials of each interface
   * Default is "material_ancestors"
   */
  vtkSetStringMacro(MaterialPIDsName);
  vtkGetStringMacro(MaterialPIDsName);
  //@}

  //@{
  /**
   * The name of the field array that has material interface type identifiers in it
   * Default is "interface_ids"
   */
  vtkSetStringMacro(InterfaceIDsName);
  vtkGetStringMacro(InterfaceIDsName);
  //@}

protected:
  vtkDataSetRegionSurfaceFilter();
  ~vtkDataSetRegionSurfaceFilter() override;

  int FillInputPortInformation(int port, vtkInformation *info) override;

  /// Implementation of the algorithm.
  int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) override;

  virtual void InsertQuadInHash(vtkIdType a, vtkIdType b, vtkIdType c,
                                vtkIdType d, vtkIdType sourceId, vtkIdType faceId);
  void InsertQuadInHash(vtkIdType a, vtkIdType b, vtkIdType c,
                                vtkIdType d, vtkIdType sourceId) override
  {
    this->InsertQuadInHash(a,b,c,d,sourceId, -1); //for -Woverloaded-virtual comp warning
  }

  void InsertTriInHash(vtkIdType a, vtkIdType b, vtkIdType c,
                       vtkIdType sourceId, vtkIdType faceId) override;
  virtual void InsertTriInHash(vtkIdType a, vtkIdType b, vtkIdType c,
                       vtkIdType sourceId)
  {
    this->InsertTriInHash(a,b,c,sourceId, -1); //for -Woverloaded-virtual comp warning
  }

  virtual vtkFastGeomQuad *GetNextVisibleQuadFromHash();

private:
  vtkDataSetRegionSurfaceFilter(const vtkDataSetRegionSurfaceFilter&) = delete;
  void operator=(const vtkDataSetRegionSurfaceFilter&) = delete;

  char *RegionArrayName;
  vtkIntArray    *RegionArray;
  vtkIdTypeArray *OrigCellIds;
  vtkCharArray   *CellFaceIds;
  bool SingleSided;
  char *MaterialPropertiesName;
  char *MaterialIDsName;
  char *MaterialPIDsName;
  char *InterfaceIDsName;

  class Internals;
  Internals *Internal;
};

#endif
