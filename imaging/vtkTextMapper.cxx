/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextMapper.cxx
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
#include "vtkTextMapper.h"

#ifdef _WIN32
  #include "vtkWin32TextMapper.h"
#else
  #include "vtkXTextMapper.h"
#endif

// Creates a new text mapper with Font size 12, bold off, italic off,
// and Arial font
vtkTextMapper::vtkTextMapper()
{
  this->Input = (char*) NULL;
  this->FontSize = 12;
  this->Bold = 0;
  this->Italic = 0;
  this->Shadow = 0;
  this->FontFamily = VTK_ARIAL;
  this->FontChanged = 0;
}

vtkTextMapper *vtkTextMapper::New()
{
#ifdef _WIN32
    return vtkWin32TextMapper::New();
#else
    return vtkXTextMapper::New();
#endif

}


vtkTextMapper::~vtkTextMapper()
{
  if (this->Input)
    {
    delete [] this->Input;
    this->Input = NULL;
    }
}



//----------------------------------------------------------------------------
void vtkTextMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Bold: " << (this->Bold ? "On\n" : "Off\n");
  os << indent << "Italic: " << (this->Italic ? "On\n" : "Off\n");
  os << indent << "Shadow: " << (this->Shadow ? "On\n" : "Off\n");
  os << indent << "FontFamily: " << this->FontFamily << "\n";
  os << indent << "FontSize: " << this->FontSize << "\n";
  os << indent << "Input: " << (this->Input ? this->Input : "(none)") << "\n";
 
  vtkMapper2D::PrintSelf(os,indent);
}


void vtkTextMapper::SetShadow(int val) 
{
  if (val == this->Shadow)
    {
    return;
    }
  
  this->Shadow = val; 
  this->FontChanged = 1; 
  this->Modified();
}







