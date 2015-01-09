/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageToPolyDataFilter.h"

#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkEdgeTable.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkScalarsToColors.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkImageToPolyDataFilter);

vtkCxxSetObjectMacro(vtkImageToPolyDataFilter,LookupTable,vtkScalarsToColors);

vtkImageToPolyDataFilter::vtkImageToPolyDataFilter()
{
  this->OutputStyle = VTK_STYLE_POLYGONALIZE;
  this->ColorMode = VTK_COLOR_MODE_LINEAR_256;
  this->Smoothing = 1;
  this->NumberOfSmoothingIterations = 40;
  this->Decimation = 1;
  this->DecimationError = 1.5;
  this->Error = 100;
  this->SubImageSize = 250;

  this->Table = vtkUnsignedCharArray::New();
  this->LookupTable = NULL;
}

vtkImageToPolyDataFilter::~vtkImageToPolyDataFilter()
{
  this->Table->Delete();
  if ( this->LookupTable )
    {
    this->LookupTable->Delete();
    }
}

// declare helper functions
//

int vtkImageToPolyDataFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData *input = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPolyData *tmpOutput;
  vtkPolyData *tmpInput;
  vtkAppendPolyData *append;
  vtkPolyData *appendOutput;
  vtkDataArray *inScalars = input->GetPointData()->GetScalars();
  vtkIdType numPixels=input->GetNumberOfPoints();
  int dims[3], numComp;
  double origin[3], spacing[3];
  vtkUnsignedCharArray *pixels;
  int type;
  int numPieces[2], extent[4];
  int i, j, newDims[3], totalPieces, pieceNum, abortExecute=0;
  double newOrigin[3];

  // Check input and initialize
  vtkDebugMacro(<<"Vectorizing image...");

  if ( inScalars == NULL || numPixels < 1 )
    {
    vtkDebugMacro(<<"Not enough input to create output");
    return 1;
    }

  append = vtkAppendPolyData::New();
  tmpOutput=vtkPolyData::New();
  tmpInput=vtkPolyData::New();
  numComp=inScalars->GetNumberOfComponents();
  type=inScalars->GetDataType();

  appendOutput=append->GetOutput();

  input->GetDimensions(dims);
  input->GetOrigin(origin);
  input->GetSpacing(spacing);

  // Figure out how many pieces to break the image into (the image
  // might be too big to process). The filter does a series of appends
  // to join the pieces together.
  numPieces[0] = ((dims[0]-2) / this->SubImageSize) + 1;
  numPieces[1] = ((dims[1]-2) / this->SubImageSize) + 1;
  totalPieces = numPieces[0]*numPieces[1];

  appendOutput->Initialize(); //empty the output
  append->AddInputData(tmpOutput); //output of piece
  append->AddInputData(tmpInput); //output of previoius append

  // Loop over this many pieces
  for (pieceNum=j=0; j < numPieces[1] && !abortExecute; j++)
    {
    extent[2] = j*this->SubImageSize; //the y range
    extent[3] = (j+1)*this->SubImageSize;
    if ( extent[3] >= dims[1] )
      {
      extent[3] = dims[1] - 1;
      }

    for (i=0; i < numPieces[0] && !abortExecute; i++)
      {
      extent[0] = i*this->SubImageSize; //the x range
      extent[1] = (i+1)*this->SubImageSize;
      if ( extent[1] >= dims[0] )
        {
        extent[1] = dims[0] - 1;
        }

      vtkDebugMacro(<<"Processing #" << pieceNum);
      this->UpdateProgress ((double)pieceNum/totalPieces);
      if (this->GetAbortExecute())
        {
        abortExecute = 1;
        break;
        }
      pieceNum++;

      // Figure out characteristics of current sub-image
      newDims[0] = extent[1] - extent[0] + 1;
      newDims[1] = extent[3] - extent[2] + 1;
      newOrigin[0] = origin[0] + extent[0]*spacing[0];
      newOrigin[1] = origin[1] + extent[2]*spacing[1];
      newOrigin[2] = 0.0;

      // Create a quantized copy of the image based on the color table
      //
      pixels = this->QuantizeImage(inScalars, numComp, type, dims, extent);
      vtkDebugMacro(<<"Quantizing color...image size (" <<newDims[0]
                    <<", " <<newDims[1] << ")");

      // Generate polygons according to mode setting
      //
      if ( this->OutputStyle == VTK_STYLE_PIXELIZE )
        {
        this->PixelizeImage(pixels, newDims, newOrigin, spacing, tmpOutput);
        }
      else if ( this->OutputStyle == VTK_STYLE_RUN_LENGTH )
        {
        this->RunLengthImage(pixels, newDims, newOrigin, spacing, tmpOutput);
        }
      else //VTK_STYLE_POLYGONALIZE
        {
        this->PolygonalizeImage(pixels, newDims, newOrigin, spacing,tmpOutput);
        }

      // Append pieces together
      //
      tmpInput->CopyStructure(appendOutput);
      tmpInput->GetPointData()->PassData(appendOutput->GetPointData());
      tmpInput->GetCellData()->PassData(appendOutput->GetCellData());
      append->Update();

      // Clean up this iteration
      //
      pixels->Delete();
      tmpInput->Initialize();
      tmpOutput->Initialize();
      } // for i pieces
    } // for j pieces

  // Create the final output contained in the append filter
  output->CopyStructure(appendOutput);
  output->GetPointData()->PassData(appendOutput->GetPointData());
  output->GetCellData()->PassData(appendOutput->GetCellData());

  append->Delete();
  tmpInput->Delete();
  tmpOutput->Delete();

  return 1;
}

void vtkImageToPolyDataFilter::PixelizeImage(vtkUnsignedCharArray *pixels,
                                             int dims[3], double origin[3],
                                             double spacing[3],
                                             vtkPolyData *output)
{
  int numPts, numCells, i, j, id;
  vtkIdType pts[4];
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  double x[3];
  vtkUnsignedCharArray *polyColors;
  unsigned char *ptr, *colors=pixels->GetPointer(0);

  // create the points - see whether to create or append
  numPts = (dims[0]+1) * (dims[1]+1);
  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numPts);

  x[2] = 0.0;
  for (id=0, j=0; j<=dims[1]; j++)
    {
    x[1] = origin[1] + j*spacing[1];
    for (i=0; i<=dims[0]; i++)
      {
      x[0] = origin[0] + i*spacing[0];
      newPts->SetPoint(id, x);
      id++;
      }
    }
  output->SetPoints(newPts);
  newPts->Delete();

  // create the cells and cell colors
  //
  numCells = dims[0] * dims[1];
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numCells,4));

  polyColors = vtkUnsignedCharArray::New();
  polyColors->SetNumberOfValues(3*numCells); //for rgb
  polyColors->SetNumberOfComponents(3);

  // loop over all pixels, creating a quad per pixel.
  // Note: copying point data (pixel values) to cell data (quad colors).
  for (id=0, j=0; j<dims[1]; j++)
    {
    for (i=0; i<dims[0]; i++)
      {
      pts[0] = i + j*(dims[0]+1);
      pts[1] = pts[0] + 1;
      pts[2] = pts[1] + dims[0] + 1;
      pts[3] = pts[2] - 1;
      newPolys->InsertNextCell(4, pts);
      ptr = colors + 3*id;
      polyColors->SetValue(3*id, ptr[0]);
      polyColors->SetValue(3*id+1, ptr[1]);
      polyColors->SetValue(3*id+2, ptr[2]);
      id++;
      }
    }

  output->SetPolys(newPolys);
  newPolys->Delete();

  output->GetCellData()->SetScalars(polyColors);
  polyColors->Delete();
}

void vtkImageToPolyDataFilter::RunLengthImage(vtkUnsignedCharArray *pixels,
                                             int dims[3], double origin[3],
                                             double spacing[3],
                                             vtkPolyData *output)
{
  int i, j;
  vtkIdType pts[4], id;
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  double x[3], minX, maxX, minY, maxY;
  vtkUnsignedCharArray *polyColors;
  unsigned char *colors=pixels->GetPointer(0), *color;

  // Setup data
  newPts = vtkPoints::New();
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(dims[0]*dims[1]/10,4));

  polyColors = vtkUnsignedCharArray::New();
  polyColors->Allocate(3*dims[0]*dims[1]/10); //for rgb
  polyColors->SetNumberOfComponents(3);

  // Loop over row-by-row generating quad polygons
  x[2] = 0.0;
  for (j=0; j<dims[1]; j++)
    {
    if ( j == 0 )
      {
      minY = origin[1];
      maxY = origin[1] + 0.5*spacing[1];
      }
    else if ( j == (dims[1]-1) )
      {
      minY = origin[1] + j*spacing[1] - 0.5*spacing[1];
      maxY = origin[1] + j*spacing[1];
      }
    else
      {
      minY = origin[1] + j*spacing[1] - 0.5*spacing[1];
      maxY = origin[1] + j*spacing[1] + 0.5*spacing[1];
      }

    for ( i=0; i < dims[0]; )
      {
      if ( i == 0 )
        {
        minX = origin[0];
        }
      else
        {
        minX = origin[0] + i*spacing[0] - 0.5*spacing[0];
        }
      color = colors + 3*(i+j*dims[0]);
      while ( i < dims[0] )
        {
        unsigned char *ptr = colors + 3*(i+j*dims[0]);
        if ( ! this->IsSameColor(color,ptr) )
          {
          break;
          }
        else
          {
          i++;
          }
        }

      if ( i >= dims[0] )
        {
        maxX = origin[0] + (dims[0]-1)*spacing[0];
        }
      else
        {
        maxX = origin[0] + (i-1)*spacing[0] + 0.5*spacing[0];
        }

      // Create quad cell
      x[0] = minX;
      x[1] = minY;
      pts[0] = newPts->InsertNextPoint(x);
      x[0] = maxX;
      pts[1] = newPts->InsertNextPoint(x);
      x[1] = maxY;
      pts[2] = newPts->InsertNextPoint(x);
      x[0] = minX;
      pts[3] = newPts->InsertNextPoint(x);
      id = newPolys->InsertNextCell(4,pts);
      polyColors->InsertValue(3*id, color[0]);
      polyColors->InsertValue(3*id+1, color[1]);
      polyColors->InsertValue(3*id+2, color[2]);
      }
    }

  output->SetPoints(newPts);
  newPts->Delete();
  output->SetPolys(newPolys);
  newPolys->Delete();

  output->GetCellData()->SetScalars(polyColors);
  polyColors->Delete();
}


void vtkImageToPolyDataFilter::PolygonalizeImage(vtkUnsignedCharArray *pixels,
                               int dims[3], double origin[3], double spacing[3],
                               vtkPolyData *output)
{
  int numPolys;
  int numPixels=dims[0]*dims[1];

  // Perform connected traversal on quantized points. This builds
  // the initial "polygons" in implicit form.
  //
  this->PolyColors = vtkUnsignedCharArray::New();
  this->PolyColors->SetNumberOfComponents(3);
  this->PolyColors->Allocate(5000);

  numPolys = this->ProcessImage(pixels, dims);
  vtkDebugMacro(<<"Visited regions..." << numPolys << " polygons");

  // Build edges around the boundary of the polygons. Also identify
  // junction points where 3 or 4 polygons meet.
  //
  vtkPoints *points =  vtkPoints::New();
  points->Allocate(numPixels/2, numPixels/2);

  vtkUnsignedCharArray *pointDescr = vtkUnsignedCharArray::New();
  pointDescr->Allocate(numPixels/2, numPixels/2);

  vtkCellArray *edgeConn = vtkCellArray::New();
  edgeConn->Allocate(numPixels/2, numPixels/2);
  vtkPolyData *edges = vtkPolyData::New();
  edges->SetPoints(points);
  edges->SetLines(edgeConn);
  points->Delete();
  edgeConn->Delete();

  this->BuildEdges(pixels, dims, origin, spacing, pointDescr, edges);
  vtkDebugMacro(<<"Edges built...");

  // Now that we've got the edges, we have to build the "loops" around the
  // polygons that define the polygon explicitly.
  //
  vtkUnsignedCharArray *polyColors = vtkUnsignedCharArray::New();
  polyColors->SetNumberOfComponents(3);
  polyColors->SetNumberOfValues(numPolys*3);

  this->BuildPolygons(pointDescr, edges, numPolys, polyColors);
  this->PolyColors->Delete();
  delete [] this->Visited;
  vtkDebugMacro(<<"Constructed polygons...");

  // Smooth edge network. Some points are identified as fixed, others
  // move using Laplacian smoothing.
  //
  if ( this->Smoothing )
    {
    this->SmoothEdges(pointDescr, edges);
    vtkDebugMacro(<<"Edges smoothed...");
    }

  // Decimate edge network. There will be colinear vertices along edges.
  // These are eliminated.
  //
  if ( this->Decimation )
    {
    this->DecimateEdges(edges, pointDescr, this->DecimationError);
    }

  // Create output polydata. Each polyon is output with its edges.
  //
  this->GeneratePolygons(edges, numPolys, output, polyColors, pointDescr);
  vtkDebugMacro(<<"Output generated...");

  // clean up and get out
  edges->Delete();
  polyColors->Delete();
  pointDescr->Delete();
}

// The following are private helper functions----------------------------------
//
vtkUnsignedCharArray *vtkImageToPolyDataFilter::QuantizeImage(
                          vtkDataArray *inScalars, int numComp, int type,
                          int dims[3], int extent[4])
{
  int numPixels, i, j, idx, id;
  vtkUnsignedCharArray *pixels;
  unsigned char *ptr, *ptr2, *outPixels;
  unsigned char *inPixels;

  // doing a portion of the image
  numPixels = (extent[1]-extent[0]+1) * (extent[3]-extent[2]+1);
  pixels = vtkUnsignedCharArray::New();
  pixels->SetNumberOfValues(3*numPixels);
  outPixels = pixels->GetPointer(0);

  // Figure out how to quantize
  //
  if ( this->ColorMode == VTK_COLOR_MODE_LINEAR_256 )
    {
    // Check scalar type
    if ( type != VTK_UNSIGNED_CHAR || numComp != 3 )
      {
      vtkErrorMacro(<<"Wrong input scalar type");
      return 0;
      }
    else
      {
      inPixels = static_cast<vtkUnsignedCharArray *>(inScalars)->GetPointer(0);
      }

    // Generate a color table used to quantize the points
    //
    if ( this->GetMTime() > this->TableMTime )
      {
      this->BuildTable(inPixels);
      }

    for (id=0, j=extent[2]; j <= extent[3]; j++)
      {
      for (i=extent[0]; i <= extent[1]; i++)
        {
        idx = i + j*dims[0];
        ptr = inPixels + 3*idx;
        ptr2 = outPixels + 3*id;
        const unsigned char *color = this->GetColor(ptr);
        ptr2[0] = color[0];
        ptr2[1] = color[1];
        ptr2[2] = color[2];
        id++;
        }
      }
    }//using build in table

  else //using provided lookup table
    {
    if ( numComp != 1 || this->LookupTable == NULL )
      {
      vtkErrorMacro(<<"LUT mode requires single component scalar and LUT");
      return 0;
      }

    double s;
    for (id=0, j=extent[2]; j <= extent[3]; j++)
      {
      for (i=extent[0]; i <= extent[1]; i++)
        {
        idx = i + j*dims[0];
        s = inScalars->GetComponent(idx,0);
        const unsigned char *color = this->LookupTable->MapValue(s);
        ptr2 = outPixels + 3*id;
        ptr2[0] = color[0];
        ptr2[1] = color[1];
        ptr2[2] = color[2];
        id++;
        }
      }
    }

  return pixels;
}

void vtkImageToPolyDataFilter::BuildTable(unsigned char *vtkNotUsed(inPixels))
{
  int red, green, blue, idx=0;

  this->Table->SetNumberOfValues(256*3);

  // use 3-3-2 bits for rgb
  for (blue=0; blue<256; blue+=64)
    {
    for (green=0; green<256; green+=32)
      {
      for (red=0; red<256; red+=32)
        {
        this->Table->SetValue(idx++, red);
        this->Table->SetValue(idx++, green);
        this->Table->SetValue(idx++, blue);
        }
      }
    }
}

int vtkImageToPolyDataFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

void vtkImageToPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Output Style: ";
  if ( this->OutputStyle == VTK_STYLE_PIXELIZE )
    {
    os << indent << "Pixelize\n";
    }
  else if ( this->OutputStyle == VTK_STYLE_RUN_LENGTH )
    {
    os << indent << "RunLength\n";
    }
  else // this->OutputStyle == VTK_STYLE_POLYGONALIZE
    {
    os << indent << "Polygonalize\n";
    }

  os << indent << "Color Mode: ";
  if ( this->ColorMode == VTK_STYLE_PIXELIZE )
    {
    os << indent << "LUT\n";
    }
  else // this->ColorMode == VTK_STYLE_POLYGONALIZE
    {
    os << indent << "Linear256\n";
    }

  os << indent << "Smoothing: " << (this->Smoothing ? "On\n" : "Off\n");
  os << indent << "Number of Smoothing Iterations: "
     << this->NumberOfSmoothingIterations << "\n";

  os << indent << "Decimation: " << (this->Decimation ? "On\n" : "Off\n");
  os << indent << "Decimation Error: "
     << (this->DecimationError ? "On\n" : "Off\n");

  os << indent << "Error: " << this->Error << "\n";
  os << indent << "Sub-Image Size: " << this->SubImageSize << "\n";

  if ( this->LookupTable )
    {
    os << indent << "LookupTable:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "LookupTable: (none)\n";
    }

}


//--------------------------private helper functions---------------------------
// Determines whether two pixels are the same color
int vtkImageToPolyDataFilter::IsSameColor(unsigned char *p1, unsigned char *p2)
{
  int d2 = (p1[0]-p2[0])*(p1[0]-p2[0]) + (p1[1]-p2[1])*(p1[1]-p2[1]) +
           (p1[2]-p2[2])*(p1[2]-p2[2]);

  return (d2 > this->Error ? 0 : 1);
}

unsigned char *vtkImageToPolyDataFilter::GetColor(unsigned char *rgb)
{
  // round to nearest value
  int red = (rgb[0] + 16) / 32;
  red = (red > 7 ? 7 : red);
  int green =(rgb[1] + 16) / 32;
  green = (green > 7 ? 7 : green);
  int blue = (rgb[2] + 32) / 64;
  blue = (blue > 3 ? 3 : blue);

  return this->Table->GetPointer(3*(red + green*8 + blue*64));
}

void vtkImageToPolyDataFilter::GetIJ(int id, int &i, int &j, int dims[3])
{
  i = id % dims[0];
  j = id / dims[0];
}

// Get the left-right-top-bottom neighboring pixels of a given pixel
// The method has been modified to return right neighbor (mode==0);
// or top neighbor (mode==1) or all neighbors (mode==2).
int vtkImageToPolyDataFilter::GetNeighbors(unsigned char *ptr, int &i, int &j,
                       int dims[2],unsigned char *neighbors[4], int mode)
{
  int numNeis=0;

  if ( mode == 0 )
    {
    if ( (i+1) < dims[0] )
      {
      neighbors[numNeis++] = ptr + 3; //jump over rgb
      }
    if ( (i-1) >= 0 )
      {
      neighbors[numNeis++] = ptr - 3; //jump over rgb
      }
    }

  else if ( mode == 1 )
    {
    if ( (j+1) < dims[1] )
      {
      neighbors[numNeis++] = ptr + 3*dims[0];
      }
    }

  else
    {
    if ( (i+1) < dims[0] )
      {
      neighbors[numNeis++] = ptr + 3; //jump over rgb
      }
    if ( (i-1) >= 0 )
      {
      neighbors[numNeis++] = ptr - 3;
      }

    if ( (j+1) < dims[1] )
      {
      neighbors[numNeis++] = ptr + 3*dims[0];
      }
    if ( (j-1) >= 0 )
      {
      neighbors[numNeis++] = ptr - 3*dims[0];
      }
    }

  return numNeis;

}


// Marks connected regions with different colors.
int vtkImageToPolyDataFilter::ProcessImage(vtkUnsignedCharArray *scalars,
                                           int dims[2])
{
  int numPixels = dims[0]*dims[1];
  vtkIdList *wave, *wave2, *tmpWave;
  int numIds, regionNumber, i, j, k, id, x, y, numNeighbors;
  unsigned char *neighbors[4], *ptr;
  unsigned char *pixels = scalars->GetPointer(0);

  // Collect groups of pixels together into similar colored regions. These
  // will be eventually grouped into polygons and/or lines.
  //
  // mark all pixels unvisited
  regionNumber = -1;
  this->Visited = new int [numPixels];
  memset (this->Visited, (int)-1, numPixels*sizeof(int));

  // set up the connected traversal
  wave = vtkIdList::New();
  wave->Allocate(static_cast<int>(numPixels/4.0),
                 static_cast<int>(numPixels/4.0));
  wave2 = vtkIdList::New();
  wave2->Allocate(static_cast<int>(numPixels/4.0),
                  static_cast<int>(numPixels/4.0));

  // visit connected pixels. Pixels are connected if they are topologically
  // adjacent and they have "equal" color values.
  for (i=0; i < numPixels; i++)
    {
    if ( this->Visited[i] == -1 )
      {//start a connected wave
      this->Visited[i] = ++regionNumber;
      ptr = pixels + 3*i;
      this->PolyColors->InsertValue(3*regionNumber, ptr[0]); //assign color
      this->PolyColors->InsertValue(3*regionNumber+1, ptr[1]);
      this->PolyColors->InsertValue(3*regionNumber+2, ptr[2]);
      wave->Reset(); wave2->Reset();

      // To prevent creating polygons with inner loops, we're going to start
      // the wave as a "vertical" stack of pixels, and then propagate the
      // wave horizontally only.
      wave->InsertId(0,i);
      this->GetIJ(i, x, y, dims);
      while ( (numNeighbors = this->GetNeighbors(ptr, x, y, dims, neighbors, 1)) )
        {
        id = (neighbors[0] - pixels) / 3;
        if ( this->Visited[id] == -1 && this->IsSameColor(ptr, neighbors[0]) )
          {
          this->Visited[id] = regionNumber;
          wave->InsertNextId(id);
          ptr = pixels + 3*id;
          this->GetIJ(id, x, y, dims);
          }
        else
          {
          break;
          }
        }

      // Okay, defined vertical wave, now propagate horizontally
      numIds = wave->GetNumberOfIds();
      while ( numIds > 0 )
        {
        for (j=0; j<numIds; j++) //propagate wave
          {
          id = wave->GetId(j);
          ptr = pixels + 3*id;
          this->GetIJ(id, x, y, dims);
          numNeighbors = this->GetNeighbors(ptr, x, y, dims, neighbors, 0);
          for (k=0; k<numNeighbors; k++)
            {
            id = (neighbors[k] - pixels) / 3;
            if ( this->Visited[id] == -1 && this->IsSameColor(ptr, neighbors[k]) )
              {
              this->Visited[id] = regionNumber;
              wave2->InsertNextId(id);
              }
            }//for each pixel neighbor
          }//for pixels left in wave
        numIds = wave2->GetNumberOfIds();
        tmpWave = wave;
        wave = wave2;
        wave2 = tmpWave;
        wave2->Reset();
        }//while still propogating
      }//if not, start wave
    }//for all pixels

  wave->Delete();
  wave2->Delete();

  return regionNumber+1;
}

// Create polygons and place into output
void vtkImageToPolyDataFilter::GeneratePolygons(vtkPolyData *edges,
                               int vtkNotUsed(numPolys), vtkPolyData *output,
                               vtkUnsignedCharArray *polyColors,
                               vtkUnsignedCharArray *pointDescr)
{
  vtkCellArray *newPolys, *inPolys;
  int i, numPts;
  vtkIdType *pts = 0;
  vtkIdType npts = 0;

  // Copy the points via reference counting
  //
  output->SetPoints(edges->GetPoints());

  // Create the polygons - points may have been decimated so these
  // points have to be culled.
  //
  inPolys = edges->GetPolys();
  newPolys = vtkCellArray::New();
  newPolys->Allocate(inPolys->GetSize());

  for ( inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    newPolys->InsertNextCell(0);
    numPts = 0;
    for (i=0; i<npts; i++)
      {
      if ( pointDescr->GetValue(pts[i]) != 2 )
        {
        newPolys->InsertCellPoint(pts[i]);
        numPts++;
        }
      }
    newPolys->UpdateCellCount(numPts);
    }

  output->SetPolys(newPolys);
  newPolys->Delete();

  output->GetCellData()->SetScalars(polyColors);
}

// Uses clipping approach to build the polygon edges
int vtkImageToPolyDataFilter::BuildEdges(vtkUnsignedCharArray *vtkNotUsed(pixels),
                                         int dims[3], double origin[3],
                                         double spacing[3],
                                         vtkUnsignedCharArray *pointDescr,
                                         vtkPolyData *edges)
{
  double x[3];
  int i, j, edgeCount;
  vtkIdType ptId, p0, p1, p2, p3, startId, attrId, id[8], pts[4];
  vtkCellArray *edgeConn = edges->GetLines();
  vtkPoints *points = edges->GetPoints();

  // Build edges around perimeter of image. Note that the point ids
  // The first four points are the image corners and are inserted and
  // marked so that they can't be moved during smoothing.
  points->InsertPoint(0, origin);
  pointDescr->InsertValue(0, 1);

  // Keep track of the polygons that use each edge as well as associated
  // intersection points on edge (if any)
  this->EdgeTable = vtkEdgeTable::New();
  this->EdgeTable->InitEdgeInsertion(dims[0]*dims[1],1);

  this->EdgeUseTable = vtkEdgeTable::New();
  this->EdgeUseTable->InitEdgeInsertion(dims[0]*dims[1],1);

  this->EdgeUses = vtkIntArray::New();
  this->EdgeUses->SetNumberOfComponents(2);
  this->EdgeUses->Allocate(4*dims[0]*dims[1],dims[0]*dims[1]);

  // Generate corner points of image
  x[0] = origin[0] + (dims[0]-1)*spacing[0];
  x[1] = origin[1];
  x[2] = 0.0;
  points->InsertPoint(1, x);
  pointDescr->InsertValue(1, 1);

  x[0] = origin[0] + (dims[0]-1)*spacing[0];
  x[1] = origin[1] + (dims[1]-1)*spacing[1];;
  x[2] = 0.0;
  points->InsertPoint(2, x);
  pointDescr->InsertValue(2, 1);

  x[0] = origin[0];
  x[1] = origin[1] + (dims[1]-1)*spacing[1];
  x[2] = 0.0;
  points->InsertPoint(3, x);
  pointDescr->InsertValue(3, 1);

  // Let's create perimeter edges - bottom x edge
  startId = 0;
  x[1] = origin[1];
  for (i=0; i<(dims[0]-1); i++)
    {
    p0 = i;
    p1 = i + 1;
    if ( this->Visited[p0] != this->Visited[p1] )
      {
      x[0] = origin[0] + i*spacing[0] + 0.5*spacing[0];
      ptId = points->InsertNextPoint(x);
      this->EdgeTable->InsertEdge(p0,p1,ptId);
      pointDescr->InsertValue(ptId, 1); //can't be smoothed

      edgeConn->InsertNextCell(2);
      edgeConn->InsertCellPoint(startId);
      edgeConn->InsertCellPoint(ptId);
      attrId = this->EdgeUseTable->InsertEdge(startId,ptId);
      this->EdgeUses->InsertValue(2*attrId, this->Visited[p0]);
      this->EdgeUses->InsertValue(2*attrId+1, -1);
      startId = ptId;
      }
    }
  edgeConn->InsertNextCell(2); //finish off the edge
  edgeConn->InsertCellPoint(startId);
  edgeConn->InsertCellPoint(1);
  attrId = this->EdgeUseTable->InsertEdge(startId,1);
  this->EdgeUses->InsertValue(2*attrId, this->Visited[dims[0]-1]);
  this->EdgeUses->InsertValue(2*attrId+1, -1);

  // Let's create perimeter edges - top x edge
  startId = 3;
  x[1] = origin[1] + (dims[1]-1)*spacing[1];
  for (i=0; i<(dims[0]-1); i++)
    {
    p0 = i + dims[0]*(dims[1]-1);
    p1 = p0 + 1;
    if ( this->Visited[p0] != this->Visited[p1] )
      {
      x[0] = origin[0] + i*spacing[0] + 0.5*spacing[0];
      ptId = points->InsertNextPoint(x);
      this->EdgeTable->InsertEdge(p0,p1,ptId);
      pointDescr->InsertValue(ptId, 1); //can't be smoothed

      edgeConn->InsertNextCell(2);
      edgeConn->InsertCellPoint(startId);
      edgeConn->InsertCellPoint(ptId);
      attrId = this->EdgeUseTable->InsertEdge(startId,ptId);
      this->EdgeUses->InsertValue(2*attrId, this->Visited[p0]);
      this->EdgeUses->InsertValue(2*attrId+1, -1);
      startId = ptId;
      }
    }
  edgeConn->InsertNextCell(2); //finish off the edge
  edgeConn->InsertCellPoint(startId);
  edgeConn->InsertCellPoint(2);
  attrId = this->EdgeUseTable->InsertEdge(startId,2);
  this->EdgeUses->InsertValue(2*attrId, this->Visited[dims[1]*dims[0]-1]);
  this->EdgeUses->InsertValue(2*attrId+1, -1);

  // Let's create perimeter edges - min y edge
  startId = 0;
  x[0] = origin[0];
  for (j=0; j<(dims[1]-1); j++)
    {
    p0 = j*dims[0];
    p1 = p0 + dims[0];
    if ( this->Visited[p0] != this->Visited[p1] )
      {
      x[1] = origin[1] + j*spacing[1] + 0.5*spacing[1];
      ptId = points->InsertNextPoint(x);
      this->EdgeTable->InsertEdge(p0,p1,ptId);
      pointDescr->InsertValue(ptId, 1); //can't be smoothed

      edgeConn->InsertNextCell(2);
      edgeConn->InsertCellPoint(startId);
      edgeConn->InsertCellPoint(ptId);
      attrId = this->EdgeUseTable->InsertEdge(startId,ptId);
      this->EdgeUses->InsertValue(2*attrId, this->Visited[p0]);
      this->EdgeUses->InsertValue(2*attrId+1, -1);
      startId = ptId;
      }
    }
  edgeConn->InsertNextCell(2); //finish off the edge
  edgeConn->InsertCellPoint(startId);
  edgeConn->InsertCellPoint(3);
  attrId = this->EdgeUseTable->InsertEdge(startId,3);
  this->EdgeUses->InsertValue(2*attrId, this->Visited[(dims[1]-1)*dims[0]]);
  this->EdgeUses->InsertValue(2*attrId+1, -1);

  // Let's create perimeter edges - max y edge
  startId = 1;
  x[0] = origin[0] + (dims[0]-1)*spacing[0];
  for (j=0; j<(dims[1]-1); j++)
    {
    p0 = j*dims[0] + (dims[0]-1);
    p1 = p0 + dims[0];
    if ( this->Visited[p0] != this->Visited[p1] )
      {
      x[1] = origin[1] + j*spacing[1] + 0.5*spacing[1];
      ptId = points->InsertNextPoint(x);
      this->EdgeTable->InsertEdge(p0,p1,ptId);
      pointDescr->InsertValue(ptId, 1); //can't be smoothed

      edgeConn->InsertNextCell(2);
      edgeConn->InsertCellPoint(startId);
      edgeConn->InsertCellPoint(ptId);
      attrId = this->EdgeUseTable->InsertEdge(startId,ptId);
      this->EdgeUses->InsertValue(2*attrId, this->Visited[p0]);
      this->EdgeUses->InsertValue(2*attrId+1, -1);
      startId = ptId;
      }
    }
  edgeConn->InsertNextCell(2); //finish off the edge
  edgeConn->InsertCellPoint(startId);
  edgeConn->InsertCellPoint(2);
  attrId = this->EdgeUseTable->InsertEdge(startId,2);
  this->EdgeUses->InsertValue(2*attrId, this->Visited[dims[1]*dims[0]-1]);
  this->EdgeUses->InsertValue(2*attrId+1, -1);

  // Loop over all edges generating intersection points and outer boundary
  // edge segments.
  //
  for (j=1; j<(dims[1]-1); j++) //loop over all x edges (except boundary)
    {
    x[1] = origin[1] + j*spacing[1];
    for (i=0; i<(dims[0]-1); i++)
      {
      p0 = i + j*dims[0];
      p1 = p0 + 1;
      if ( this->Visited[p0] != this->Visited[p1] )
        {
        x[0] = origin[0] + i*spacing[0] + 0.5*spacing[0];
        ptId = points->InsertNextPoint(x);
        this->EdgeTable->InsertEdge(p0,p1,ptId);
        pointDescr->InsertValue(ptId, 0);
        }
      }
    }

  for (i=1; i<(dims[0]-1); i++) //loop over all y edges (except boundary)
    {
    x[0] = origin[0] + i*spacing[0];
    for (j=0; j<(dims[1]-1); j++)
      {
      p0 = i + j*dims[0];
      p1 = i + (j+1)*dims[0];
      if ( this->Visited[p0] != this->Visited[p1] )
        {
        x[1] = origin[1] + j*spacing[1] + 0.5*spacing[1];
        ptId = points->InsertNextPoint(x);
        this->EdgeTable->InsertEdge(p0,p1,ptId);
        pointDescr->InsertValue(ptId, 0); //can be smoothed
        }
      }
    }

  // All intersection points are generated, now create edges. Use a clipping
  // approach to create line segments. Later we'll connect them into polylines.
  //
  for (j=0; j<(dims[1]-1); j++) //loop over all x edges
    {
    for (i=0; i<(dims[0]-1); i++)
      {
      edgeCount = 0;
      p0 = i   + j*dims[0];
      p1 = p0 + 1;
      p2 = i+1 + (j+1)*dims[0];
      p3 = p2 - 1;

      if ( (ptId=this->EdgeTable->IsEdge(p0,p1)) != -1 )
        {
        id[2*edgeCount] = p0; id[2*edgeCount+1] = p1;
        pts[edgeCount++] = ptId;
        }
      if ( (ptId=this->EdgeTable->IsEdge(p1,p2)) != -1 )
        {
        id[2*edgeCount] = p1; id[2*edgeCount+1] = p2;
        pts[edgeCount++] = ptId;
        }
      if ( (ptId=this->EdgeTable->IsEdge(p2,p3)) != -1 )
        {
        id[2*edgeCount] = p2; id[2*edgeCount+1] = p3;
        pts[edgeCount++] = ptId;
        }
      if ( (ptId=this->EdgeTable->IsEdge(p3,p0)) != -1 )
        {
        id[2*edgeCount] = p3; id[2*edgeCount+1] = p0;
        pts[edgeCount++] = ptId;
        }

      if ( edgeCount == 4 )
        {
        x[0] = origin[0] + i*spacing[0] + 0.5*spacing[0];
        x[1] = origin[1] + j*spacing[1] + 0.5*spacing[1];
        ptId = points->InsertNextPoint(x);
        pointDescr->InsertValue(ptId, 0); //intersection points are fixed

        edgeConn->InsertNextCell(2);
        edgeConn->InsertCellPoint(ptId);
        edgeConn->InsertCellPoint(pts[0]);
        attrId = this->EdgeUseTable->InsertEdge(ptId,pts[0]);
        this->EdgeUses->InsertValue(2*attrId, this->Visited[id[0]]);
        this->EdgeUses->InsertValue(2*attrId+1, this->Visited[id[1]]);

        edgeConn->InsertNextCell(2);
        edgeConn->InsertCellPoint(ptId);
        edgeConn->InsertCellPoint(pts[1]);
        attrId = this->EdgeUseTable->InsertEdge(ptId,pts[1]);
        this->EdgeUses->InsertValue(2*attrId, this->Visited[id[2]]);
        this->EdgeUses->InsertValue(2*attrId+1, this->Visited[id[3]]);

        edgeConn->InsertNextCell(2);
        edgeConn->InsertCellPoint(ptId);
        edgeConn->InsertCellPoint(pts[2]);
        attrId = this->EdgeUseTable->InsertEdge(ptId,pts[2]);
        this->EdgeUses->InsertValue(2*attrId, this->Visited[id[4]]);
        this->EdgeUses->InsertValue(2*attrId+1, this->Visited[id[5]]);

        edgeConn->InsertNextCell(2);
        edgeConn->InsertCellPoint(ptId);
        edgeConn->InsertCellPoint(pts[3]);
        attrId = this->EdgeUseTable->InsertEdge(ptId,pts[3]);
        this->EdgeUses->InsertValue(2*attrId, this->Visited[id[6]]);
        this->EdgeUses->InsertValue(2*attrId+1, this->Visited[id[7]]);
        }

      else if ( edgeCount == 3 )
        {
        x[0] = origin[0] + i*spacing[0] + 0.5*spacing[0];
        x[1] = origin[1] + j*spacing[1] + 0.5*spacing[1];
        ptId = points->InsertNextPoint(x);
        pointDescr->InsertValue(ptId, 0); //intersection points are fixed

        edgeConn->InsertNextCell(2);
        edgeConn->InsertCellPoint(ptId);
        edgeConn->InsertCellPoint(pts[0]);
        attrId = this->EdgeUseTable->InsertEdge(ptId, pts[0]);
        this->EdgeUses->InsertValue(2*attrId, this->Visited[id[0]]);
        this->EdgeUses->InsertValue(2*attrId+1, this->Visited[id[1]]);

        edgeConn->InsertNextCell(2);
        edgeConn->InsertCellPoint(ptId);
        edgeConn->InsertCellPoint(pts[1]);
        attrId = this->EdgeUseTable->InsertEdge(ptId, pts[1]);
        this->EdgeUses->InsertValue(2*attrId, this->Visited[id[2]]);
        this->EdgeUses->InsertValue(2*attrId+1, this->Visited[id[3]]);

        edgeConn->InsertNextCell(2);
        edgeConn->InsertCellPoint(ptId);
        edgeConn->InsertCellPoint(pts[2]);
        attrId = this->EdgeUseTable->InsertEdge(ptId, pts[2]);
        this->EdgeUses->InsertValue(2*attrId, this->Visited[id[4]]);
        this->EdgeUses->InsertValue(2*attrId+1, this->Visited[id[5]]);
        }

      else if ( edgeCount == 2 )
        {
        edgeConn->InsertNextCell(2);
        edgeConn->InsertCellPoint(pts[0]);
        edgeConn->InsertCellPoint(pts[1]);
        attrId = this->EdgeUseTable->InsertEdge(pts[0],pts[1]);
        this->EdgeUses->InsertValue(2*attrId, this->Visited[id[0]]);
        this->EdgeUses->InsertValue(2*attrId+1, this->Visited[id[1]]);
        }

      else if ( edgeCount == 1 )
        {
        vtkErrorMacro(<<"Bad mojo");
        return 0;
        }
      }
    }

  // Cleanup
  this->EdgeUseTable->Delete();
  this->EdgeTable->Delete();

  return 0;
}

void vtkImageToPolyDataFilter::BuildPolygons(vtkUnsignedCharArray *vtkNotUsed(pointDescr),
                                             vtkPolyData *edges, int numPolys,
                                             vtkUnsignedCharArray *polyColors)
{
  vtkPoints *points = edges->GetPoints();
  vtkIdType numPts = points->GetNumberOfPoints(), ptId;
  int i, j, k, *polyId, *polyId2, edgeId;
  vtkIdType *cells, *pts, *cells2, npts, cellId;
  int numPolyPts, p1, p2;
  unsigned short ncells, ncells2;
  unsigned char *polyVisited, *ptr;
  vtkCellArray *newPolys;

  // Make sure we can topological info
  edges->BuildLinks();

  // Mark all polygons as unvisited
  polyVisited = new unsigned char [numPolys];
  for (i=0; i<numPolys; i++)
    {
    polyVisited[i] = 0;
    }

  // Create connectivity array for polygon definition
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys,25));

  // Loop over all edge points tracking around each polygon
  for (ptId=0; ptId<numPts; ptId++)
    {
    edges->GetPointCells(ptId, ncells, cells);
    if (ncells < 2)
      {
      vtkErrorMacro(<<"Bad mojo");
      return;
      }
    //for each edge, walk around polygon (if not visited before)
    for (i=0; i<ncells; i++)
      {
      edgeId = cells[i];
      polyId = this->EdgeUses->GetPointer(2*edgeId);
      for (j=0; j<2; j++)
        {
        if ( polyId[j] != -1 && !polyVisited[polyId[j]] )
          {//build loop
          polyVisited[polyId[j]] = 1;
          numPolyPts = 1;
          cellId = newPolys->InsertNextCell(0); //will update count later
          newPolys->InsertCellPoint(ptId);

          // Update polygonal color
          ptr = this->PolyColors->GetPointer(3*polyId[j]);
          polyColors->SetValue(3*cellId, ptr[0]); //assign poly color
          polyColors->SetValue(3*cellId+1, ptr[1]);
          polyColors->SetValue(3*cellId+2, ptr[2]);

          p1 = ptId;
          while ( true )
            {
            edges->GetCellPoints(edgeId, npts, pts);
            p2 = (pts[0] != p1 ? pts[0] : pts[1]);
            if (p2 == ptId)
              {
              break;
              }

            newPolys->InsertCellPoint(p2);
            numPolyPts++;
            edges->GetPointCells(p2, ncells2, cells2);
            if (ncells < 2)
              {
              vtkErrorMacro(<<"Bad mojo");
              return;
              }
            for (k=0; k<ncells2; k++)
              {
              polyId2 = this->EdgeUses->GetPointer(2*cells2[k]);
              if ( cells2[k] != edgeId &&
                   (polyId2[0] == polyId[j] || polyId2[1] == polyId[j]) )
                {
                p1 = p2;
                edgeId = cells2[k];
                break;
                }
              }
            }//while not completed loop
          newPolys->UpdateCellCount(numPolyPts);

          }//if polygon not yet visited
        }//for each use of edge by polygon (at most 2 polygons)
      }//for each edge connected to this point
    }//for all points in edge list

  edges->SetPolys(newPolys);
  newPolys->Delete();
  this->EdgeUses->Delete();
  delete [] polyVisited;
}

void vtkImageToPolyDataFilter::SmoothEdges(vtkUnsignedCharArray *pointDescr,
                                           vtkPolyData *edges)
{

  vtkPoints *points=edges->GetPoints();
  vtkIdType numPts=points->GetNumberOfPoints(), ptId;
  int i, iterNum;
  int connId;
  double x[3], xconn[3], xave[3], factor;
  unsigned short int ncells;
  vtkIdType *cells, *pts, npts;


  // For each smoothing operation, loop over points. Points that can be
  // smoothed are moved in the direction of the average of their neighbor
  // points.
  for ( iterNum=0; iterNum<this->NumberOfSmoothingIterations; iterNum++ )
    {
    if ( (iterNum % 2) ) //alternate smoothing direction
      {
      factor = -0.331;
      }
    else
      {
      factor = 0.330;
      }

    for (ptId=0; ptId<numPts; ptId++)
      {
      if ( pointDescr->GetValue(ptId) == 0 ) //can smooth
        {
        points->GetPoint(ptId, x);
        edges->GetPointCells(ptId, ncells, cells);
        xave[0] = xave[1] = xave[2] = 0.0;
        for (i=0; i<ncells; i++)
          {
          edges->GetCellPoints(cells[i], npts, pts);
          if (pts[0] != ptId)
            {
            connId = pts[0];
            }
          else if (npts > 1)
            {
            connId = pts[1];
            }
          else
            {
            vtkErrorMacro("Bad cell in smoothing operation");
            connId = pts[0];
            }
          points->GetPoint(connId, xconn);
          xave[0] += xconn[0]; xave[1] += xconn[1]; xave[2] += xconn[2];
          }
        if ( ncells > 0 )
          {
          xave[0] /= ncells; xave[1] /= ncells; xave[2] /= ncells;
          x[0] = x[0] + factor * (xave[0] - x[0]);
          x[1] = x[1] + factor * (xave[1] - x[1]);
          x[2] = x[2] + factor * (xave[2] - x[2]);
          points->SetPoint(ptId, x);
          }

        }//if smoothable point
      }//for all points
    }//for all smoothing operations
}


// Remove points that are nearly co-linear to reduce the total point count
//
void vtkImageToPolyDataFilter::DecimateEdges(vtkPolyData *edges,
                                             vtkUnsignedCharArray *pointDescr,
                                             double tol2)
{
  vtkPoints *points=edges->GetPoints();
  vtkIdType numPts=points->GetNumberOfPoints(), ptId, prevId, nextId;
  vtkIdType npts;
  double x[3], xPrev[3], xNext[3];
  unsigned short int ncells;
  vtkIdType *cells, *pts;

  // Loop over all points, finding those that are connected to just two
  // edges. If the point is colinear to the previous and next edge point,
  // then mark it as deleted.
  for (ptId=0; ptId<numPts; ptId++)
    {
    if ( pointDescr->GetValue(ptId) == 0 )
      {
      points->GetPoint(ptId, x);
      edges->GetPointCells(ptId, ncells, cells);
      if ( ncells == 2 )
        {
        edges->GetCellPoints(cells[0], npts, pts);
        prevId = (pts[0] != ptId ? pts[0] : pts[1]);
        points->GetPoint(prevId, xPrev);

        edges->GetCellPoints(cells[1], npts, pts);
        nextId = (pts[0] != ptId ? pts[0] : pts[1]);
        points->GetPoint(nextId, xNext);

        if ( vtkLine::DistanceToLine(x, xPrev, xNext) <= tol2 )
          {
          pointDescr->SetValue(ptId, 2); //mark deleted
          }
        }
      } //if manifold
    } //for all points
}
