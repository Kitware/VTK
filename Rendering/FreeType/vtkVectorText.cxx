/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorText.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVectorText.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTransformPolyDataFilter.h"

#include <clocale>

vtkStandardNewMacro(vtkVectorText);

#include "vtkVectorTextData.cxx"

// Construct object with no string set and backing enabled.
vtkVectorText::vtkVectorText()
{
  this->Text = nullptr;

  this->SetNumberOfInputPorts(0);
}

int vtkVectorText::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints* newPoints;
  vtkCellArray* newPolys;
  int ptOffset = 0;
  int aPoint, i;
  int pos = 0;
  float xpos = 0;
  float ypos = 0;
  int ptCount, triCount;
  VTK_VECTOR_TEXT_GLYPH aLetter;
  float width;
  float ftmp[3];

  if (this->Text == nullptr)
  {
    vtkErrorMacro(<< "Text is not set!");
    return 0;
  }

  // Set things up; allocate memory
  newPoints = vtkPoints::New();
  newPolys = vtkCellArray::New();
  ftmp[2] = 0.0;

  // Create Text
  while (this->Text[pos])
  {
    switch (this->Text[pos])
    {
      case 32:
        xpos += 0.4;
        break;

      case 10:
        ypos -= 1.4;
        xpos = 0;
        break;

      default:
        // if we have a valid character
        if ((this->Text[pos] > 32) && (this->Text[pos] < 127))
        {
          // add the result to our output
          aLetter = Letters[static_cast<int>(this->Text[pos]) - 33];
          ptCount = aLetter.ptCount;
          width = aLetter.width;
          for (i = 0; i < ptCount; i++)
          {
            ftmp[0] = aLetter.points[i].x;
            ftmp[1] = aLetter.points[i].y;
            ftmp[0] += xpos;
            ftmp[1] += ypos;
            newPoints->InsertNextPoint(ftmp);
          }
          triCount = aLetter.triCount;
          for (i = 0; i < triCount; i++)
          {
            newPolys->InsertNextCell(3);
            aPoint = aLetter.triangles[i].p1;
            newPolys->InsertCellPoint(aPoint + ptOffset);
            aPoint = aLetter.triangles[i].p2;
            newPolys->InsertCellPoint(aPoint + ptOffset);
            aPoint = aLetter.triangles[i].p3;
            newPolys->InsertCellPoint(aPoint + ptOffset);
          }
          ptOffset += ptCount;
          xpos += width;
        }
        break;
    }
    pos++;
  }

  //
  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  return 1;
}

void vtkVectorText::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Text: " << (this->Text ? this->Text : "(none)") << "\n";
}

vtkVectorText::~vtkVectorText()
{
  delete[] this->Text;
}
