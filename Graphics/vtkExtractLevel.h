/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractLevel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractLevel - extract levels between min and max from a
// hierarchical box dataset.
// .SECTION Description
// vtkExtractLevel filter extracts the levels between (and including) the user
// specified min and max levels.

#ifndef __vtkExtractLevel_h
#define __vtkExtractLevel_h

#include "vtkHierarchicalBoxDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkExtractLevel : public vtkHierarchicalBoxDataSetAlgorithm
{
public:
  static vtkExtractLevel* New();
  vtkTypeMacro(vtkExtractLevel, vtkHierarchicalBoxDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Select the levels that should be extracted. All other levels will have no
  // datasets in them.
  void AddLevel(unsigned int level);
  void RemoveLevel(unsigned int level);
  void RemoveAllLevels();

//BTX
protected:
  vtkExtractLevel();
  ~vtkExtractLevel();

  /// Implementation of the algorithm.
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

private:
  vtkExtractLevel(const vtkExtractLevel&); // Not implemented.
  void operator=(const vtkExtractLevel&); // Not implemented.

  class vtkSet;
  vtkSet* Levels;
//ETX
};

#endif


