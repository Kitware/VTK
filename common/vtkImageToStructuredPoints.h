/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToStructuredPoints.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkImageToStructuredPoints - Attaches image pipeline to VTK. 
// .SECTION Description
// vtkImageToStructuredPoints changes an image region format to
// a structured points dataset.


#ifndef __vtkImageToStructuredPoints_h
#define __vtkImageToStructuredPoints_h

#include "vtkStructuredPointsSource.h"
#include "vtkGraymap.h"
#include "vtkImageSource.h"
#include "vtkImageRegion.h"


class VTK_EXPORT vtkImageToStructuredPoints : public vtkStructuredPointsSource
{
public:
  vtkImageToStructuredPoints();
  ~vtkImageToStructuredPoints();
  vtkImageToStructuredPoints *New() {return new vtkImageToStructuredPoints;};
  char *GetClassName() {return "vtkImageToStructuredPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the scalar input object from the image pipeline.
  vtkSetObjectMacro(ScalarInput,vtkImageSource);
  vtkGetObjectMacro(ScalarInput,vtkImageSource);

  // Description:
  // Set/Get the vector input object from the image pipeline.
  vtkSetObjectMacro(VectorInput,vtkImageSource);
  vtkGetObjectMacro(VectorInput,vtkImageSource);

  // Description:
  // Set/Get the flag that tells the object to convert the whole image or not.
  vtkSetMacro(WholeImage,int);
  vtkGetMacro(WholeImage,int);
  vtkBooleanMacro(WholeImage,int);

  // Description:
  // ColorScalarsOn tells this output to create color scalars.
  // This object then can be used as input to vtkTIFFWriter.
  vtkSetMacro(ColorScalars,int);
  vtkGetMacro(ColorScalars,int);
  vtkBooleanMacro(ColorScalars,int);

  // Set/Get the extent to translate explicitely.
  void SetExtent(int dim, int *extent);
  vtkImageSetExtentMacro(Extent);
  void GetExtent(int dim, int *extent);
  vtkImageGetExtentMacro(Extent);


  // Description:
  // Set/Get the coordinate system which determines how extent are interpreted.
  // Note: This does not yet change the order of the structured points!
  void SetAxes(int dim, int *axes);
  vtkImageSetMacro(Axes,int);
  void GetAxes(int dim, int *axes);
  vtkImageGetMacro(Axes,int);
  int *GetAxes() {return this->Axes;};  

  // Description:
  // Set/Get the order of the axes to split while streaming.
  void SetSplitOrder(int dim, int *axes);
  vtkImageSetMacro(SplitOrder,int);
  void GetSplitOrder(int dim, int *axes);
  vtkImageGetMacro(SplitOrder,int);
  int *GetSplitOrder() {return this->SplitOrder;};  

  // Description:
  // This object will stream to keep the input regions below this limit.
  vtkSetMacro(InputMemoryLimit,int);
  vtkGetMacro(InputMemoryLimit,int);

  // Description:
  // Which coordinate to use for the fourth dimension. (slice)
  vtkSetMacro(Coordinate3,int);
  vtkGetMacro(Coordinate3,int);
  
  void Update();
  
protected:
  vtkImageSource *ScalarInput;
  vtkImageSource *VectorInput;
  int WholeImage;
  int ColorScalars;
  int Coordinate3;
  int Extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int Axes[VTK_IMAGE_DIMENSIONS];
  int NumberOfSplitAxes;
  int SplitOrder[VTK_IMAGE_DIMENSIONS];
  int InputMemoryLimit;

  void Execute();
  vtkScalars *ScalarExecute(vtkImageRegion *region);
  int ScalarSplitExecute(vtkImageRegion *outRegion, long volumeLimit);
  vtkVectors *VectorExecute(vtkImageRegion *region);
  vtkScalars *CopyToColorScalars(vtkImageRegion *region);
};


#endif


