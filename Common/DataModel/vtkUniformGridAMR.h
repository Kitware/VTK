/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkUniformGridAMR.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkUniformGridAMR.h -- Abstract AMR class of uniform grids
//
// .SECTION Description
//  vtkUniformGridAMR is an abstract base class that implements common access
//  operations for AMR data.
//
// .SECTION See Also
// vtkOverlappingAMR vtkNonOverlappingAMR

#ifndef VTKUNIFORMGRIDAMR_H_
#define VTKUNIFORMGRIDAMR_H_

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCompositeDataSet.h"

class vtkUniformGrid;
class vtkTimeStamp;
class vtkDataObject;
class vtkCompositeDataIterator;
class vtkUniformGridAMRDataIterator;
class vtkInformation;
class vtkInformationVector;

class VTKCOMMONDATAMODEL_EXPORT vtkUniformGridAMR : public vtkCompositeDataSet
{
  public:
    vtkTypeMacro(vtkUniformGridAMR,vtkCompositeDataSet);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Description:
    // Return data type (See vtkType.h for definitions)
    virtual int GetDataObjectType() {return VTK_UNIFORM_GRID_AMR; }

    // Description:
    // Sets the number of refinement levels.
    void SetNumberOfLevels(const unsigned int numLevels);

    // Description:
    // Return the number of levels
    unsigned int GetNumberOfLevels();

    // Description:
    // Sets the number of datasets at the given level
    void SetNumberOfDataSets(const unsigned int level, const unsigned int N);

    // Description:
    // Returns the number of datasets at the given level
    unsigned int GetNumberOfDataSets(const unsigned int level);

    // Description:
    // Returns the total number of blocks in the AMR dataset
    unsigned int GetTotalNumberOfBlocks();

    // Description:
    // Return a new iterator (the iterator has to be deleted by the user).
    virtual vtkCompositeDataIterator* NewIterator();

    // Description:
    // Unhiding superclass method
    virtual void SetDataSet(
        vtkCompositeDataIterator* iter, vtkDataObject* dataObj)
      { this->Superclass::SetDataSet(iter, dataObj); };

    // Description:
    // Unhiding superclass method.
    virtual vtkDataObject* GetDataSet(vtkCompositeDataIterator* iter)
      { return this->Superclass::GetDataSet(iter); }

    // Description:
    // Unhiding superclass method.
    virtual vtkInformation* GetMetaData(vtkCompositeDataIterator* iter)
     { return this->Superclass::GetMetaData(iter); }

    // Description:
    // Unhiding superclass method.
    virtual int HasMetaData(vtkCompositeDataIterator* iter)
     { return this->Superclass::HasMetaData(iter); }

    // Description:
    // Get meta-data associated with a level. This may allocate a new
    // vtkInformation object if none is already present. Use HasLevelMetaData to
    // avoid unnecessary allocations.
    virtual vtkInformation* GetLevelMetaData(unsigned int level)
     { return this->GetChildMetaData(level); }

    // Description:
    // Returns if meta-data exists for a given level.
    virtual int HasLevelMetaData(unsigned int level)
     { return this->HasChildMetaData(level); }

    // Description:
    // Get meta-data associated with a dataset.  This may allocate a new
    // vtkInformation object if none is already present. Use HasMetaData to
    // avoid unnecessary allocations.
    virtual vtkInformation* GetMetaData(unsigned int level, unsigned int index);

    // Description:
    // Returns if meta-data exists for a given dataset under a given level.
    virtual int HasMetaData(unsigned int level, unsigned int index);

    // Description:
    // Sets the dataset at the given level and index. If insufficient number
    // of levels or data slots within the level, this method will grow the
    // data-structure accordingly.
    virtual void SetDataSet(
        unsigned int level,unsigned int idx,vtkUniformGrid *grid);

    // Description:
    // Appends the dataset at the given level. Increments the number of datasets
    // within the given level. Further, if an insufficient number of levels the
    // data-structure will grow accordingly.
    virtual void AppendDataSet(unsigned int level, vtkUniformGrid *grid);

    // Description:
    // Returns the dataset stored at the given (level,idx). The user-supplied
    // level and idx must be within the bounds of the data-structure.
    virtual vtkUniformGrid* GetDataSet( unsigned int level, unsigned int idx);

    // Description:
    // Shallow/Deep copy & CopyStructure
    virtual void ShallowCopy(vtkDataObject *src)
     {this->Superclass::ShallowCopy(src);}
    virtual void DeepCopy(vtkDataObject *src)
     {this->Superclass::DeepCopy(src);}
    virtual void CopyStructure(vtkCompositeDataSet* input)
     {this->Superclass::CopyStructure(input);}

    // Description:
    // Retrieve the cached scalar range into the user-supplied buffer.
    void GetScalarRange(double range[2]);
    double* GetScalarRange();

    // Description:
    // Retrieve the bounds of the AMR domain
    void GetBounds(double bounds[6]);
    double* GetBounds();

  protected:
    vtkUniformGridAMR();
    virtual ~vtkUniformGridAMR();

    // Description:
    // Compute the rane of the scalars of the entire datasets and cache it into
    // ScalarRange. This method executes iff the cache is invalidated based on
    // the ScalarRangeComputTime.
    virtual void ComputeScalarRange();

    // Description:
    // Computes the bounds of the AMR dataset.
    virtual void ComputeBounds();

    double ScalarRange[2];
    vtkTimeStamp ScalarRangeComputeTime;

    double Bounds[6];
  private:
    vtkUniformGridAMR(const vtkUniformGridAMR&);//Not implemented
    void operator=(const vtkUniformGridAMR&); // Not implemented
};

#endif /* VTKUNIFORMGRIDAMR_H_ */
