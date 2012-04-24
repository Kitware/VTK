/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClipPolyData - clip polygonal data with user-specified implicit function or input scalar data
// .SECTION Description
// vtkClipPolyData is a filter that clips polygonal data using either
// any subclass of vtkImplicitFunction, or the input scalar
// data. Clipping means that it actually "cuts" through the cells of
// the dataset, returning everything inside of the specified implicit
// function (or greater than the scalar value) including "pieces" of
// a cell. (Compare this with vtkExtractGeometry, which pulls out
// entire, uncut cells.) The output of this filter is polygonal data.
//
// To use this filter, you must decide if you will be clipping with an
// implicit function, or whether you will be using the input scalar
// data.  If you want to clip with an implicit function, you must:
// 1) define an implicit function
// 2) set it with the SetClipFunction method
// 3) apply the GenerateClipScalarsOn method
// If a ClipFunction is not specified, or GenerateClipScalars is off
// (the default), then the input's scalar data will be used to clip
// the polydata.
//
// You can also specify a scalar value, which is used to
// decide what is inside and outside of the implicit function. You can
// also reverse the sense of what inside/outside is by setting the
// InsideOut instance variable. (The cutting algorithm proceeds by
// computing an implicit function value or using the input scalar data
// for each point in the dataset. This is compared to the scalar value
// to determine inside/outside.)
//
// This filter can be configured to compute a second output. The
// second output is the polygonal data that is clipped away. Set the
// GenerateClippedData boolean on if you wish to access this output data.

// .SECTION Caveats
// In order to cut all types of cells in polygonal data, vtkClipPolyData
// triangulates some cells, and then cuts the resulting simplices
// (i.e., points, lines, and triangles). This means that the resulting
// output may consist of different cell types than the input data.

// .SECTION See Also
// vtkImplicitFunction vtkCutter vtkClipVolume

#ifndef __vtkClipPolyData_h
#define __vtkClipPolyData_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkImplicitFunction;
class vtkIncrementalPointLocator;

class VTKFILTERSCORE_EXPORT vtkClipPolyData : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkClipPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function; InsideOut turned off;
  // value set to 0.0; and generate clip scalars turned off.
  static vtkClipPolyData *New();

  // Description:
  // Set the clipping value of the implicit function (if clipping with
  // implicit function) or scalar value (if clipping with
  // scalars). The default value is 0.0.
  vtkSetMacro(Value,double);
  vtkGetMacro(Value,double);

  // Description:
  // Set/Get the InsideOut flag. When off, a vertex is considered
  // inside the implicit function if its value is greater than the
  // Value ivar. When InsideOutside is turned on, a vertex is
  // considered inside the implicit function if its implicit function
  // value is less than or equal to the Value ivar.  InsideOut is off
  // by default.
  vtkSetMacro(InsideOut,int);
  vtkGetMacro(InsideOut,int);
  vtkBooleanMacro(InsideOut,int);

  // Description
  // Specify the implicit function with which to perform the
  // clipping. If you do not define an implicit function, then the input
  // scalar data will be used for clipping.
  virtual void SetClipFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ClipFunction,vtkImplicitFunction);

  // Description:
  // If this flag is enabled, then the output scalar values will be
  // interpolated from the implicit function values, and not the
  // input scalar data. If you enable this flag but do not provide an
  // implicit function an error will be reported.
  vtkSetMacro(GenerateClipScalars,int);
  vtkGetMacro(GenerateClipScalars,int);
  vtkBooleanMacro(GenerateClipScalars,int);

  // Description:
  // Control whether a second output is generated. The second output
  // contains the polygonal data that's been clipped away.
  vtkSetMacro(GenerateClippedOutput,int);
  vtkGetMacro(GenerateClippedOutput,int);
  vtkBooleanMacro(GenerateClippedOutput,int);

  // Description:
  // Return the Clipped output.
  vtkPolyData *GetClippedOutput();

  // Description:
  // Return the output port (a vtkAlgorithmOutput) of the clipped output.
  vtkAlgorithmOutput* GetClippedOutputPort()
    {
      return this->GetOutputPort(1);
    }

  // Description:
  // Specify a spatial locator for merging points. By default, an
  // instance of vtkMergePoints is used.
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified. The
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

  // Description:
  // Return the mtime also considering the locator and clip function.
  unsigned long GetMTime();

protected:
  vtkClipPolyData(vtkImplicitFunction *cf=NULL);
  ~vtkClipPolyData();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  vtkImplicitFunction *ClipFunction;

  vtkIncrementalPointLocator *Locator;
  int InsideOut;
  double Value;
  int GenerateClipScalars;

  int GenerateClippedOutput;
private:
  vtkClipPolyData(const vtkClipPolyData&);  // Not implemented.
  void operator=(const vtkClipPolyData&);  // Not implemented.
};

#endif
