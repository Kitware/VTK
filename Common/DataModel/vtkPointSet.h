/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointSet
 * @brief   abstract class for specifying dataset behavior
 *
 * vtkPointSet is an abstract class that specifies the interface for
 * datasets that explicitly use "point" arrays to represent geometry.
 * For example, vtkPolyData and vtkUnstructuredGrid require point arrays
 * to specify point position, while vtkStructuredPoints generates point
 * positions implicitly.
 *
 * @sa
 * vtkPolyData vtkStructuredGrid vtkUnstructuredGrid
*/

#ifndef vtkPointSet_h
#define vtkPointSet_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSet.h"

#include "vtkPoints.h" // Needed for inline methods

class vtkPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkPointSet : public vtkDataSet
{
public:
  vtkTypeMacro(vtkPointSet,vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Reset to an empty state and free any memory.
   */
  void Initialize() VTK_OVERRIDE;

  /**
   * Copy the geometric structure of an input point set object.
   */
  void CopyStructure(vtkDataSet *pd) VTK_OVERRIDE;

  //@{
  /**
   * See vtkDataSet for additional information.
   */
  vtkIdType GetNumberOfPoints() VTK_OVERRIDE;
  void GetPoint(vtkIdType ptId, double x[3]) VTK_OVERRIDE
    {this->Points->GetPoint(ptId,x);};
  vtkIdType FindPoint(double x[3]) VTK_OVERRIDE;
  vtkIdType FindPoint(double x, double y, double z) {
    return this->vtkDataSet::FindPoint(x, y, z);};
  vtkIdType FindCell(double x[3], vtkCell *cell, vtkIdType cellId,
                             double tol2, int& subId, double pcoords[3],
                             double *weights) VTK_OVERRIDE;
  vtkIdType FindCell(double x[3], vtkCell *cell,
                             vtkGenericCell *gencell, vtkIdType cellId,
                             double tol2, int& subId, double pcoords[3],
                             double *weights) VTK_OVERRIDE;
  //@}

  /**
   * See vtkDataSet for additional information.
   * WARNING: Just don't use this error-prone method, the returned pointer
   * and its values are only valid as long as another method invocation is not
   * performed. Prefer GetPoint() with the return value in argument.
   */
  double *GetPoint(vtkIdType ptId) VTK_OVERRIDE
    {return this->Points->GetPoint(ptId);};

  /**
   * Return an iterator that traverses the cells in this data set.
   */
  vtkCellIterator* NewCellIterator() VTK_OVERRIDE;

  /**
   * Get MTime which also considers its vtkPoints MTime.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  /**
   * Compute the (X, Y, Z)  bounds of the data.
   */
  void ComputeBounds() VTK_OVERRIDE;

  /**
   * Reclaim any unused memory.
   */
  void Squeeze() VTK_OVERRIDE;

  //@{
  /**
   * Specify point array to define point coordinates.
   */
  virtual void SetPoints(vtkPoints*);
  vtkGetObjectMacro(Points,vtkPoints);
  //@}

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value). THIS METHOD
   * IS THREAD SAFE.
   */
  unsigned long GetActualMemorySize() VTK_OVERRIDE;

  //@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkDataObject *src) VTK_OVERRIDE;
  void DeepCopy(vtkDataObject *src) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Overwritten to handle the data/locator loop
   */
  void Register(vtkObjectBase* o) VTK_OVERRIDE;
  void UnRegister(vtkObjectBase* o) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkPointSet* GetData(vtkInformation* info);
  static vtkPointSet* GetData(vtkInformationVector* v, int i=0);
  //@}

protected:
  vtkPointSet();
  ~vtkPointSet() VTK_OVERRIDE;

  vtkPoints *Points;
  vtkPointLocator *Locator;

  void ReportReferences(vtkGarbageCollector*) VTK_OVERRIDE;
private:

  void Cleanup();

  vtkPointSet(const vtkPointSet&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPointSet&) VTK_DELETE_FUNCTION;
};

inline vtkIdType vtkPointSet::GetNumberOfPoints()
{
  if (this->Points)
  {
    return this->Points->GetNumberOfPoints();
  }
  else
  {
    return 0;
  }
}


#endif


