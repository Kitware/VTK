/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataExtractDataSets.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataExtractDataSets - extract a number of datasets
// .SECTION Description
// vtkHierarchicalDataExtractDataSets extracts the user specified list
// of datasets from a hierarchical dataset.

#ifndef __vtkHierarchicalDataExtractDataSets_h
#define __vtkHierarchicalDataExtractDataSets_h

#include "vtkHierarchicalDataSetAlgorithm.h"

//BTX
struct vtkHierarchicalDataExtractDataSetsInternals;
//ETX

class VTK_GRAPHICS_EXPORT vtkHierarchicalDataExtractDataSets : public vtkHierarchicalDataSetAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkHierarchicalDataExtractDataSets,vtkHierarchicalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkHierarchicalDataExtractDataSets *New();

  // Description:
  // Add a dataset to be extracted.
  void AddDataSet(unsigned int level, unsigned int idx);

  // Description:
  // Remove all entries from the list of datasets to be extracted.
  void ClearDataSetList();

protected:
  vtkHierarchicalDataExtractDataSets();
  ~vtkHierarchicalDataExtractDataSets();

  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);
  virtual int RequestInformation(vtkInformation *, 
                                 vtkInformationVector **, 
                                 vtkInformationVector *);
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

  unsigned int ComputeOutputLevels(unsigned int inputNumLevels);

//BTX
  friend struct vtkHierarchicalDataExtractDataSetsInternals;

  struct DataSetNode
  {
    unsigned int Level;
    unsigned int DataSetId;
    int Initialized;
    
    DataSetNode() : Initialized(0) {}
    DataSetNode(unsigned int level, unsigned int dsid) : 
      Level(level), DataSetId(dsid), Initialized(1)  {}
  };
//ETX

private:
  vtkHierarchicalDataExtractDataSetsInternals* Internal;

  vtkHierarchicalDataExtractDataSets(const vtkHierarchicalDataExtractDataSets&);  // Not implemented.
  void operator=(const vtkHierarchicalDataExtractDataSets&);  // Not implemented.
};

#endif


