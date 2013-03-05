/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowToImageFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWindowToImageFilter.h"

#include "vtkMath.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCoordinate.h"
#include "vtkActor2D.h"
#include "vtkActor2DCollection.h"
#include <vector>

#define BORDER_PIXELS 2

vtkStandardNewMacro(vtkWindowToImageFilter);

class vtkWTI2DHelperClass
{
public:
  // maintain a list of 2D actors
  vtkActor2DCollection *StoredActors;
  // maintain lists of their vtkCoordinate objects
  vtkCollection        *Coord1s;
  vtkCollection        *Coord2s;
  // Store the display coords for adjustment during tiling
  std::vector< std::pair<int, int> > Coords1;
  std::vector< std::pair<int, int> > Coords2;
  //
  vtkWTI2DHelperClass()
    {
    this->StoredActors = vtkActor2DCollection::New();
    this->Coord1s = vtkCollection::New();
    this->Coord2s = vtkCollection::New();
    }
  ~vtkWTI2DHelperClass()
    {
    this->Coord1s->RemoveAllItems();
    this->Coord2s->RemoveAllItems();
    this->StoredActors->RemoveAllItems();
    this->Coord1s->Delete();
    this->Coord2s->Delete();
    this->StoredActors->Delete();
    }
};

//----------------------------------------------------------------------------
vtkWindowToImageFilter::vtkWindowToImageFilter()
{
  this->Input = NULL;
  this->Magnification = 1;
  this->ReadFrontBuffer = 1;
  this->ShouldRerender = 1;
  this->Viewport[0] = 0;
  this->Viewport[1] = 0;
  this->Viewport[2] = 1;
  this->Viewport[3] = 1;
  this->InputBufferType = VTK_RGB;
  this->FixBoundary = false;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->StoredData = new vtkWTI2DHelperClass;
}

//----------------------------------------------------------------------------
vtkWindowToImageFilter::~vtkWindowToImageFilter()
{
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
  delete this->StoredData;
}

//----------------------------------------------------------------------------
vtkImageData* vtkWindowToImageFilter::GetOutput()
{
  return vtkImageData::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void vtkWindowToImageFilter::SetInput(vtkWindow *input)
{
  if (input != this->Input)
    {
    if (this->Input) {this->Input->UnRegister(this);}
    this->Input = input;
    if (this->Input) {this->Input->Register(this);}
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkWindowToImageFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Input )
    {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Input: (none)\n";
    }
  os << indent << "ReadFrontBuffer: " << this->ReadFrontBuffer << "\n";
  os << indent << "Magnification: " << this->Magnification << "\n";
  os << indent << "ShouldRerender: " << this->ShouldRerender << "\n";
  os << indent << "Viewport: " << this->Viewport[0] << "," << this->Viewport[1]
     << "," << this->Viewport[2] << "," << this->Viewport[3] << "\n";
  os << indent << "InputBufferType: " << this->InputBufferType << "\n";
  os << indent << "FixBoundary: " << this->FixBoundary << endl;
}


//----------------------------------------------------------------------------
// This method returns the largest region that can be generated.
void vtkWindowToImageFilter::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  if (this->Input == NULL )
    {
    vtkErrorMacro(<<"Please specify a renderer as input!");
    return;
    }

  if(this->Magnification > 1 &&
     (this->Viewport[0] != 0 || this->Viewport[1] != 0 ||
      this->Viewport[2] != 1 || this->Viewport[3] != 1))
    {
    vtkWarningMacro(<<"Viewport extents are not used when Magnification > 1");
    this->Viewport[0] = 0;
    this->Viewport[1] = 0;
    this->Viewport[2] = 1;
    this->Viewport[3] = 1;
    }


  // set the extent
  int *size = this->Input->GetSize();
  int wExtent[6];
  wExtent[0]= 0;
  wExtent[1] = (int(this->Viewport[2] * size[0] + 0.5)-
    int(this->Viewport[0] * size[0])) * this->Magnification - 1;
  wExtent[2] = 0;
  wExtent[3] = (int(this->Viewport[3] * size[1] + 0.5)-
    int(this->Viewport[1] * size[1])) * this->Magnification - 1;
  wExtent[4] = 0;
  wExtent[5] = 0;

  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent, 6);

  switch( this->InputBufferType )
    {
    case VTK_RGB:
      vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 3);
      break;
    case VTK_RGBA:
      vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 4);
      break;
    case VTK_ZBUFFER:
      vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
      break;
    default:
      // VTK_RGB configuration by default
      vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 3);
      break;
    }
}

//----------------------------------------------------------------------------
int vtkWindowToImageFilter::ProcessRequest(vtkInformation* request,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    this->RequestData(request, inputVector, outputVector);
    return 1;
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    this->RequestInformation(request, inputVector, outputVector);
    return 1;
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}


//----------------------------------------------------------------------------
// This function reads a region from a file.  The regions extent/axes
// are assumed to be the same as the file extent/order.
void vtkWindowToImageFilter::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *out =
    vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  out->SetExtent(
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
  out->AllocateScalars(outInfo);

  if (!this->Input)
    {
    return;
    }

  int outIncrY;
  int size[2],winsize[2];
  int idxY, rowSize;
  int i;

  if (! ((out->GetScalarType() == VTK_UNSIGNED_CHAR &&
        (this->InputBufferType == VTK_RGB || this->InputBufferType == VTK_RGBA)) ||
        (out->GetScalarType() == VTK_FLOAT && this->InputBufferType == VTK_ZBUFFER)))
    {
    vtkErrorMacro("mismatch in scalar types!");
    return;
    }

  // get the size of the render window
  winsize[0] = this->Input->GetSize()[0];
  winsize[1] = this->Input->GetSize()[1];

  size[0] = int(this->Viewport[2]* winsize[0] + 0.5)
            - int(this->Viewport[0]* winsize[0]);
  size[1] = int(this->Viewport[3]* winsize[1] + 0.5)
            - int(this->Viewport[1]* winsize[1]);

  rowSize = size[0]*out->GetNumberOfScalarComponents();
  outIncrY = size[0]*this->Magnification*out->GetNumberOfScalarComponents();

  float *viewAngles;
  double *windowCenters;
  vtkRenderWindow *renWin = vtkRenderWindow::SafeDownCast(this->Input);
  if (!renWin)
    {
    vtkWarningMacro(
      "The window passed to window to image should be a RenderWindow or one of its subclasses");
    return;
    }

  vtkRendererCollection *rc = renWin->GetRenderers();
  vtkRenderer *aren;
  vtkCamera *cam;
  int numRenderers = rc->GetNumberOfItems();

  // for each renderer
  vtkCamera **cams = new vtkCamera *[numRenderers];
  viewAngles = new float [numRenderers];
  windowCenters = new double [numRenderers*2];
  double *parallelScale = new double [numRenderers];
  vtkCollectionSimpleIterator rsit;
  rc->InitTraversal(rsit);
  for (i = 0; i < numRenderers; ++i)
    {
    aren = rc->GetNextRenderer(rsit);
    cams[i] = aren->GetActiveCamera();
    cams[i]->Register(this);
    cams[i]->GetWindowCenter(windowCenters+i*2);
    viewAngles[i] = cams[i]->GetViewAngle();
    parallelScale[i] = cams[i]->GetParallelScale();
    cam = cams[i]->NewInstance();
    cam->SetPosition(cams[i]->GetPosition());
    cam->SetFocalPoint(cams[i]->GetFocalPoint());
    cam->SetViewUp(cams[i]->GetViewUp());
    cam->SetClippingRange(cams[i]->GetClippingRange());
    cam->SetParallelProjection(cams[i]->GetParallelProjection());
    cam->SetFocalDisk(cams[i]->GetFocalDisk());
    cam->SetUserTransform(cams[i]->GetUserTransform());
    cam->SetUseHorizontalViewAngle(cams[i]->GetUseHorizontalViewAngle());
    cam->SetViewShear(cams[i]->GetViewShear());
    cam->SetModelTransformMatrix(cams[i]->GetModelTransformMatrix());
    cam->SetEyeTransformMatrix(cams[i]->GetEyeTransformMatrix());
    cam->SetUseOffAxisProjection(cams[i]->GetUseOffAxisProjection());
    cam->SetScreenBottomLeft(cams[i]->GetScreenBottomLeft());
    cam->SetScreenBottomRight(cams[i]->GetScreenBottomRight());
    cam->SetScreenTopRight(cams[i]->GetScreenTopRight());
    cam->SetEyeSeparation(cams[i]->GetEyeSeparation());
    double eyePos[3];
    cams[i]->GetEyePosition(eyePos);
    cam->SetEyePosition(eyePos);
    aren->SetActiveCamera(cam);
    }

  // render each of the tiles required to fill this request
  this->Input->SetTileScale(this->Magnification);
  this->Input->GetSize();

  //this->Rescale2DActors();
  int x, y;

  int num_iterations = this->Magnification;
  bool overlap_viewports = false;
  if (this->Magnification > 1 && this->FixBoundary &&
    winsize[0] >= 50 && winsize[1] >= 50)
    {
    num_iterations++;
    overlap_viewports = true;
    }

  // Precompute the tile viewport for each iteration.
  double *viewports = new double[4*num_iterations*num_iterations];
  for (y = 0; y < num_iterations; y++)
    {
    for (x = 0; x < num_iterations; x++)
      {
      double* cur_viewport = &viewports[ (num_iterations*y + x)*4 ];
      cur_viewport[0] = static_cast<double>(x)/this->Magnification;
      cur_viewport[1] = static_cast<double>(y)/this->Magnification;
      cur_viewport[2] = (x+1.0)/this->Magnification;
      cur_viewport[3] = (y+1.0)/this->Magnification;

      if (overlap_viewports)
        {
        if (x > 0 && x < num_iterations-1)
          {
          cur_viewport[0] -= x * (BORDER_PIXELS * 2.0)/winsize[0];
          cur_viewport[2] -= x * (BORDER_PIXELS * 2.0)/winsize[0];
          }
        if (x == num_iterations -1)
          {
          cur_viewport[0] = static_cast<double>(x-1)/this->Magnification;
          cur_viewport[2] = static_cast<double>(x)/this->Magnification;

          }
        if (y > 0 && y < num_iterations-1)
          {
          cur_viewport[1] -= y * (BORDER_PIXELS * 2.0)/winsize[1];
          cur_viewport[3] -= y * (BORDER_PIXELS * 2.0)/winsize[1];
          }
        if (y == num_iterations-1)
          {
          cur_viewport[1] = static_cast<double>(y-1)/this->Magnification;
          cur_viewport[3] = static_cast<double>(y)/this->Magnification;
          }
        }
      }
    }

  for (y = 0; y < num_iterations; y++)
    {
    for (x = 0; x < num_iterations; x++)
      {
      // setup the Window ivars
      double* cur_viewport = &viewports[ (num_iterations*y + x)*4 ];
      this->Input->SetTileViewport(cur_viewport);
      double *tvp = this->Input->GetTileViewport();

      // for each renderer, setup camera
      rc->InitTraversal(rsit);
      for (i = 0; i < numRenderers; ++i)
        {
        aren = rc->GetNextRenderer(rsit);
        cam = aren->GetActiveCamera();
        double *vp = aren->GetViewport();
        double visVP[4];
        visVP[0] = (vp[0] < tvp[0]) ? tvp[0] : vp[0];
        visVP[0] = (visVP[0] > tvp[2]) ? tvp[2] : visVP[0];
        visVP[1] = (vp[1] < tvp[1]) ? tvp[1] : vp[1];
        visVP[1] = (visVP[1] > tvp[3]) ? tvp[3] : visVP[1];
        visVP[2] = (vp[2] > tvp[2]) ? tvp[2] : vp[2];
        visVP[2] = (visVP[2] < tvp[0]) ? tvp[0] : visVP[2];
        visVP[3] = (vp[3] > tvp[3]) ? tvp[3] : vp[3];
        visVP[3] = (visVP[3] < tvp[1]) ? tvp[1] : visVP[3];

        // compute magnification
        double mag = (visVP[3] - visVP[1])/(vp[3] - vp[1]);
        // compute the delta
        double deltax = (visVP[2] + visVP[0])/2.0 - (vp[2] + vp[0])/2.0;
        double deltay = (visVP[3] + visVP[1])/2.0 - (vp[3] + vp[1])/2.0;
        // scale by original window size
        if (visVP[2] - visVP[0] > 0)
          {
          deltax = 2.0*deltax/(visVP[2] - visVP[0]);
          }
        if (visVP[3] - visVP[1] > 0)
          {
          deltay = 2.0*deltay/(visVP[3] - visVP[1]);
          }
        cam->SetWindowCenter(windowCenters[i*2]+deltax,windowCenters[i*2+1]+deltay);
        //cam->SetViewAngle(atan(tan(viewAngles[i]*vtkMath::Pi()/360.0)*mag)  //FIXME
        cam->SetViewAngle(asin(sin(viewAngles[i]*vtkMath::Pi()/360.0)*mag)
                          * 360.0 / vtkMath::Pi());
        cam->SetParallelScale(parallelScale[i]*mag);
        }

      // Shift 2d actors just before rendering
      //this->Shift2DActors(size[0]*x, size[1]*y);
      // now render the tile and get the data
      if (this->ShouldRerender || num_iterations > 1)
        {
        // if interactor is present, trigger render through interactor. This
        // allows for custom applications that provide interactors that
        // customize rendering e.g. ParaView.
        if (renWin->GetInteractor())
          {
          renWin->GetInteractor()->Render();
          }
        else
          {
          this->Input->Render();
          }
        }
      this->Input->MakeCurrent();

      int buffer = this->ReadFrontBuffer;
      if(!this->Input->GetDoubleBuffer())
        {
        buffer = 1;
        }
      if (this->InputBufferType == VTK_RGB || this->InputBufferType == VTK_RGBA)
        {
        unsigned char *pixels, *pixels1, *outPtr;
        if (this->InputBufferType == VTK_RGB)
          {
          pixels =
            this->Input->GetPixelData(int(this->Viewport[0]* winsize[0]),
                                      int(this->Viewport[1]* winsize[1]),
                                      int(this->Viewport[2]* winsize[0] + 0.5) - 1,
                                      int(this->Viewport[3]* winsize[1] + 0.5) - 1, buffer);
          }
        else
          {
          pixels =
            renWin->GetRGBACharPixelData(int(this->Viewport[0]* winsize[0]),
                                         int(this->Viewport[1]* winsize[1]),
                                         int(this->Viewport[2]* winsize[0] + 0.5) - 1,
                                         int(this->Viewport[3]* winsize[1] + 0.5) - 1, buffer);

          }

        pixels1 = pixels;

        // now write the data to the output image
        if (overlap_viewports)
          {
          int xpos = int(cur_viewport[0]*size[0]*this->Magnification + 0.5);
          int ypos = int(cur_viewport[1]*size[1]*this->Magnification + 0.5);

          outPtr = static_cast<unsigned char *>(
            out->GetScalarPointer(xpos,ypos, 0));

          // We skip padding pixels around internal borders.
          int ncomp = out->GetNumberOfScalarComponents();
          int start_x_offset = (x != 0)? BORDER_PIXELS : 0;
          int end_x_offset = (x != num_iterations-1 && x!=0)? BORDER_PIXELS : 0;
          int start_y_offset = (y != 0)? BORDER_PIXELS : 0;
          int end_y_offset = (y != num_iterations-1)? BORDER_PIXELS : 0;
          start_x_offset *= ncomp;
          end_x_offset *= ncomp;
          for (idxY = 0; idxY < size[1]; idxY++)
            {
            if (idxY >= start_y_offset && idxY < size[1] - end_y_offset)
              {
              memcpy(outPtr + start_x_offset, pixels1 + start_x_offset,
                rowSize - (start_x_offset + end_x_offset));
              }
            outPtr += outIncrY;
            pixels1 += rowSize;
            }
          }
        else
          {
          outPtr = static_cast<unsigned char *>(
            out->GetScalarPointer(x*size[0],y*size[1], 0));

          for (idxY = 0; idxY < size[1]; idxY++)
            {
            memcpy(outPtr,pixels1,rowSize);
            outPtr += outIncrY;
            pixels1 += rowSize;
            }

          }

        // free the memory
        delete [] pixels;
        }
      else
        { // VTK_ZBUFFER
        float *pixels, *pixels1, *outPtr;
        pixels = renWin->GetZbufferData(int(this->Viewport[0]* winsize[0]),
                                        int(this->Viewport[1]* winsize[1]),
                                        int(this->Viewport[2]* winsize[0] + 0.5) - 1,
                                        int(this->Viewport[3]* winsize[1] + 0.5) - 1);

        pixels1 = pixels;

        // now write the data to the output image
        outPtr =
          static_cast<float *>(out->GetScalarPointer(x*size[0],y*size[1], 0));
        for (idxY = 0; idxY < size[1]; idxY++)
          {
          memcpy(outPtr,pixels1,rowSize*sizeof(float));
          outPtr += outIncrY;
          pixels1 += rowSize;
          }

        // free the memory
        delete [] pixels;
        }
      }
    }

  // restore settings
  // for each renderer
  rc->InitTraversal(rsit);
  for (i = 0; i < numRenderers; ++i)
    {
    aren = rc->GetNextRenderer(rsit);
    // store the old view angle & set the new
    cam = aren->GetActiveCamera();
    aren->SetActiveCamera(cams[i]);
    cams[i]->UnRegister(this);
    cam->Delete();
    }
  delete [] viewAngles;
  delete [] windowCenters;
  delete [] parallelScale;
  delete [] cams;
  delete [] viewports;

  // render each of the tiles required to fill this request
  this->Input->SetTileScale(1);
  this->Input->SetTileViewport(0.0,0.0,1.0,1.0);
  this->Input->GetSize();
  // restore every 2d actors
  //this->Restore2DActors();
}

//----------------------------------------------------------------------------
// On each tile we must subtract the origin of each actor to ensure
// it appears in the corrrect relative location
void vtkWindowToImageFilter::Restore2DActors()
{
  vtkActor2D            *actor;
  vtkCoordinate         *c1, *c2;
  vtkCoordinate         *n1, *n2;
  int i;
  //
  for (this->StoredData->StoredActors->InitTraversal(), i=0;
    (actor = this->StoredData->StoredActors->GetNextItem()); i++)
    {
    c1 = actor->GetPositionCoordinate();
    c2 = actor->GetPosition2Coordinate();
    n1 = vtkCoordinate::SafeDownCast(this->StoredData->Coord1s->GetItemAsObject(i));
    n2 = vtkCoordinate::SafeDownCast(this->StoredData->Coord2s->GetItemAsObject(i));
    c1->SetCoordinateSystem(n1->GetCoordinateSystem());
    c1->SetReferenceCoordinate(n1->GetReferenceCoordinate());
    c1->SetReferenceCoordinate(n1->GetReferenceCoordinate());
    c1->SetValue(n1->GetValue());
    c2->SetCoordinateSystem(n2->GetCoordinateSystem());
    c2->SetReferenceCoordinate(n2->GetReferenceCoordinate());
    c2->SetValue(n2->GetValue());
    }
  this->StoredData->Coord1s->RemoveAllItems();
  this->StoredData->Coord2s->RemoveAllItems();
  this->StoredData->StoredActors->RemoveAllItems();
}

//----------------------------------------------------------------------------
void vtkWindowToImageFilter::Rescale2DActors()
{
  vtkActor2D            *actor;
  vtkProp               *aProp;
  vtkRenderer           *aren;
  vtkPropCollection     *pc;
  vtkRendererCollection *rc;
  vtkCoordinate         *c1, *c2;
  vtkCoordinate         *n1, *n2;
  int                   *p1, *p2;
  double                d1[3], d2[3];
  //
  vtkRenderWindow *renWin = vtkRenderWindow::SafeDownCast(this->Input);
  rc = renWin->GetRenderers();
  for (rc->InitTraversal(); (aren = rc->GetNextItem()); )
    {
    pc = aren->GetViewProps();
    if (pc)
      {
      for ( pc->InitTraversal(); (aProp = pc->GetNextProp()); )
        {
        actor = vtkActor2D::SafeDownCast((aProp));
        if (actor)
          {
          // put the actor in our list for retrieval later
          this->StoredData->StoredActors->AddItem(actor);
          // Copy all existing coordinate stuff
          n1 = actor->GetPositionCoordinate();
          n2 = actor->GetPosition2Coordinate();
          c1 = vtkCoordinate::New();
          c2 = vtkCoordinate::New();
          c1->SetCoordinateSystem(n1->GetCoordinateSystem());
          c1->SetReferenceCoordinate(n1->GetReferenceCoordinate());
          c1->SetReferenceCoordinate(n1->GetReferenceCoordinate());
          c1->SetValue(n1->GetValue());
          c2->SetCoordinateSystem(n2->GetCoordinateSystem());
          c2->SetReferenceCoordinate(n2->GetReferenceCoordinate());
          c2->SetValue(n2->GetValue());
          this->StoredData->Coord1s->AddItem(c1);
          this->StoredData->Coord2s->AddItem(c2);
          c1->Delete();
          c2->Delete();
          // work out the position in new magnified pixels
          p1 = n1->GetComputedDisplayValue(aren);
          p2 = n2->GetComputedDisplayValue(aren);
          d1[0] = p1[0]*this->Magnification;
          d1[1] = p1[1]*this->Magnification;
          d1[2] = 0.0;
          d2[0] = p2[0]*this->Magnification;
          d2[1] = p2[1]*this->Magnification;
          d2[2] = 0.0;
          this->StoredData->Coords1.push_back(
            std::pair<int, int>(static_cast<int>(d1[0]), static_cast<int>(d1[1])) );
          this->StoredData->Coords2.push_back(
            std::pair<int, int>(static_cast<int>(d2[0]), static_cast<int>(d2[1])) );
          // Make sure they have no dodgy offsets
          n1->SetCoordinateSystemToDisplay();
          n2->SetCoordinateSystemToDisplay();
          n1->SetReferenceCoordinate(NULL);
          n2->SetReferenceCoordinate(NULL);
          n1->SetValue(d1[0], d1[1]);
          n2->SetValue(d2[0], d2[1]);
          //
          }
        }
      }
    }
}
//----------------------------------------------------------------------------
// On each tile we must subtract the origin of each actor to ensure
// it appears in the correct relative location
void vtkWindowToImageFilter::Shift2DActors(int x, int y)
{
  vtkActor2D    *actor;
  vtkCoordinate *c1, *c2;
  double        d1[3], d2[3];
  int           i;
  //
  for (this->StoredData->StoredActors->InitTraversal(), i=0;
    (actor = this->StoredData->StoredActors->GetNextItem()); i++)
    {
    c1 = actor->GetPositionCoordinate();
    c2 = actor->GetPosition2Coordinate();
    c1->GetValue(d1);
    c2->GetValue(d2);
    d1[0] = this->StoredData->Coords1[i].first  - x;
    d1[1] = this->StoredData->Coords1[i].second - y + 1;
    d2[0] = this->StoredData->Coords2[i].first  - x;
    d2[1] = this->StoredData->Coords2[i].second - y + 1;
    c1->SetValue(d1);
    c2->SetValue(d2);
    }
}

//----------------------------------------------------------------------------
int vtkWindowToImageFilter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}
