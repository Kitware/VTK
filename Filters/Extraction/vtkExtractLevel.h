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
/**
 * @class   vtkExtractLevel
 * @brief   extract levels between min and max from a
 * hierarchical box dataset.
 *
 * vtkExtractLevel filter extracts the levels between (and including) the user
 * specified min and max levels.
*/

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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;


  //@{
  /**
   * Select the levels that should be extracted. All other levels will have no
   * datasets in them.
   */
  void AddLevel(unsigned int level);
  void RemoveLevel(unsigned int level);
  void RemoveAllLevels();
  //@}

protected:
  vtkExtractLevel();
  ~vtkExtractLevel() VTK_OVERRIDE;

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**,vtkInformationVector* ) VTK_OVERRIDE;

  /// Implementation of the algorithm.
  int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  int FillInputPortInformation(int port,vtkInformation *info) VTK_OVERRIDE;
  int FillOutputPortInformation(int port,vtkInformation *info) VTK_OVERRIDE;

private:
  vtkExtractLevel(const vtkExtractLevel&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractLevel&) VTK_DELETE_FUNCTION;

  class vtkSet;
  vtkSet* Levels;

};

#endif


