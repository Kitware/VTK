/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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

#include "vtkImageWindow.h"

#ifdef _WIN32
  #include "vtkWin32ImageWindow.h"
#else
  #include "vtkXImageWindow.h"
#endif

#include "vtkImagerCollection.h"

// Creates a vtkImageWindow with 
// background erasing disabled and gray scale hint off
vtkImageWindow::vtkImageWindow()
{
  vtkDebugMacro(<<"vtkImageWindow()");

  this->WindowCreated = 0;
  // Override the default erase mode
  this->Erase = 0;
  this->GrayScaleHint = 0;
  this->FileName = (char*) NULL;
  this->PPMImageFilePtr = (FILE*) NULL;
}

void vtkImageWindow::SaveImageAsPPM()
{
  if( this->OpenPPMImageFile() )
    {
    this->WritePPMImageFile();
    this->ClosePPMImageFile();
    }
}


int vtkImageWindow::OpenPPMImageFile()
{
  //  open the ppm file and write header 
  if ( this->FileName != NULL && *this->FileName != '\0')
    {
    this->PPMImageFilePtr = fopen(this->FileName,"wb");
    if (!this->PPMImageFilePtr)
      {
      vtkErrorMacro(<< "ImageWindow unable to open image file for writing\n");
      return 0;
      }
  }
  return 1;
}

void vtkImageWindow::ClosePPMImageFile()
{
  fclose(this->PPMImageFilePtr);
  this->PPMImageFilePtr = NULL;
}


void vtkImageWindow::WritePPMImageFile()
{
  int    *size;
  unsigned char *buffer;
  int i;

  // get the size
  size = this->GetSize();
  // get the data
  buffer = this->GetPixelData(0,0,size[0]-1,size[1]-1,1);

  if(!this->PPMImageFilePtr)
    {
    vtkErrorMacro(<< "ImageWindow: no image file for writing\n");
    return;
    }
 
  // write out the header info 
  fprintf(this->PPMImageFilePtr,"P6\n%i %i\n255\n",size[0],size[1]);
 
  // now write the binary info 
  for (i = size[1]-1; i >= 0; i--)
    {
    fwrite(buffer + i*size[0]*3,1,size[0]*3,this->PPMImageFilePtr);
    }

  delete [] buffer;
}

void vtkImageWindow::GetPosition(int *x, int *y)
{
  int *tmp;
  
  tmp = this->GetPosition();
  *x = tmp[0];
  *y = tmp[1];
}

void vtkImageWindow::GetSize(int *x, int *y)
{
  int *tmp;
  
  tmp = this->GetSize();
  *x = tmp[0];
  *y = tmp[1];
}
  
vtkImageWindow::~vtkImageWindow()
{
  vtkDebugMacro(<<"~vtkImageWindow");
}

vtkImageWindow* vtkImageWindow::New()
{
#ifdef _WIN32
    return vtkWin32ImageWindow::New();
#else
    return vtkXImageWindow::New();
#endif
}

void vtkImageWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWindow::PrintSelf(os, indent);

  if ( this->FileName )
    {
    os << indent << "FileName: " << this->FileName << "\n";
    }
  else
    {
    os << indent << "FileName: (none)\n";
    }

  os << indent << "Position: (" << this->Position[0]
     << "," << this->Position[1] << ") \n";

  os << indent << "Gray scale hint: " << this->GrayScaleHint << "\n";
}


void vtkImageWindow::Render()
{
  vtkImager* tempImager;

  vtkDebugMacro (<< "vtkImageWindow::Render" );

  if (!this->WindowCreated)
    {
    vtkDebugMacro (<< "vtkImageWindow::Render - Creating default window");
    this->MakeDefaultWindow();
    this->WindowCreated = 1;
    }
 
  int numImagers = this->Imagers.GetNumberOfItems();

  if (numImagers == 0)
    {
    vtkDebugMacro (<< "vtkImageWindow::Render - No imagers in collection");
    return;
    }
 
  if (this->DoubleBuffer)
    {
    this->SwapBuffers();
    }

  if (this->Erase)
    {
    this->EraseWindow();
    }

  // tell each of the imagers to render
  for (this->Imagers.InitTraversal(); 
       (tempImager = this->Imagers.GetNextItem());)
    {
    tempImager->Render(); 
    }
 
  if (this->DoubleBuffer)
    {
    this->SwapBuffers();
    }

  return;
}


void vtkImageWindow::EraseWindow()
{
  vtkImager* tempImager;

  // tell each of the imagers to erase
  for (this->Imagers.InitTraversal(); 
       (tempImager = this->Imagers.GetNextItem());)
    {
    tempImager->Erase(); 
    }
 
}

void vtkImageWindow::AddImager(vtkImager* imager)
{
  // Set the imager's parent window
  imager->SetVTKWindow(this);

  // Add the imager to the collection
  this->Imagers.AddItem(imager);

  // Window will need to update 
  this->Modified();
}

void vtkImageWindow::RemoveImager(vtkImager* imager)
{
  this->Imagers.RemoveItem(imager);

  // Window will need to update
  this->Modified();
}














