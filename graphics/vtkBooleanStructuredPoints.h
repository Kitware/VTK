/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBooleanStructuredPoints.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkBooleanStructuredPoints - combine two or more structured point sets
// .SECTION Description
// vtkBooleanStructuredPoints is a filter that performs boolean combinations on
// two or more input structured point sets. Operations supported include union,
// intersection, and difference. A special method is provided that allows 
// incremental appending of data to the filter output.
// .SECTION Caveats
// To boolean two structured point datasets together requires that the 
// dimensions of each dataset is identical. The origin and data spacing are 
// ignored.

#ifndef __vtkBooleanStructuredPoints_h
#define __vtkBooleanStructuredPoints_h

#include "vtkFilter.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsCollection.h"

#define VTK_UNION_OPERATOR 0
#define VTK_INTERSECTION_OPERATOR 1
#define VTK_DIFFERENCE_OPERATOR 2

class VTK_EXPORT vtkBooleanStructuredPoints : public vtkFilter
{
public:
  vtkBooleanStructuredPoints();
  static vtkBooleanStructuredPoints *New() {return new vtkBooleanStructuredPoints;};
  const char *GetClassName() {return "vtkBooleanStructuredPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddInput(vtkStructuredPoints *in);
  void AddInput(vtkStructuredPoints &in) {this->AddInput(&in);};
  void RemoveInput(vtkStructuredPoints *in);
  void RemoveInput(vtkStructuredPoints &in) {this->RemoveInput(&in);};
  vtkStructuredPointsCollection *GetInput() {return &(this->InputList);};

  // filter interface
  void Update();

  // alternative method to boolean data
  void Append(vtkStructuredPoints *);
  
  // Description:
  // Specify the type of boolean operation.
  vtkSetClampMacro(OperationType,int,VTK_UNION_OPERATOR,VTK_DIFFERENCE_OPERATOR);
  vtkGetMacro(OperationType,int);
  void SetOperationTypeToUnion() 
    {this->SetOperationType(VTK_UNION_OPERATOR);};
  void SetOperationTypeToIntersection() 
    {this->SetOperationType(VTK_INTERSECTION_OPERATOR);};
  void SetOperationTypeToDifference() 
    {this->SetOperationType(VTK_DIFFERENCE_OPERATOR);};
  char *GetOperationTypeAsString();

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Get the output of this filter.
  vtkStructuredPoints *GetOutput() {return (vtkStructuredPoints *)this->Output;};

protected:
  // Usual data generation method
  void Execute();
  void InitializeBoolean();

  // list of data sets to append together
  vtkStructuredPointsCollection InputList;

  // pointer to operation function
  //BTX
  void (vtkBooleanStructuredPoints::*Operator)();
  //ETX

  // boolean is performed on this resolution in this space
  int SampleDimensions[3];
  float ModelBounds[6];

  // various operations
  int OperationType;
};

// Description:
// Return the boolean operation type as a descriptive character string.
inline char *vtkBooleanStructuredPoints::GetOperationTypeAsString(void)
{
  if ( this->OperationType == VTK_UNION_OPERATOR )
    {
    return "Union";
    }
  else if ( this->OperationType == VTK_INTERSECTION_OPERATOR ) 
    {
    return "Intersection";
    }
  else
    {
    return "Difference";
    }
}


#endif
