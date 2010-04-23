/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalDataSet - Composite dataset that holds multiple times
// .SECTION Description
// vtkTemporalDataSet is a vtkCompositeDataSet that stores
// multiple time steps of data. 
// .SECTION See Also
// vtkCompositeDataSet

#ifndef __vtkTemporalDataSet_h
#define __vtkTemporalDataSet_h

#include "vtkCompositeDataSet.h"

class vtkDataObject;

class VTK_FILTERING_EXPORT vtkTemporalDataSet : public vtkCompositeDataSet
{
public:
  static vtkTemporalDataSet *New();

  vtkTypeMacro(vtkTemporalDataSet,vtkCompositeDataSet);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return class name of data type (see vtkType.h for
  // definitions).
  virtual int GetDataObjectType() {return VTK_TEMPORAL_DATA_SET;}

  // Description:
  // Set the number of time steps in theis dataset
  void SetNumberOfTimeSteps(unsigned int numLevels)
    {
      this->SetNumberOfChildren(numLevels);
    }

  // Description:
  // Returns the number of time steps.
  unsigned int GetNumberOfTimeSteps()
    {
      return this->GetNumberOfChildren();
    }

  // Description:
  // Set a data object as a timestep. Cannot be vtkTemporalDataSet.
  void SetTimeStep(unsigned int timestep, vtkDataObject* dobj);

  // Description:
  // Get a timestep.
  vtkDataObject* GetTimeStep(unsigned int timestep)
    { return this->GetChild(timestep); }

  // Description:
  // Get timestep meta-data.
  vtkInformation* GetMetaData(unsigned int timestep)
    { return this->Superclass::GetChildMetaData(timestep); }

  // Description:
  // Returns if timestep meta-data is present.
  int HasMetaData(unsigned int timestep)
    { return this->Superclass::HasChildMetaData(timestep); }

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkTemporalDataSet* GetData(vtkInformation* info);
  static vtkTemporalDataSet* GetData(vtkInformationVector* v, int i=0);
  //ETX

  // Description:
  // The extent type is a 3D extent
  virtual int GetExtentType() { return VTK_TIME_EXTENT; };

  // Description:
  // Unhiding superclass method.
  virtual vtkInformation* GetMetaData(vtkCompositeDataIterator* iter)
    { return this->Superclass::GetMetaData(iter); }


  // Description:
  // Unhiding superclass method.
  virtual int HasMetaData(vtkCompositeDataIterator* iter)
    { return this->Superclass::HasMetaData(iter); }

protected:
  vtkTemporalDataSet();
  ~vtkTemporalDataSet();

private:
  vtkTemporalDataSet(const vtkTemporalDataSet&);  // Not implemented.
  void operator=(const vtkTemporalDataSet&);  // Not implemented.
};

#endif

