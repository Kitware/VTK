/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer.cxx
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
#include "vtkImageViewer.h"
#ifdef _WIN32
#include "vtkImageWin32Viewer.h"
#else
#include "vtkImageXViewer.h"
#endif

//----------------------------------------------------------------------------
vtkImageViewer::vtkImageViewer()
{
  this->Size[0] = 0;
  this->Size[1] = 0;
  this->Input = NULL;
  this->WholeImage = 1;
  this->Coordinate2 = 0;
  this->Coordinate3 = 0;
  this->Mapped = 0;
  
  this->ColorWindow = 255.0;
  this->ColorLevel = 127.0;
  this->ColorFlag = 0;
  this->Red = 0;
  this->Green = 1;
  this->Blue = 2;

  this->XOffset = 0;
  this->YOffset = 0;
}


//----------------------------------------------------------------------------
vtkImageViewer::~vtkImageViewer()
{
}

void vtkImageViewer::SetSize(int a[2])
{
  this->SetSize(a[0],a[1]);
}

//----------------------------------------------------------------------------
vtkImageViewer *vtkImageViewer::New()
{
#ifdef _WIN32
  return new vtkImageWin32Viewer;
#else
  return new vtkImageXViewer;
#endif  
  return NULL;
}


//----------------------------------------------------------------------------
void vtkImageViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  int *b;
  
  vtkObject::PrintSelf(os, indent);
  b = this->Region.GetExtent();
  os << indent << "Extent: (" << b[0] << ", " << b[1] << ", " << b[2] 
     << ", " << b[3] << ")\n";
  os << indent << "Coordinate2: " << this->Coordinate2 << "\n";
  os << indent << "Coordinate3: " << this->Coordinate3 << "\n";
}












