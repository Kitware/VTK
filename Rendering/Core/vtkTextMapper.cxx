/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextMapper.h"

#include "vtkTextProperty.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkAbstractObjectFactoryNewMacro(vtkTextMapper)
//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkTextMapper,TextProperty,vtkTextProperty);

//----------------------------------------------------------------------------
// Creates a new text mapper
vtkTextMapper::vtkTextMapper()
{
  this->Input = NULL;
  // consistent Register/unregister
  this->TextProperty = NULL;
  this->SetTextProperty(vtkTextProperty::New());
  this->TextProperty->Delete();

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
int vtkTextMapper::SetConstrainedFontSize(vtkViewport *viewport,
                                          int targetWidth,
                                          int targetHeight)
{
  return this->SetConstrainedFontSize(this, viewport, targetWidth, targetHeight);
}


//----------------------------------------------------------------------------
int vtkTextMapper::SetConstrainedFontSize(vtkTextMapper *tmapper, vtkViewport *viewport,
                                          int targetWidth, int targetHeight)
{
  // If target "empty" just return
  if (targetWidth == 0 && targetHeight == 0)
    {
    return 0;
    }

  vtkTextProperty *tprop = tmapper->GetTextProperty();
  if (!tprop)
    {
    vtkGenericWarningMacro(<<"Need text property to apply constraint");
    return 0;
    }
  int fontSize = tprop->GetFontSize();

  // Use the last size as a first guess
  int tempi[2];
  tmapper->GetSize(viewport, tempi);

  // Now get an estimate of the target font size using bissection
  // Based on experimentation with big and small font size increments,
  // ceil() gives the best result.
  // big:   floor: 10749, ceil: 10106, cast: 10749, vtkMath::Round: 10311
  // small: floor: 12122, ceil: 11770, cast: 12122, vtkMath::Round: 11768
  // I guess the best optim would be to have a look at the shape of the
  // font size growth curve (probably not that linear)
  if (tempi[0] && tempi[1])
    {
    float fx = targetWidth / static_cast<float>(tempi[0]);
    float fy = targetHeight / static_cast<float>(tempi[1]);
    fontSize = static_cast<int>(ceil(fontSize * ((fx <= fy) ? fx : fy)));
    tprop->SetFontSize(fontSize);
    tmapper->GetSize(viewport, tempi);
    }

  // While the size is too small increase it
  while (tempi[1] <= targetHeight &&
         tempi[0] <= targetWidth &&
         fontSize < 100)
    {
    fontSize++;
    tprop->SetFontSize(fontSize);
    tmapper->GetSize(viewport, tempi);
    }

  // While the size is too large decrease it
  while ((tempi[1] > targetHeight || tempi[0] > targetWidth)
         && fontSize > 0)
    {
    fontSize--;
    tprop->SetFontSize(fontSize);
    tmapper->GetSize(viewport, tempi);
    }

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
  int i, first;
  for (first = 0; first < nbOfMappers && !mappers[first]; first++) {}

  if (first >= nbOfMappers)
    {
    return 0;
    }

  fontSize = mappers[first]->SetConstrainedFontSize(
    viewport, targetWidth, targetHeight);

  // Find the constrained font size for the remaining mappers and
  // pick the smallest
  for (i = first + 1; i < nbOfMappers; i++)
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
int vtkTextMapper::SetRelativeFontSize(vtkTextMapper *tmapper,
                                       vtkViewport *viewport,  int *targetSize,
                                       int *stringSize, float sizeFactor)
{
  sizeFactor = (sizeFactor <= 0.0f ? 0.015f : sizeFactor);

  int fontSize, targetWidth, targetHeight;
  // Find the best size for the font
  targetWidth = targetSize[0] > targetSize[1] ? targetSize[0] : targetSize[1];
  targetHeight = static_cast<int>(sizeFactor * targetSize[0]
                                  + sizeFactor * targetSize[1]);

  fontSize = tmapper->SetConstrainedFontSize(tmapper, viewport, targetWidth, targetHeight);
  tmapper->GetSize(viewport, stringSize);

  return fontSize;
}

//----------------------------------------------------------------------------
int vtkTextMapper::SetMultipleRelativeFontSize(vtkViewport *viewport,
                                               vtkTextMapper **textMappers,
                                               int nbOfMappers, int *targetSize,
                                               int *stringSize, float sizeFactor)
{
  int fontSize, targetWidth, targetHeight;

  // Find the best size for the font
  // WARNING: check that the below values are in sync with the above
  // similar function.

  targetWidth = targetSize [0] > targetSize[1] ? targetSize[0] : targetSize[1];

  targetHeight = static_cast<int>(sizeFactor * targetSize[0]
                                  + sizeFactor * targetSize[1]);

  fontSize =
    vtkTextMapper::SetMultipleConstrainedFontSize(viewport,
                                                  targetWidth, targetHeight,
                                                  textMappers,
                                                  nbOfMappers,
                                                  stringSize);

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
  size[1] = static_cast<int>(
    size[1] * (1.0 + (this->NumberOfLines - 1) * tprop->GetLineSpacing()));
}

//----------------------------------------------------------------------------
void vtkTextMapper::RenderOverlayMultipleLines(vtkViewport *viewport,
                                               vtkActor2D *actor)
{
  float offset = 0.0f;
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
      offset = 0.0f;
      break;
    case VTK_TEXT_CENTERED:
      offset = (-this->NumberOfLines + 1.0f) / 2.0f;
      break;
    case VTK_TEXT_BOTTOM:
      offset = -this->NumberOfLines + 1.0f;
      break;
    }

  for (int lineNum=0; lineNum < this->NumberOfLines; lineNum++)
    {
    this->TextLines[lineNum]->GetTextProperty()->ShallowCopy(tprop);
    this->TextLines[lineNum]->GetTextProperty()->SetLineOffset
      (tprop->GetLineOffset() + static_cast<int>(this->LineSize * (lineNum + offset) * tprop->GetLineSpacing()));
    this->TextLines[lineNum]->RenderOverlay(viewport,actor);
    }
}
