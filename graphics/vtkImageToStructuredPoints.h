/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToStructuredPoints.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// a structured points dataset.  It was modeled after vtkPNMReader.


#ifndef __vtkImageToStructuredPoints_h
#define __vtkImageToStructuredPoints_h

#include "vtkStructuredPointsSource.h"
#include "vtkGraymap.h"
#include "vtkImageSource.h"
#include "vtkImageRegion.h"


class vtkImageToStructuredPoints : public vtkStructuredPointsSource
{
public:
  vtkImageToStructuredPoints();
  ~vtkImageToStructuredPoints();
  char *GetClassName() {return "vtkImageToStructuredPoints";};

  // Description:
  // Set/Get the input object from the image pipline.
  vtkSetObjectMacro(Input,vtkImageSource);
  vtkGetObjectMacro(Input,vtkImageSource);
  // Description:
  // Set/Get the flag that tells the object to convert the whole image or not.
  vtkSetMacro(WholeImage,int);
  vtkGetMacro(WholeImage,int);
  vtkBooleanMacro(WholeImage,int);

  // Forward these messages to the "Region".
  void SetBounds(int *bounds)
  {this->Region.SetBounds3d(bounds); this->WholeImageOff();};
  void SetBounds(int min0, int max0, int min1, int max1, int min2, int max2)
  {this->Region.SetBounds3d(min0,max0,min1,max1,min2,max2);
  this->WholeImageOff();};
  int *GetBounds(){return this->Region.GetBounds3d();};
  void GetBounds(int *bounds){this->Region.GetBounds3d(bounds);};
  void GetBounds(int &min0,int &max0,int &min1,int &max1,int &min2,int &max2)
  {this->Region.GetBounds3d(min0,max0,min1,max1,min2,max2);};

  // Description:
  // Set the coordinate system which determines how bounds are interpreted.
  // Note: This does not yet change the order of the structured points!
  void SetAxes(int axis0, int axis1, int axis2)
  {this->Region.SetAxes3d(axis0,axis1,axis2); this->Modified();};
  void SetAxes(int axis0, int axis1, int axis2, int axis3)
  {this->Region.SetAxes4d(axis0,axis1,axis2,axis3); this->Modified();};

  // Description:
  // Set the order of the axes to split while streaming.
  void SetSplitOrder(int axis0, int axis1)
  {this->SplitOrder.SetAxes2d(axis0,axis1);};
  void SetSplitOrder(int axis0, int axis1, int axis2)
  {this->SplitOrder.SetAxes3d(axis0,axis1,axis2);};
  void SetSplitOrder(int axis0, int axis1, int axis2, int axis3)
  {this->SplitOrder.SetAxes4d(axis0,axis1,axis2,axis3);};
  
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
  vtkImageSource *Input;
  int WholeImage;
  int Coordinate3;
  vtkImageRegion Region;
  vtkImageRegion SplitOrder;
  int InputMemoryLimit;

  void Execute();
  int SplitExecute(vtkImageRegion *outRegion);
};

#endif


