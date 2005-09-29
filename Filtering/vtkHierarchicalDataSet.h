/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataSet - abstract superclass for hierarchical datasets
// .SECTION Description
// vtkHierarchicalDataSet is a vtkCompositeDataSet that stores
// a hierarchy of datasets. The dataset collection consists of
// multiple levels. Each dataset can have an arbitrary number of
// parents and children at levels above and below. The levels are
// implemented as groups (see vtkMultiGroupDataSet) and can be
// treated as such. This allows the use of all vtkMultiGroupDataSet.
// Currently, the interface for connecting parents-children is incomplete.
// .SECTION See Also
// vtkMultiGroupDataSet

#ifndef __vtkHierarchicalDataSet_h
#define __vtkHierarchicalDataSet_h

#include "vtkMultiGroupDataSet.h"

class vtkDataObject;
class vtkHierarchicalDataInformation;

class VTK_FILTERING_EXPORT vtkHierarchicalDataSet : public vtkMultiGroupDataSet
{
public:
  static vtkHierarchicalDataSet *New();

  vtkTypeRevisionMacro(vtkHierarchicalDataSet,vtkMultiGroupDataSet);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return class name of data type (see vtkType.h for
  // definitions).
  virtual int GetDataObjectType() {return VTK_HIERARCHICAL_DATA_SET;}

  // Description:
  // Set the number of refinement levels. This call might cause
  // allocation if the new number of levels is larger than the
  // current one.
  void SetNumberOfLevels(unsigned int numLevels)
    {
      this->SetNumberOfGroups(numLevels);
    }

  // Description:
  // Returns the number of levels.
  unsigned int GetNumberOfLevels()
    {
      return this->GetNumberOfGroups();
    }

  // Description:
  // Uses keys LEVEL() and INDEX() to call SetDataSet(LEVEL, INDEX, dobj)
  virtual void AddDataSet(vtkInformation* index, vtkDataObject* dobj);

  // Description:
  // Uses keys LEVEL() and INDEX() to call GetDataSet(LEVEL, INDEX)
  virtual vtkDataObject* GetDataSet(vtkInformation* index);

  vtkDataObject* GetDataSet(unsigned int level, unsigned int id)
    { return this->Superclass::GetDataSet(level, id); }

  // Description:
  // Legacy method. Use GetMultiGroupDataInformation() instead.
  vtkHierarchicalDataInformation* GetHierarchicalDataInformation();

  // Description:
  // Legacy method. Use SetMultiGroupDataInformation() instead.
  void SetHierarchicalDataInformation(vtkHierarchicalDataInformation* info);

  static vtkInformationIntegerKey* LEVEL();

protected:
  vtkHierarchicalDataSet();
  ~vtkHierarchicalDataSet();

private:
  vtkHierarchicalDataSet(const vtkHierarchicalDataSet&);  // Not implemented.
  void operator=(const vtkHierarchicalDataSet&);  // Not implemented.
};

#endif

