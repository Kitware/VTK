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

#ifndef vtkExtractLevel_h
#define vtkExtractLevel_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class VTKFILTERSEXTRACTION_EXPORT vtkExtractLevel :
  public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkExtractLevel* New();
  vtkTypeMacro(vtkExtractLevel, vtkMultiBlockDataSetAlgorithm);
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

  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**,vtkInformationVector* );

  /// Implementation of the algorithm.
  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  virtual int FillInputPortInformation(int port,vtkInformation *info);
  virtual int FillOutputPortInformation(int port,vtkInformation *info);

private:
  vtkExtractLevel(const vtkExtractLevel&); // Not implemented.
  void operator=(const vtkExtractLevel&); // Not implemented.

  class vtkSet;
  vtkSet* Levels;
//ETX
};

#endif


