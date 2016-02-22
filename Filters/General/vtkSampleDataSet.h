/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSampleDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSampleDataSet - sample an implicit function over a dataset,
// generating scalar values and optional gradient vectors

// .SECTION Description
// vtkSampleDataSet is a filter that evaluates an implicit function and
// (optional) gradients at each point in an input vtkDataSet. The output
// of the filter are new scalar values (the function values) and the
// optional vector, gradient array.

// .SECTION Caveats
// This class has been threaded with vtkSMPTools. Using TBB or other
// non-sequential type (set in the CMake variable
// VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.

// .SECTION See Also
// vtkSampleFunction vtkImplicitModeller

#ifndef vtkSampleDataSet_h
#define vtkSampleDataSet_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkImplicitFunction;
class vtkDataArray;

class VTKFILTERSGENERAL_EXPORT vtkSampleDataSet : public vtkDataSetAlgorithm
{
public:
  // Description:
  // Standard instantiation, type information, and print methods.
  static vtkSampleDataSet *New();
  vtkTypeMacro(vtkSampleDataSet,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the implicit function to use to generate data.
  virtual void SetImplicitFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);

  // Description:
  // Turn on/off the computation of gradients.
  vtkSetMacro(ComputeGradients,int);
  vtkGetMacro(ComputeGradients,int);
  vtkBooleanMacro(ComputeGradients,int);

  // Description:
  // Set/get the scalar array name for this data set. Initial value is
  // "Implicit scalars".
  vtkSetStringMacro(ScalarArrayName);
  vtkGetStringMacro(ScalarArrayName);

  // Description:
  // Set/get the gradient array name for this data set. Initial value is
  // "Implicit gradients".
  vtkSetStringMacro(GradientArrayName);
  vtkGetStringMacro(GradientArrayName);

  // Description:
  // Return the MTime also considering the implicit function.
  unsigned long GetMTime();

protected:
  vtkSampleDataSet();
  ~vtkSampleDataSet();

  vtkImplicitFunction *ImplicitFunction;
  int ComputeGradients;
  char *ScalarArrayName;
  char *GradientArrayName;

  virtual void ReportReferences(vtkGarbageCollector*);

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);


private:
  vtkSampleDataSet(const vtkSampleDataSet&);  // Not implemented.
  void operator=(const vtkSampleDataSet&);  // Not implemented.
};

#endif
