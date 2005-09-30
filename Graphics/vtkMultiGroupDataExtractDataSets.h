/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataExtractDataSets.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataExtractDataSets - extract a number of datasets
// .SECTION Description
// vtkMultiGroupDataExtractDataSets extracts the user specified list
// of datasets from a multi-group dataset.

#ifndef __vtkMultiGroupDataExtractDataSets_h
#define __vtkMultiGroupDataExtractDataSets_h

#include "vtkMultiGroupDataSetAlgorithm.h"

//BTX
struct vtkMultiGroupDataExtractDataSetsInternals;
//ETX

class VTK_GRAPHICS_EXPORT vtkMultiGroupDataExtractDataSets : public vtkMultiGroupDataSetAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkMultiGroupDataExtractDataSets,vtkMultiGroupDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkMultiGroupDataExtractDataSets *New();

  // Description:
  // Add a dataset to be extracted.
  void AddDataSet(unsigned int group, unsigned int idx);

  // Description:
  // Remove all entries from the list of datasets to be extracted.
  void ClearDataSetList();

protected:
  vtkMultiGroupDataExtractDataSets();
  ~vtkMultiGroupDataExtractDataSets();

  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);
  virtual int RequestInformation(vtkInformation *, 
                                 vtkInformationVector **, 
                                 vtkInformationVector *);
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

  unsigned int ComputeOutputGroups(unsigned int inputNumGroups);

//BTX
  friend struct vtkMultiGroupDataExtractDataSetsInternals;

  struct DataSetNode
  {
    unsigned int Group;
    unsigned int DataSetId;
    int Initialized;
    
    DataSetNode() : Initialized(0) {}
    DataSetNode(unsigned int group, unsigned int dsid) : 
      Group(group), DataSetId(dsid), Initialized(1)  {}
  };
//ETX

  unsigned int MinGroup;

private:
  vtkMultiGroupDataExtractDataSetsInternals* Internal;

  vtkMultiGroupDataExtractDataSets(const vtkMultiGroupDataExtractDataSets&);  // Not implemented.
  void operator=(const vtkMultiGroupDataExtractDataSets&);  // Not implemented.
};

#endif


