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
#include "vtkTextProperty.h"

vtkCxxRevisionMacro(vtkTextMapper, "1.43");

vtkCxxSetObjectMacro(vtkTextMapper,TextProperty,vtkTextProperty);

//----------------------------------------------------------------------------
// Creates a new text mapper

vtkTextMapper::vtkTextMapper()
{
  this->Input = (char*)NULL;
  this->TextProperty = vtkTextProperty::New();

  this->TextLines = NULL;
  this->NumberOfLines = 0;
  this->NumberOfLinesAllocated = 0;
}

//----------------------------------------------------------------------------
// Shallow copy of an actor.

void vtkTextMapper::ShallowCopy(vtkTextMapper *tm)
{
  this->SetInput(tm->GetInput());
  this->SetTextProperty(tm->GetTextProperty());

  this->SetClippingPlanes(tm->GetClippingPlanes());
}

//----------------------------------------------------------------------------
vtkTextMapper *vtkTextMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkImagingFactory::CreateInstance("vtkTextMapper");
  return (vtkTextMapper*)ret;
}

//----------------------------------------------------------------------------
vtkTextMapper::~vtkTextMapper()
{
  if (this->Input)
    {
    delete [] this->Input;
    this->Input = NULL;
    }

  if (this->TextLines != NULL)
    {
    for (int i=0; i < this->NumberOfLinesAllocated; i++)
      {
      this->TextLines[i]->Delete();
      }
    delete [] this->TextLines;
    }
  
  this->SetTextProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkTextMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->TextProperty)
    {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Text Property: (none)\n";
    }
   
  os << indent << "Input: " << (this->Input ? this->Input : "(none)") << "\n";
  os << indent << "NumberOfLines: " << this->NumberOfLines << "\n";  
}

//----------------------------------------------------------------------------
int vtkTextMapper::GetWidth(vtkViewport* viewport)
{
  int size[2];
  this->GetSize(viewport, size);
  return size[0];
}

//----------------------------------------------------------------------------
int vtkTextMapper::GetHeight(vtkViewport* viewport)
{
  int size[2];
  this->GetSize(viewport, size);
  return size[1];
}

//----------------------------------------------------------------------------
#define VTK_TM_DEBUG 0

int vtkTextMapper::SetConstrainedFontSize(vtkViewport *viewport,
                                          int targetWidth,
                                          int targetHeight)
{
  vtkTextProperty *tprop = this->GetTextProperty();
  if (!tprop)
    {
    vtkErrorMacro(<<"Need text property to apply constaint");
    return 0;
    }

  int fontSize = tprop->GetFontSize();
#if VTK_TM_DEBUG
  int oldfontSize = fontSize;
#endif

  // Use the last size as a first guess

  int tempi[2];
  this->GetSize(viewport, tempi);

#if 1
  // Now get an estimate of the target font size using bissection

#if VTK_TM_DEBUG
  printf("vtkTextMapper::SetConstrainedFontSize:     init size: (%d, %d) => (%d, %d)\n", tempi[0], tempi[1], targetWidth, targetHeight);
#endif

  // Based on experimentation with big and small font size increments,
  // ceil() gives the best result.
  // big:   floor: 10749, ceil: 10106, cast: 10749, vtkMath::Round: 10311
  // small: floor: 12122, ceil: 11770, cast: 12122, vtkMath::Round: 11768
  // I guess the best optim would be to have a look at the shape of the
  // font size growth curve (probably not that linear)

  float fx = (float)targetWidth / (float)tempi[0];
  float fy = (float)targetHeight / (float)tempi[1] ;
  fontSize = (int)ceil((float)fontSize * ((fx <= fy) ? fx : fy));
  tprop->SetFontSize(fontSize);
  this->GetSize(viewport, tempi);

#if VTK_TM_DEBUG
  printf("vtkTextMapper::SetConstrainedFontSize: estimate size: %2d (was: %2d)\n", 
         fontSize, oldfontSize);
  printf("vtkTextMapper::SetConstrainedFontSize: estimate size: (%d, %d) => (%d, %d)\n", tempi[0], tempi[1], targetWidth, targetHeight);
#endif
#endif

  // While the size is too small increase it

  while (tempi[1] < targetHeight &&
         tempi[0] < targetWidth && 
         fontSize < 100)
    {
    fontSize++;
#if VTK_TM_DEBUG
    printf("vtkTextMapper::SetConstrainedFontSize:  search+ size: %2d\n", 
           fontSize);
#endif
    tprop->SetFontSize(fontSize);
    this->GetSize(viewport, tempi);
    }

#if VTK_TM_DEBUG
  printf("vtkTextMapper::SetConstrainedFontSize:  search+ size: (%d, %d) => (%d, %d)\n", tempi[0], tempi[1], targetWidth, targetHeight);
#endif

  // While the size is too large decrease it

  while ((tempi[1] > targetHeight || tempi[0] > targetWidth) 
         && fontSize > 0)
    {
    fontSize--;
#if VTK_TM_DEBUG
    printf("vtkTextMapper::SetConstrainedFontSize:  search- size: %2d\n", 
           fontSize);
#endif
    tprop->SetFontSize(fontSize);
    this->GetSize(viewport, tempi);
    }

#if VTK_TM_DEBUG
  printf("vtkTextMapper::SetConstrainedFontSize:  search- size: (%d, %d) => (%d, %d)\n", tempi[0], tempi[1], targetWidth, targetHeight);

  printf("vtkTextMapper::SetConstrainedFontSize:      new size: %2d (was: %2d)\n", 
         fontSize, oldfontSize);
#endif

  return fontSize;
}

//----------------------------------------------------------------------------
int vtkTextMapper::SetMultipleConstrainedFontSize(vtkViewport *viewport, 
                                                  int targetWidth, 
                                                  int targetHeight, 
                                                  vtkTextMapper **mappers, 
                                                  int nbOfMappers, 
                                                  int *maxResultingSize)
{
  maxResultingSize[0] = maxResultingSize[1] = 0;

  if (nbOfMappers == 0)
    {
    return 0;
    }

  int fontSize, aSize;

  // First try to find the constrained font size of the first mapper: it
  // will be used minimize the search for the remaining mappers, given the 
  // fact that all mappers are likely to have the same constrained font size.

  for (int first = 0; first < nbOfMappers && !mappers[first]; first++) {}

  if (first >= nbOfMappers)
    {
    return 0;
    }
  
  fontSize = mappers[first]->SetConstrainedFontSize(
    viewport, targetWidth, targetHeight);

#if VTK_TM_DEBUG
  printf("vtkTextMapper::SetMultipleConstrainedFontSize:    first size: %2d\n", 
         fontSize);
#endif

  // Find the constrained font size for the remaining mappers and 
  // pick the smallest

  for (int i = first + 1; i < nbOfMappers; i++)
    {
    if (mappers[i])
      {
      mappers[i]->GetTextProperty()->SetFontSize(fontSize);
      aSize = mappers[i]->SetConstrainedFontSize(
        viewport, targetWidth, targetHeight);
      if (aSize < fontSize)
        {
        fontSize = aSize;
        }
      }
    }

#if VTK_TM_DEBUG
  printf("vtkTextMapper::SetMultipleConstrainedFontSize: smallest size: %2d\n", 
         fontSize);
#endif

  // Assign the smallest size to all text mappers and find the largest area

  int tempi[2];
  for (i = first; i < nbOfMappers; i++)
    {
    if (mappers[i])
      {
      mappers[i]->GetTextProperty()->SetFontSize(fontSize);
      mappers[i]->GetSize(viewport, tempi);
      if (tempi[0] > maxResultingSize[0])
        {
        maxResultingSize[0] = tempi[0];
        }
      if (tempi[1] > maxResultingSize[1])
        {
        maxResultingSize[1] = tempi[1];
        }
      }
    }

  // The above code could be optimized further since the mappers
  // labels are likely to have the same height: in that case, we could
  // have searched for the largest label, find the constrained size
  // for this one, then applied this size to all others.  But who
  // knows, maybe one day the text property will support a text
  // orientation/rotation, and in that case the height will vary.

  return fontSize;
}


//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
// Get the size of a multi-line text string

void vtkTextMapper::GetMultiLineSize(vtkViewport* viewport, int size[2])
{
  int i;
  int lineSize[2];

  vtkTextProperty *tprop = this->GetTextProperty();
  if (!tprop) 
    {
    vtkErrorMacro(<<"Need text property to get multiline size of mapper");
    size[0] = size[1] = 0;
    return;
    }

  lineSize[0] = lineSize[1] = size[0] = size[1] = 0;
  for ( i=0; i < this->NumberOfLines; i++ )
    {
    this->TextLines[i]->GetTextProperty()->ShallowCopy(tprop);
    this->TextLines[i]->GetSize(viewport, lineSize);
    size[0] = (lineSize[0] > size[0] ? lineSize[0] : size[0]);
    size[1] = (lineSize[1] > size[1] ? lineSize[1] : size[1]);
    }
  
  // add in the line spacing
  this->LineSize = size[1];
  size[1] = (int)(this->NumberOfLines * tprop->GetLineSpacing() * size[1]);
}

//----------------------------------------------------------------------------
void vtkTextMapper::RenderOverlayMultipleLines(vtkViewport *viewport, 
                                               vtkActor2D *actor)    
{
  float offset = 0.0;
  int size[2];
  // make sure LineSize is up to date 
  this->GetMultiLineSize(viewport,size);

  vtkTextProperty *tprop = this->GetTextProperty();
  if (!tprop)
    {
    vtkErrorMacro(<<"Need text property to render multiple lines of mapper");
    return;
    }
  
  switch (tprop->GetVerticalJustification())
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
    this->TextLines[lineNum]->GetTextProperty()->ShallowCopy(tprop);
    this->TextLines[lineNum]->GetTextProperty()->SetLineOffset
      (this->LineSize*(lineNum+offset));
    this->TextLines[lineNum]->GetTextProperty()->SetVerticalJustificationToBottom();
    this->TextLines[lineNum]->RenderOverlay(viewport,actor);
    }
}

//----------------------------------------------------------------------------
// Backward compatibility calls

void vtkTextMapper::SetFontFamily(int val) 
{ 
  if (this->TextProperty)
    {
    this->TextProperty->SetFontFamily(val); 
    }
}

int vtkTextMapper::GetFontFamily()
{ 
  if (this->TextProperty)
    {
    return this->TextProperty->GetFontFamily(); 
    }
  else
    {
    return 0;
    }
}

void vtkTextMapper::SetFontSize(int size) 
{ 
  if (this->TextProperty)
    {
    this->TextProperty->SetFontSize(size); 
    }
}

int vtkTextMapper::GetFontSize()
{ 
  if (this->TextProperty)
    {
    return this->TextProperty->GetFontSize(); 
    }
  else
    {
    return 0;
    }
}

void vtkTextMapper::SetBold(int val)
{ 
  if (this->TextProperty)
    {
    this->TextProperty->SetBold(val); 
    }
}

int vtkTextMapper::GetBold()
{ 
  if (this->TextProperty)
    {
    return this->TextProperty->GetBold(); 
    }
  else
    {
    return 0;
    }
}

void vtkTextMapper::SetItalic(int val)
{ 
  if (this->TextProperty)
    {
    this->TextProperty->SetItalic(val); 
    }
}

int vtkTextMapper::GetItalic()
{ 
  if (this->TextProperty)
    {
    return this->TextProperty->GetItalic(); 
    }
  else
    {
    return 0;
    }
}

void vtkTextMapper::SetShadow(int val)
{ 
  if (this->TextProperty)
    {
    this->TextProperty->SetShadow(val); 
    }
}

int vtkTextMapper::GetShadow()
{ 
  if (this->TextProperty)
    {
    return this->TextProperty->GetShadow(); 
    }
  else
    {
    return 0;
    }
}
  
void vtkTextMapper::SetJustification(int val)
{ 
  if (this->TextProperty)
    {
    this->TextProperty->SetJustification(val); 
    }
}

int vtkTextMapper::GetJustification()
{ 
  if (this->TextProperty)
    {
    return this->TextProperty->GetJustification(); 
    }
  else
    {
    return 0;
    }
}
    
void vtkTextMapper::SetVerticalJustification(int val)
{ 
  if (this->TextProperty)
    {
    this->TextProperty->SetVerticalJustification(val); 
    }
}

int vtkTextMapper::GetVerticalJustification()
{ 
  if (this->TextProperty)
    {
    return this->TextProperty->GetVerticalJustification(); 
    }
  else
    {
    return 0;
    }
}
    
void vtkTextMapper::SetLineOffset(float val)
{ 
  if (this->TextProperty)
    {
    this->TextProperty->SetLineOffset(val); 
    }
}

float vtkTextMapper::GetLineOffset()
{ 
  if (this->TextProperty)
    {
    return this->TextProperty->GetLineOffset(); 
    }
  else
    {
    return 0.0f;
    }
}

void vtkTextMapper::SetLineSpacing(float val)
{ 
  if (this->TextProperty)
    {
    this->TextProperty->SetLineSpacing(val); 
    }
}

float vtkTextMapper::GetLineSpacing()
{ 
  if (this->TextProperty)
    {
    return this->TextProperty->GetLineSpacing(); 
    }
  else
    {
    return 0.0f;
    }
}
