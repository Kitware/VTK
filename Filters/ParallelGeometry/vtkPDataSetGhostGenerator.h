/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPDataSetGhostGenerator.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkPDataSetGhostGenerator.h -- Base class for parallel ghost generators
//
// .SECTION Description
//  An abstract class that provides common functionality and implements an
//  interface for all parallel ghost data generators.
//
// .SECTION See Also
// vtkDataSetGhostGenerator, vtkPUniformGridGhostDataGenerator,
// vtkPStructuredGridGhostDataGenerator, vtkPRectilinearGridGhostDataGenerator

#ifndef VTKPDATASETGHOSTGENERATOR_H_
#define VTKPDATASETGHOSTGENERATOR_H_

#include "vtkFiltersParallelGeometryModule.h" // For export macro
#include "vtkDataSetGhostGenerator.h"

class vtkMultiProcessController;
class vtkMultiBlockDataSet;

class VTKFILTERSPARALLELGEOMETRY_EXPORT vtkPDataSetGhostGenerator :
  public vtkDataSetGhostGenerator
{
public:
   vtkTypeMacro(vtkPDataSetGhostGenerator,vtkDataSetGhostGenerator);
   void PrintSelf(ostream& os, vtkIndent indent);

   // Description:
   // Get/Set macro for the multi-process controller. If a controller is not
   // supplied, then, the global controller is assumed.
   vtkSetMacro(Controller, vtkMultiProcessController*);
   vtkGetMacro(Controller, vtkMultiProcessController*);

   // Description:
   // Initializes
   void Initialize();

   // Description:
   // Barrier synchronization
   void Barrier();

protected:
  vtkPDataSetGhostGenerator();
  virtual ~vtkPDataSetGhostGenerator();

  // Description:
  // Creates ghost layers. Implemented by concrete implementations.
  virtual void GenerateGhostLayers(
      vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out)=0;

  int Rank;
  bool Initialized;
  vtkMultiProcessController *Controller;

private:
  vtkPDataSetGhostGenerator(const vtkPDataSetGhostGenerator&); // Not implemented
  void operator=(const vtkPDataSetGhostGenerator&); // Not implemented
};

#endif /* VTKPDATASETGHOSTGENERATOR_H_ */
