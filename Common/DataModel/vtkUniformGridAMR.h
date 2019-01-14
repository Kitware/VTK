/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// .SECTION Description
// vtkUniformGridAMR is a concrete implementation of
// vtkCompositeDataSet. The dataset type is restricted to
// vtkUniformGrid.


#ifndef vtkUniformGridAMR_h
#define vtkUniformGridAMR_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCompositeDataSet.h"

class vtkCompositeDataIterator;
class vtkUniformGrid;
class vtkAMRInformation;
class vtkAMRDataInternals;

class VTKCOMMONDATAMODEL_EXPORT vtkUniformGridAMR: public vtkCompositeDataSet
{
public:
  static vtkUniformGridAMR *New();
  vtkTypeMacro(vtkUniformGridAMR,vtkCompositeDataSet);

  // Description:
  // Return a new iterator (the iterator has to be deleted by the user).
  VTK_NEWINSTANCE vtkCompositeDataIterator* NewIterator() override;

  // Description:
  // Return class name of data type (see vtkType.h for definitions).
  int GetDataObjectType() override {return VTK_UNIFORM_GRID_AMR;}

  // Description:  // Print internal states
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Restore data object to initial
  void Initialize() override;

  // Description:
  // Initialize the AMR.
  virtual void Initialize(int numLevels, const int * blocksPerLevel);

  // Description:
  // Set/Get the data description of this uniform grid instance,
  // e.g. VTK_XYZ_GRID
  void SetGridDescription(int gridDescription);
  int GetGridDescription();

  // Description:
  // Return the number of levels
  unsigned int GetNumberOfLevels();

  // Description:
  // Return the total number of blocks, including nullptr blocks
  virtual unsigned int GetTotalNumberOfBlocks();

  // Description:
  // Returns the number of datasets at the given level, including null blocks
  unsigned int GetNumberOfDataSets(const unsigned int level);

  // Description:
  // Retrieve the bounds of the AMR domain
  void GetBounds(double bounds[6]);
  const double* GetBounds();
  void GetMin(double min[3]);
  void GetMax(double max[3]);

  // Description:
  // Unhiding superclass method.
  void SetDataSet(vtkCompositeDataIterator* iter,
                  vtkDataObject* dataObj) override;

  // Description:
  // At the passed in level, set grid as the idx'th block at that level.
  // idx must be less than the number of data sets at that level.
  virtual void SetDataSet(unsigned int level, unsigned int idx, vtkUniformGrid *grid);

  // Description:
  // Return the data set pointed to by iter
  vtkDataObject* GetDataSet(vtkCompositeDataIterator* iter) override;

  // Description:
  // Get the data set using the index pair
  vtkUniformGrid* GetDataSet(unsigned int level, unsigned int idx);

  // Description:
  // Retrieves the composite index associated with the data at the given
  // (level,index) pair.
  int GetCompositeIndex( const unsigned int level, const unsigned int index );

  // Description:
  // Givenes the composite Idx (as set by SetCompositeIdx) this method returns the
  // corresponding level and dataset index within the level.
  void GetLevelAndIndex(
      const unsigned int compositeIdx, unsigned int &level, unsigned int &idx );

  // Description:
  // Override ShallowCopy/DeepCopy and CopyStructure
  void ShallowCopy(vtkDataObject *src) override;
  void DeepCopy(vtkDataObject *src) override;
  void CopyStructure(vtkCompositeDataSet *src) override;

  // Retrieve an instance of this class from an information object.
  static vtkUniformGridAMR* GetData(vtkInformation* info);
  static vtkUniformGridAMR* GetData(vtkInformationVector* v, int i=0);

protected:
  vtkUniformGridAMR();
  ~vtkUniformGridAMR() override;

  // Description:
  // Get/Set the meta AMR meta data
  vtkGetObjectMacro(AMRData, vtkAMRDataInternals);

  vtkAMRInformation* AMRInfo;
  vtkAMRDataInternals* AMRData;
  double Bounds[6];

  // Description:
  // Get/Set the meta AMR meta data
  vtkGetObjectMacro(AMRInfo, vtkAMRInformation);
  virtual void SetAMRInfo(vtkAMRInformation*);


private:
  vtkUniformGridAMR(const vtkUniformGridAMR&) = delete;
  void operator=(const vtkUniformGridAMR&) = delete;

  friend class vtkUniformGridAMRDataIterator;
};

#endif
