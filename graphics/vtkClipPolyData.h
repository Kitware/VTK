/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipPolyData.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkClipPolyData - clip polygonal data with user-specified implicit function
// .SECTION Description
// vtkClipPolyData is a filter that clips polygonal data to by a any subclass
// of vtkImplicitFunction. Clipping means that it actually "cuts" through
// the cells of the dataset, returning everthing inside of the specified 
// implicit function, including "pieces" of a cell. (Compare this with 
// vtkExtractGeometry, which pulls out entire, uncut cells.)
//
// To use this filter, at a minimum you need to specify an implicit
// function. You can also specify a implicit function value, which is
// used to decide what is inside and outside of the implicit
// function. You can also reverse the sense of what inside/outside is by
// setting the InsideOut instance variable. (The cutting algorithm
// proceeds by computing an implicit function value for each point in
// the dataset. This is compared to the implicit function value to
// determine inside/outside.)
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
// vtkImplicitFunction vtkCutter

#ifndef __vtkClipPolyData_h
#define __vtkClipPolyData_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkImplicitFunction.h"

class VTK_EXPORT vtkClipPolyData : public vtkPolyDataToPolyDataFilter
{
public:
  vtkClipPolyData(vtkImplicitFunction *cf=NULL);
  ~vtkClipPolyData();
  static vtkClipPolyData *New() {return new vtkClipPolyData;};
  char *GetClassName() {return "vtkClipPolyData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the clipping value of the implicit function. Default is 0.0.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);
  
  // Description:
  // Set/Get the InsideOut flag. When off, a vertex is considered inside
  // the implicit function if its implicit function value is greater
  // than the Value ivar. When InsideOutside is turned on, a vertex is
  // considered inside the implicit function if its implicit function
  // value is less than or equal to the Value ivar.  
  // InsideOut is off by default.
  vtkSetMacro(InsideOut,int);
  vtkGetMacro(InsideOut,int);
  vtkBooleanMacro(InsideOut,int);

  // Description
  // Specify the implicit function with which to perform the clipping.
  vtkSetObjectMacro(ClipFunction,vtkImplicitFunction);
  vtkGetObjectMacro(ClipFunction,vtkImplicitFunction);

  // Description:
  // If this flag is enabled, then the output scalar values will be 
  // interpolated from the implicit function values, and not the 
  // input scalar data.
  vtkSetMacro(GenerateClipScalars,int);
  vtkGetMacro(GenerateClipScalars,int);
  vtkBooleanMacro(GenerateClipScalars,int);

  // Description:
  // Control whether a second output is generated. The second output
  // contains the polygonal data that's been clipped away.
  vtkSetMacro(GenerateClippedOutput,int);
  vtkGetMacro(GenerateClippedOutput,int);
  vtkBooleanMacro(GenerateClippedOutput,int);

  vtkPolyData *GetClippedOutput() {return this->ClippedOutput;};

  void SetLocator(vtkPointLocator *locator);
  void SetLocator(vtkPointLocator& locator) {this->SetLocator(&locator);};
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified. The 
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

  unsigned long int GetMTime();

protected:
  void Execute();
  vtkImplicitFunction *ClipFunction;
  
  vtkPointLocator *Locator;
  int SelfCreatedLocator;
  int InsideOut;
  float Value;
  int GenerateClipScalars;

  int GenerateClippedOutput;
  vtkPolyData *ClippedOutput;
};

#endif
