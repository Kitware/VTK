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
// vtkMultiGroupDataSet

#ifndef __vtkTemporalDataSet_h
#define __vtkTemporalDataSet_h

#include "vtkMultiGroupDataSet.h"

class vtkDataObject;

class VTK_FILTERING_EXPORT vtkTemporalDataSet : public vtkMultiGroupDataSet
{
public:
  static vtkTemporalDataSet *New();

  vtkTypeRevisionMacro(vtkTemporalDataSet,vtkMultiGroupDataSet);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return class name of data type (see vtkType.h for
  // definitions).
  virtual int GetDataObjectType() {return VTK_TEMPORAL_DATA_SET;}

  // Description:
  // Set the number of time steps in theis dataset
  void SetNumberOfTimeSteps(unsigned int numLevels)
    {
      this->SetNumberOfGroups(numLevels);
    }

  // Description:
  // Returns the number of time steps.
  unsigned int GetNumberOfTimeSteps()
    {
      return this->GetNumberOfGroups();
    }

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkTemporalDataSet* GetData(vtkInformation* info);
  static vtkTemporalDataSet* GetData(vtkInformationVector* v, int i=0);
  //ETX

  // Description:
  // The extent type is a 3D extent
  virtual int GetExtentType() { return VTK_TIME_EXTENT; };

protected:
  vtkTemporalDataSet();
  ~vtkTemporalDataSet();

private:
  vtkTemporalDataSet(const vtkTemporalDataSet&);  // Not implemented.
  void operator=(const vtkTemporalDataSet&);  // Not implemented.
};

#endif

