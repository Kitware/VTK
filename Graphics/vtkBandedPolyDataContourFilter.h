/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBandedPolyDataContourFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkBandedPolyDataContourFilter - generate filled contours for vtkPolyData
// .SECTION Description
// vtkBandedPolyDataContourFilter is a filter that takes as input vtkPolyData
// and produces as output filled contours (also represented as vtkPolyData).
// Filled conoturs are bands of cells that all have the same cell scalar
// value, and can therefore be colored the same. The method is also referred
// to as filled contour generation.
//
// To use this filter you must specify one or more contour values.  You can
// either use the method SetValue() to specify each contour value, or use
// GenerateValues() to generate a series of evenly spaced contours.  Each
// contour value divides (or clips) the data into two pieces, values below
// the contour value, and values above it. The scalar values of the piece
// that is below the contour value is set to the average value of the (i-1)
// and ith contour value; the piece above the contour value i is set to the
// average of the ith and (i+1) contour value. Note that if the first and
// last contour values are not the minimum/maximum contour range, then two
// extra contour values are added corresponding to the minimum and maximum
// range values. These extra contour bands can be prevented from being output
// by turning clipping on.
//
// .SECTION See Also
// vtkClipDataSet vtkClipPolyData vtkClipVolume vtkContourFilter
//
#ifndef __vtkBandedPolyDataContourFilter_h
#define __vtkBandedPolyDataContourFilter_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkContourValues.h"

class VTK_GRAPHICS_EXPORT vtkBandedPolyDataContourFilter : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkBandedPolyDataContourFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with no contours defined.
  static vtkBandedPolyDataContourFilter *New();

  // Description:
  // Methods to set / get contour values. A single value at a time can be
  // set with SetValue(). Multiple contour values can be set with
  // GenerateValues(). Note that GenerateValues() generates n values
  // inclusive of the start and end range values.
  void SetValue(int i, float value);
  float GetValue(int i);
  float *GetValues();
  void GetValues(float *contourValues);
  void SetNumberOfContours(int number);
  int GetNumberOfContours();
  void GenerateValues(int numContours, float range[2]);
  void GenerateValues(int numContours, float rangeStart, float rangeEnd);

  // Description:
  // Indicate whether to clip outside the range specified by the user.
  // (The range is contour value[0] to contour value[numContours-1].
  // Clipping means all cells outside of the range specified are not
  // sent to the output.
  vtkSetMacro(Clipping,int);
  vtkGetMacro(Clipping,int);
  vtkBooleanMacro(Clipping,int);

  // Description:
  // Overload GetMTime because we delegate to vtkContourValues so its
  // modified time must be taken into account.
  unsigned long GetMTime();

protected:
  vtkBandedPolyDataContourFilter();
  ~vtkBandedPolyDataContourFilter();

  void Execute();

  int ComputeIndex(float,float);
  int ComputeLowerScalarIndex(float);
  int ComputeUpperScalarIndex(float);
  int IsContourValue(float val);
  int ClipEdge(int v1, int v2, vtkPoints *pts, vtkDataArray *scalars,
               vtkPointData *inPD, vtkPointData *outPD);

  // data members
  vtkContourValues *ContourValues;
  int   NumberOfClipValues;

  int Clipping;

  // sorted and cleaned contour values
  float *ClipValues;
  
private:
  vtkBandedPolyDataContourFilter(const vtkBandedPolyDataContourFilter&);  // Not implemented.
  void operator=(const vtkBandedPolyDataContourFilter&);  // Not implemented.
};

// Description:
// Set a particular contour value at contour number i. The index i ranges 
// between 0<=i<NumberOfContours.
inline void vtkBandedPolyDataContourFilter::SetValue(int i, float value)
  {this->ContourValues->SetValue(i,value);}

// Description:
// Get the ith contour value.
inline float vtkBandedPolyDataContourFilter::GetValue(int i)
  {return this->ContourValues->GetValue(i);}

// Description:
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
inline float *vtkBandedPolyDataContourFilter::GetValues()
  {return this->ContourValues->GetValues();}

// Description:
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
inline void vtkBandedPolyDataContourFilter::GetValues(float *contourValues)
  {this->ContourValues->GetValues(contourValues);}

// Description:
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
inline void vtkBandedPolyDataContourFilter::SetNumberOfContours(int number)
  {this->ContourValues->SetNumberOfContours(number);}

// Description:
// Get the number of contours in the list of contour values.
inline int vtkBandedPolyDataContourFilter::GetNumberOfContours()
  {return this->ContourValues->GetNumberOfContours();}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkBandedPolyDataContourFilter::GenerateValues(int numContours, 
                                                           float range[2])
  {this->ContourValues->GenerateValues(numContours, range);}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkBandedPolyDataContourFilter::GenerateValues(int numContours, 
                                                           float rangeStart, 
                                                           float rangeEnd)
  {this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}


#endif


