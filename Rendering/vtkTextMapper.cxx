/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextMapper.h"
#include "vtkImagingFactory.h"

// Control wheter to globally force text antialiasing (ALL), 
// disable antialiasing (NONE), allow antialising (SOME) depending on
// the per-object AntiAliasing attribute.
 
static int vtkTextMapperGlobalAntiAliasing = VTK_TEXT_GLOBAL_ANTIALIASING_SOME;

void vtkTextMapper::SetGlobalAntiAliasing(int val)
{
  if (val == vtkTextMapperGlobalAntiAliasing)
    {
    return;
    }
  if (val < VTK_TEXT_GLOBAL_ANTIALIASING_SOME)
    {
    val = VTK_TEXT_GLOBAL_ANTIALIASING_SOME;
    }
  else if (val > VTK_TEXT_GLOBAL_ANTIALIASING_ALL)
    {
    val = VTK_TEXT_GLOBAL_ANTIALIASING_ALL;
    }

  vtkTextMapperGlobalAntiAliasing = val;
}

int vtkTextMapper::GetGlobalAntiAliasing()
{
  return vtkTextMapperGlobalAntiAliasing;
}

vtkCxxRevisionMacro(vtkTextMapper, "1.38");

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
  this->Justification = VTK_TEXT_LEFT;
  this->VerticalJustification = VTK_TEXT_BOTTOM;

  this->TextLines = NULL;
  this->NumberOfLines = 0;
  this->NumberOfLinesAllocated = 0;
  this->LineOffset = 0.0;
  this->LineSpacing = 1.0;

  this->AntiAliasing = 1;
}

// Shallow copy of an actor.
void vtkTextMapper::ShallowCopy(vtkTextMapper *tm)
{
  this->SetInput(tm->GetInput());
  this->SetClippingPlanes(tm->GetClippingPlanes());
  this->SetFontSize(tm->GetFontSize());
  this->SetBold(tm->GetBold());
  this->SetItalic(tm->GetItalic());
  this->SetShadow(tm->GetShadow());
  this->SetFontFamily(tm->GetFontFamily());
  this->SetJustification(tm->GetJustification());
  this->SetVerticalJustification(tm->GetVerticalJustification());
}

vtkTextMapper *vtkTextMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkImagingFactory::CreateInstance("vtkTextMapper");
  return (vtkTextMapper*)ret;
}

vtkTextMapper::~vtkTextMapper()
{
  if (this->Input)
    {
    delete [] this->Input;
    this->Input = NULL;
    }

  if ( this->TextLines != NULL )
    {
    for (int i=0; i < this->NumberOfLinesAllocated; i++)
      {
      this->TextLines[i]->Delete();
      }
    delete [] this->TextLines;
    }
  
}

//----------------------------------------------------------------------------
void vtkTextMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Line Offset: " << this->LineOffset;
  os << indent << "Line Spacing: " << this->LineSpacing;
  os << indent << "Bold: " << (this->Bold ? "On\n" : "Off\n");
  os << indent << "Italic: " << (this->Italic ? "On\n" : "Off\n");
  os << indent << "Shadow: " << (this->Shadow ? "On\n" : "Off\n");
  os << indent << "FontFamily: " << this->FontFamily << "\n";
  os << indent << "FontSize: " << this->FontSize << "\n";
  os << indent << "Input: " << (this->Input ? this->Input : "(none)") << "\n";
  os << indent << "Justification: ";
  switch (this->Justification)
    {
    case 0: os << "Left  (0)" << endl; break;
    case 1: os << "Centered  (1)" << endl; break;
    case 2: os << "Right  (2)" << endl; break;
    }
  os << indent << "VerticalJustification: ";
  switch (this->VerticalJustification)
    {
    case VTK_TEXT_TOP: os << "Top" << endl; break;
    case VTK_TEXT_CENTERED: os << "Centered" << endl; break;
    case VTK_TEXT_BOTTOM: os << "Bottom" << endl; break;
    }
  
  os << indent << "NumberOfLines: " << this->NumberOfLines << "\n";  
}

int vtkTextMapper::GetWidth(vtkViewport* viewport)
{
  int size[2];
  this->GetSize(viewport, size);
  return size[0];
}

int vtkTextMapper::GetHeight(vtkViewport* viewport)
{
  int size[2];
  this->GetSize(viewport, size);
  return size[1];
}

// Parse the input and create multiple text mappers if multiple lines
// (delimited by \n) are specified.
void vtkTextMapper::SetInput(const char *input)
{
  if ( this->Input && input && (!strcmp(this->Input,input))) 
    {
    return;
    }
  if (this->Input) 
    { 
    delete [] this->Input; 
    }  
  if (input)
    {
    this->Input = new char[strlen(input)+1];
    strcpy(this->Input,input);
    }
  else
    {
    this->Input = NULL;
    }
  this->Modified();
  
  int numLines = this->GetNumberOfLines(input);
  
  if ( numLines <= 1) // a line with no "\n"
    {
    this->NumberOfLines = numLines;
    this->LineOffset = 0.0;
    }

  else //multiple lines
    {
    char *line;
    int i;

    if ( numLines > this->NumberOfLinesAllocated )
      {
      // delete old stuff
      if ( this->TextLines )
        {
        for (i=0; i < this->NumberOfLinesAllocated; i++)
          {
          this->TextLines[i]->Delete();
          }
        delete [] this->TextLines;
        }
      
      // allocate new text mappers
      this->NumberOfLinesAllocated = numLines;
      this->TextLines = new vtkTextMapper *[numLines];
      for (i=0; i < numLines; i++)
        {
        this->TextLines[i] = vtkTextMapper::New();
        }
      } //if we need to reallocate
    
    // set the input strings
    this->NumberOfLines = numLines;
    for (i=0; i < this->NumberOfLines; i++)
      {
      line = this->NextLine(input, i);
      this->TextLines[i]->SetInput( line );
      delete [] line;
      }
    }
}

// Determine the number of lines in the Input string (delimited by "\n").
int vtkTextMapper::GetNumberOfLines(const char *input)
{
  if ( input == NULL || input[0] == '\0')
    {
    return 0;
    }

  int numLines=1;
  const char *ptr = input;

  while ( ptr != NULL )
    {
    if ( (ptr=strstr(ptr,"\n")) != NULL )
      {
      numLines++;
      ptr++; //skip over \n
      }
    }
  
  return numLines;
}

// Get the next \n delimited line. Returns a string that
// must be freed by the calling function.
char *vtkTextMapper::NextLine(const char *input, int lineNum)
{
  const char *ptr, *ptrEnd;
  int strLen;
  char *line;
  
  ptr = input;
  for (int i=0; i != lineNum; i++)
    {
    ptr = strstr(ptr,"\n");
    ptr++;
    }
  ptrEnd = strstr(ptr,"\n");
  if ( ptrEnd == NULL )
    {
    ptrEnd = strchr(ptr, '\0');
    }
  
  strLen = ptrEnd - ptr;
  line = new char[strLen+1];
  strncpy(line, ptr, strLen);
  line[strLen] = '\0';

  return line;
}

// Get the size of a multi-line text string
void vtkTextMapper::GetMultiLineSize(vtkViewport* viewport, int size[2])
{
  int i;
  int lineSize[2];
  
  lineSize[0] = lineSize[1] = size[0] = size[1] = 0;
  for ( i=0; i < this->NumberOfLines; i++ )
    {
    this->TextLines[i]->SetItalic(this->Italic);
    this->TextLines[i]->SetBold(this->Bold);
    this->TextLines[i]->SetShadow(this->Shadow);
    this->TextLines[i]->SetFontSize(this->FontSize);
    this->TextLines[i]->SetFontFamily(this->FontFamily);
    this->TextLines[i]->GetSize(viewport, lineSize);
    size[0] = (lineSize[0] > size[0] ? lineSize[0] : size[0]);
    size[1] = (lineSize[1] > size[1] ? lineSize[1] : size[1]);
    }
  
  // add in the line spacing
  this->LineSize = size[1];
  size[1] = (int)(this->NumberOfLines* this->LineSpacing * size[1]);
}

void vtkTextMapper::RenderOverlayMultipleLines(vtkViewport *viewport, 
                                               vtkActor2D *actor)    
{
  float offset = 0.0;
  int size[2];
  // make sure LineSize is up to date 
  this->GetMultiLineSize(viewport,size);
  
  switch (this->VerticalJustification)
    {
    case VTK_TEXT_TOP:
      offset = 1.0;
      break;
    case VTK_TEXT_CENTERED:
      offset = -this->NumberOfLines/2.0 + 1;
      break;
    case VTK_TEXT_BOTTOM:
      offset = -(this->NumberOfLines - 1.0);
      break;
    }

  for (int lineNum=0; lineNum < this->NumberOfLines; lineNum++)
    {
    this->TextLines[lineNum]->SetItalic(this->Italic);
    this->TextLines[lineNum]->SetBold(this->Bold);
    this->TextLines[lineNum]->SetShadow(this->Shadow);
    this->TextLines[lineNum]->SetFontSize(this->FontSize);
    this->TextLines[lineNum]->SetFontFamily(this->FontFamily);
    this->TextLines[lineNum]->SetJustification(this->Justification);
    this->TextLines[lineNum]->SetLineOffset(this->LineSize*(lineNum+offset));
    this->TextLines[lineNum]->SetLineSpacing(this->LineSpacing);
    this->TextLines[lineNum]->RenderOverlay(viewport,actor);
    }
}

void vtkTextMapper::SetFontSize(int size)
{
  if (size != this->FontSize)
    {
    this->FontSize = size;
    this->Modified();
    this->FontMTime.Modified();
    }

}

void vtkTextMapper::SetBold(int val)
{
  if (val == this->Bold)
    {
    return;
    }
  this->Bold = val;
  this->Modified();
  this->FontMTime.Modified();
}

void vtkTextMapper::SetItalic(int val)
{
  if (val == this->Italic)
    {
    return;
    }
  this->Italic = val;
  this->Modified();
  this->FontMTime.Modified();
}

void vtkTextMapper::SetShadow(int val)
{
  if (val == this->Shadow)
    {
    return;
    }
  this->Shadow = val;
  this->Modified();
}

void vtkTextMapper::SetFontFamily(int val)
{
  if (val == this->FontFamily)
    {
    return;
    }
  this->FontFamily = val;
  this->Modified();
  this->FontMTime.Modified();
}
