// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractCellLinks
 * @brief   an abstract base class for classes that build
 * topological links from points to cells
 *
 * vtkAbstractCellLinks is a family of supplemental objects to vtkCellArray and
 * vtkCellTypes, enabling fast access from points to the cells using the
 * points. vtkAbstractCellLinks is an array of links, each link representing a
 * list of cell ids using a particular point. The information provided by
 * this object can be used to determine neighbors and construct other local
 * topological information.
 *
 * @sa
 * vtkCellLinks vtkStaticCellLinks vtkStaticCellLinksTemplate
 */

#ifndef vtkAbstractCellLinks_h
#define vtkAbstractCellLinks_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDeprecation.h"           // For VTK_DEPRECATED_IN_9_5_0
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkCellArray;
class vtkIdList;

class VTKCOMMONDATAMODEL_EXPORT vtkAbstractCellLinks : public vtkObject
{
public:
  ///@{
  /**
   * Standard type and print methods.
   */
  vtkTypeMacro(vtkAbstractCellLinks, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/Get the points/cells defining this dataset.
   */
  virtual void SetDataSet(vtkDataSet*);
  vtkGetObjectMacro(DataSet, vtkDataSet);
  ///@}

  /**
   * Build the link list array from the input dataset.
   */
  virtual void BuildLinks() = 0;

  /**
   * Release memory and revert to empty state.
   */
  virtual void Initialize() = 0;

  /**
   * Reclaim any unused memory.
   */
  virtual void Squeeze() = 0;

  /**
   * Reset to a state of no entries without freeing the memory.
   */
  virtual void Reset() = 0;

  /**
   * Return the memory in kibibytes (1024 bytes) consumed by this cell links array.
   * Used to support streaming and reading/writing data. The value
   * returned is guaranteed to be greater than or equal to the memory
   * required to actually represent the data represented by this object.
   * The information returned is valid only after the pipeline has
   * been updated.
   */
  virtual unsigned long GetActualMemorySize() = 0;

  /**
   * Standard DeepCopy method.
   *
   * Before you deep copy, make sure to call SetDataSet()
   */
  virtual void DeepCopy(vtkAbstractCellLinks* src) = 0;

  /**
   * Standard ShallowCopy method.
   *
   * Before you shallow copy, make sure to call SetDataSet()
   */
  virtual void ShallowCopy(vtkAbstractCellLinks* src) = 0;

  // Enums for cell links type. Note that the specialized type is
  // set when users do not use ComputeType() and roll their own type.
  enum CellLinksTypes
  {
    LINKS_NOT_DEFINED = 0,
    CELL_LINKS = 1,
    STATIC_CELL_LINKS_USHORT = 2,
    STATIC_CELL_LINKS_UINT = 3,
    STATIC_CELL_LINKS_IDTYPE = 4,
    STATIC_CELL_LINKS_SPECIALIZED = 5
  };

  /**
   * Based on the input (i.e., number of points, number of cells, and length
   * of connectivity array) this helper method returns the integral type to
   * use when instantiating cell link-related classes in order to properly
   * represent the data.  The return value is one of the types
   * defined in the enum CellLinksType enum defined previously. Subclasses
   * may choose to instantiate themselves with different integral types for
   * performance and/or memory reasons. This method is useful when instantiating
   * a vtkStaticCellLinksTemplate; when instantiating a vtkCellLinks the class
   * is hardwired for vtkIdType.
   */
  static int ComputeType(vtkIdType maxPtId, vtkIdType maxCellId, vtkCellArray* ca);
  static int ComputeType(vtkIdType maxPtId, vtkIdType maxCellId, vtkIdType connectivitySize);

  /**
   * Return the type of locator (see enum above).
   */
  vtkGetMacro(Type, int);

  /**
   * These methods are not virtual due to performance concerns. However,
   * subclasses of this class will implement them, using a combination of
   * static_cast<> and templating used to invoke the methods in a way that
   * the compiler can optimize.
   *
   * Get the number of cells using the point specified by ptId:
   *    vtkIdType GetNcells(vtkIdType ptId)
   *
   * Return a list of cell ids using the point.
   *    TIds *GetCells(vtkIdType ptId)
   */

  ///@{
  /**
   * Select all cells with a point degree in the range [minDegree,maxDegree).
   * The degree is the number of cells using a point. The selection is
   * indicated through the provided unsigned char array, with a non-zero
   * value indicates selection. The memory allocated for cellSelection must
   * be the maximum cell id referenced in the links.
   */
  virtual void SelectCells(vtkIdType minMaxDegree[2], unsigned char* cellSelection) = 0;
  ///@}

  ///@{
  /**
   * Force sequential processing (i.e. single thread) of the link building
   * process. By default, sequential processing is off. Note this flag only
   * applies if the class has been compiled with VTK_SMP_IMPLEMENTATION_TYPE
   * set to something other than Sequential. (If set to Sequential, then the
   * filter always runs in serial mode.) This flag is typically used for
   * benchmarking purposes.
   */
  VTK_DEPRECATED_IN_9_5_0("No longer used.")
  vtkSetMacro(SequentialProcessing, bool);
  VTK_DEPRECATED_IN_9_5_0("No longer used.")
  vtkGetMacro(SequentialProcessing, bool);
  VTK_DEPRECATED_IN_9_5_0("No longer used.")
  virtual void SequentialProcessingOn()
  {
    if (!this->SequentialProcessing)
    {
      this->SequentialProcessing = true;
      this->Modified();
    }
  }
  VTK_DEPRECATED_IN_9_5_0("No longer used.")
  virtual void SequentialProcessingOff()
  {
    if (this->SequentialProcessing)
    {
      this->SequentialProcessing = false;
      this->Modified();
    }
  }
  ///@}

  ///@{
  /**
   * Return the time of the last data structure build.
   */
  vtkGetMacro(BuildTime, vtkMTimeType);
  ///@}

  ///@{
  /**
   * Handle the dataset <-> Links loop.
   */
  bool UsesGarbageCollector() const override { return true; }
  ///@}
protected:
  vtkAbstractCellLinks();
  ~vtkAbstractCellLinks() override;

  vtkDataSet* DataSet;
  // VTK_DEPRECATED_IN_9_5_0("No longer used.")
  bool SequentialProcessing; // control whether to thread or not
  int Type;                  // derived classes set this instance variable when constructed

  vtkTimeStamp BuildTime; // time at which links were built

  void ReportReferences(vtkGarbageCollector*) override;

private:
  vtkAbstractCellLinks(const vtkAbstractCellLinks&) = delete;
  void operator=(const vtkAbstractCellLinks&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
