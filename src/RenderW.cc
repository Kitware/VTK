/*=========================================================================

  Program:   Visualization Library
  Module:    RenderW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "RenderW.hh"
#include "Interact.hh"

// Description:
// Construct object with screen size 300x300, borders turned on, position
// at (0,0), and double buffering turned on.
vlRenderWindow::vlRenderWindow()
{
  Size[0] = Size[1] = 300;
  Position[0] = Position[1] = 0;
  Borders = 1;
  FullScreen = 0;
  OldScreen[0] = OldScreen[1] = 0;
  OldScreen[2] = OldScreen[3] = 300;
  OldScreen[4] = 1;
  Mapped = 0;
  DoubleBuffer = 1;
  StereoRender = 0;
  StereoStatus = 0;
  Interactor = NULL;
  strcpy(this->Name,"Visualization Library");
  AAFrames = 0;
  AABuffer = NULL;
  FDFrames = 0;
  FDBuffer = NULL;
  SubFrames = 0;
  SubBuffer = NULL;
  CurrentSubFrame = 0;
  ResultFrame = NULL;
  FileName = NULL;
}

// Description:
// Ask each renderer to render an image. Synchronize this process.
void vlRenderWindow::Render()
{
  int i;

  vlDebugMacro(<< "Starting Render Method.\n");

  if ( this->Interactor && ! this->Interactor->GetInitialized() )
    this->Interactor->Initialize();

  // handle any sub frames
  if (this->SubFrames)
    {
    int *size;
    int x,y;
    int r,g,b;
    unsigned char *p1;

    // get the size
    size = this->GetSize();

    // free old and get new memory on first sub frame
    if (!this->CurrentSubFrame)
      {
      if (this->SubBuffer) delete SubBuffer;
      this->SubBuffer = new unsigned char *[this->SubFrames];
      }

    // draw the images
    this->DoAARender();

    // get the pixels for accumulation
    if (this->ResultFrame)
      {
      this->SubBuffer[this->CurrentSubFrame] = this->ResultFrame;
      this->ResultFrame = NULL;
      }
    else
      {
      this->SubBuffer[this->CurrentSubFrame] 
	= this->GetPixelData(0,0,size[0]-1,size[1]-1);
      }

    // if this is the last sub frame then merge the images
    this->CurrentSubFrame++;
    if (this->CurrentSubFrame == this->SubFrames)
      {
      this->CurrentSubFrame = 0;
      this->ResultFrame = this->SubBuffer[0];
      
      // now accumulate the images 
      p1 = this->ResultFrame;
      for (y = 0; y < size[1]; y++)
	{
	for (x = 0; x < size[0]; x++)
	  {
	  r = *p1; 
	  g = *(p1 + 1);  
	  b = *(p1 + 2);  
	  for (i = 1; i < this->SubFrames; i++)
	    {
	    r += this->SubBuffer[i][(y*size[0] + x)*3];
	    g += this->SubBuffer[i][(y*size[0] + x)*3 + 1];
	    b += this->SubBuffer[i][(y*size[0] + x)*3 + 2];
	    }
	  r /= this->SubFrames;
	  g /= this->SubFrames;
	  b /= this->SubFrames;
	  *p1 = r; p1++;
	  *p1 = g; p1++;
	  *p1 = b; p1++;
	  }
	}
      for (i = 1; i < this->SubFrames; i++)
	{
	delete this->SubBuffer[i];
	}
      delete this->SubBuffer;
      this->CopyResultFrame();
      }
    }
  else
    {
    this->DoAARender();
    this->CopyResultFrame();
    }
}

// Description:
// Ask each renderer to render an aa image. Synchronize this process.
void vlRenderWindow::DoAARender()
{
  int i;

  // handle any anti aliasing
  if (this->AAFrames)
    {
    int *size;
    int x,y;
    int r,g,b;
    unsigned char *p1;
    vlRenderer *aren;
    vlCamera *acam;
    float *dpoint;
    float offsets[2];
    float origfocus[4];
    float worldOffset[3];

    // get the size
    size = this->GetSize();

    // free old and get new memory
    if (this->AABuffer) delete AABuffer;
    this->AABuffer = new unsigned char *[this->AAFrames];
    origfocus[3] = 1.0;

    for (i = 0; i < AAFrames; i++)
      {
      // jitter the cameras
      offsets[0] = drand48() - 0.5;
      offsets[1] = drand48() - 0.5;

      for (this->Renderers.InitTraversal(); 
	   aren = this->Renderers.GetNextItem(); )
	{
	acam = aren->GetActiveCamera();

	// calculate the amount to jitter
	memcpy(origfocus,acam->GetFocalPoint(),12);
	aren->SetWorldPoint(origfocus);
	aren->WorldToDisplay();
	dpoint = aren->GetDisplayPoint();
	aren->SetDisplayPoint(dpoint[0] + offsets[0],
			      dpoint[1] + offsets[1],
			      dpoint[2]);
	aren->DisplayToWorld();
	dpoint = aren->GetWorldPoint();
	dpoint[0] /= dpoint[3];
	dpoint[1] /= dpoint[3];
	dpoint[2] /= dpoint[3];
	acam->SetFocalPoint(dpoint);

	worldOffset[0] = dpoint[0] - origfocus[0];
	worldOffset[1] = dpoint[1] - origfocus[1];
	worldOffset[2] = dpoint[2] - origfocus[2];

	dpoint = acam->GetPosition();
	acam->SetPosition(dpoint[0]+worldOffset[0],
			  dpoint[1]+worldOffset[1],
			  dpoint[2]+worldOffset[2]);
	}

      // draw the images
      this->DoFDRender();

      // restore the jitter to normal
      for (this->Renderers.InitTraversal(); 
	   aren = this->Renderers.GetNextItem(); )
	{
	acam = aren->GetActiveCamera();

	// calculate the amount to jitter
	memcpy(origfocus,acam->GetFocalPoint(),12);
	aren->SetWorldPoint(origfocus);
	aren->WorldToDisplay();
	dpoint = aren->GetDisplayPoint();
	aren->SetDisplayPoint(dpoint[0] - offsets[0],
			      dpoint[1] - offsets[1],
			      dpoint[2]);
	aren->DisplayToWorld();
	dpoint = aren->GetWorldPoint();
	dpoint[0] /= dpoint[3];
	dpoint[1] /= dpoint[3];
	dpoint[2] /= dpoint[3];
	acam->SetFocalPoint(dpoint);

	worldOffset[0] = dpoint[0] - origfocus[0];
	worldOffset[1] = dpoint[1] - origfocus[1];
	worldOffset[2] = dpoint[2] - origfocus[2];

	dpoint = acam->GetPosition();
	acam->SetPosition(dpoint[0]+worldOffset[0],
			  dpoint[1]+worldOffset[1],
			  dpoint[2]+worldOffset[2]);
	}

      // get the pixels for accumulation
      if (this->ResultFrame)
	{
	this->AABuffer[i] = this->ResultFrame;
	this->ResultFrame = NULL;
	}
      else
	{
	this->AABuffer[i] = this->GetPixelData(0,0,size[0]-1,size[1]-1);
	}
      }
    
    this->ResultFrame = this->AABuffer[0];
    
    // now accumulate the images 
    p1 = this->ResultFrame;
    for (y = 0; y < size[1]; y++)
      {
      for (x = 0; x < size[0]; x++)
	{
	r = *p1; 
	g = *(p1 + 1);  
	b = *(p1 + 2);  
	for (i = 1; i < this->AAFrames; i++)
	  {
	  r += this->AABuffer[i][(y*size[0] + x)*3];
	  g += this->AABuffer[i][(y*size[0] + x)*3 + 1];
	  b += this->AABuffer[i][(y*size[0] + x)*3 + 2];
	  }
	r /= this->AAFrames;
	g /= this->AAFrames;
	b /= this->AAFrames;
	*p1 = r; p1++;
	*p1 = g; p1++;
	*p1 = b; p1++;
	}
      }
    for (i = 1; i < this->AAFrames; i++)
      {
      delete this->AABuffer[i];
      }
    delete this->AABuffer;
    }
  else
    {
    this->DoFDRender();
    }
}


// Description:
// Ask each renderer to render an image. Synchronize this process.
void vlRenderWindow::DoFDRender()
{
  int i;

  // handle any focal depth
  if (this->FDFrames)
    {
    int *size;
    int x,y;
    int r,g,b;
    unsigned char *p1;
    vlRenderer *aren;
    vlCamera *acam;
    float focalDisk;
    float viewUp[4];
    float *vpn;
    float *dpoint;
    vlTransform aTrans;
    float offsets[2];
    float *orig;

    // get the size
    size = this->GetSize();

    // free old and get new memory
    if (this->FDBuffer) delete FDBuffer;
    this->FDBuffer = new unsigned char *[this->FDFrames];
    viewUp[3] = 1.0;

    orig = new float [3*this->Renderers.GetNumberOfItems()];

    for (i = 0; i < FDFrames; i++)
      {
      int j = 0;

      offsets[0] = drand48(); // radius
      offsets[1] = drand48()*360.0; // angle

      // store offsets for each renderer 
      for (this->Renderers.InitTraversal(); 
	   aren = this->Renderers.GetNextItem(); )
	{
	acam = aren->GetActiveCamera();
	focalDisk = acam->GetFocalDisk()*offsets[0];

	memcpy(viewUp,acam->GetViewUp(),12);
	vpn = acam->GetViewPlaneNormal();
	aTrans.Identity();
	aTrans.Scale(focalDisk,focalDisk,focalDisk);
	aTrans.RotateWXYZ(offsets[1],vpn[0],vpn[1],vpn[2]);
	aTrans.SetPoint(viewUp);
	vpn = aTrans.GetPoint();
	dpoint = acam->GetPosition();

	// store the position for later
	memcpy(orig + j*3,dpoint,12);
	j++;

	acam->SetPosition(dpoint[0]+vpn[0],
			  dpoint[1]+vpn[1],
			  dpoint[2]+vpn[2]);
	}

      // draw the images
      this->DoStereoRender();

      // restore the jitter to normal
      j = 0;
      for (this->Renderers.InitTraversal(); 
	   aren = this->Renderers.GetNextItem(); )
	{
	acam = aren->GetActiveCamera();
	acam->SetPosition(orig + j*3);
	j++;
	}

      // get the pixels for accumulation
      if (this->ResultFrame)
	{
	this->FDBuffer[i] = this->ResultFrame;
	this->ResultFrame = NULL;
	}
      else
	{
	this->FDBuffer[i] = this->GetPixelData(0,0,size[0]-1,size[1]-1);
	}
      }
    
    // free memory
    delete orig;
    
    this->ResultFrame = this->FDBuffer[0];
    
    // now accumulate the images 
    p1 = this->ResultFrame;
    for (y = 0; y < size[1]; y++)
      {
      for (x = 0; x < size[0]; x++)
	{
	r = *p1; 
	g = *(p1 + 1);  
	b = *(p1 + 2);  
	for (i = 1; i < this->FDFrames; i++)
	  {
	  r += this->FDBuffer[i][(y*size[0] + x)*3];
	  g += this->FDBuffer[i][(y*size[0] + x)*3 + 1];
	  b += this->FDBuffer[i][(y*size[0] + x)*3 + 2];
	  }
	r /= this->FDFrames;
	g /= this->FDFrames;
	b /= this->FDFrames;
	*p1 = r; p1++;
	*p1 = g; p1++;
	*p1 = b; p1++;
	}
      }
    for (i = 1; i < this->FDFrames; i++)
      {
      delete this->FDBuffer[i];
      }
    delete this->FDBuffer;
    }
  else
    {
    this->DoStereoRender();
    }
}


// Description:
// Ask each renderer to render an image.
void vlRenderWindow::DoStereoRender()
{
  this->Start();
  this->StereoUpdate();
  this->Renderers.Render();
  if (this->StereoRender)
    {
    this->StereoMidpoint();
    this->Renderers.Render();
    this->StereoRenderComplete();
    }
}

// Description:
// Add a renderer to the list of renderers.
void vlRenderWindow::AddRenderers(vlRenderer *ren)
{
  // we are its parent 
  ren->SetRenderWindow(this);
  this->Renderers.AddItem(ren);
}

// Description:
// Remove a renderer from the list of renderers.
void vlRenderWindow::RemoveRenderers(vlRenderer *ren)
{
  // we are its parent 
  this->Renderers.RemoveItem(ren);
}

// Description:
// Set the size of the window in screen coordinates.
void vlRenderWindow::SetSize(int a[2])
{
  this->SetSize(a[0],a[1]);
}

void vlRenderWindow::PrintSelf(ostream& os, vlIndent indent)
{
  int *temp;

  vlObject::PrintSelf(os,indent);

  os << indent << "Borders: " << (this->Borders ? "On\n":"Off\n");
  os << indent << "Double Buffer: " << (this->DoubleBuffer ? "On\n":"Off\n");
  os << indent << "Full Screen: " << (this->FullScreen ? "On\n":"Off\n");
  os << indent << "Name: " << this->Name << "\n";
  temp = this->GetPosition();
  os << indent << "Position: (" << temp[0] << ", " << temp[1] << ")\n";
  temp = this->GetSize();
  os << indent << "Renderers:\n";
  this->Renderers.PrintSelf(os,indent.GetNextIndent());
  os << indent << "Size: (" << temp[0] << ", " << temp[1] << ")\n";
  os << indent << "Stereo Render: " 
     << (this->StereoRender ? "On\n":"Off\n");
}


void vlRenderWindow::SaveImageAsPPM()
{
  int    *size;
  FILE   *fp;
  unsigned char *buffer;
 
  // get the size
  size = this->GetSize();
  // get the data
  buffer = this->GetPixelData(0,0,size[0]-1,size[1]-1);

  //  open the ppm file and write header 
  if ( this->FileName != NULL && *this->FileName != '\0')
    {
    fp = fopen(this->FileName,"w");
    if (!fp)
      {
      vlErrorMacro(<< "RenderWindow unable to open image file for writing\n");
      delete buffer;
      return;
      }
 
    // write out the header info 
    fprintf(fp,"P6\n%i %i\n255\n",size[0],size[1]);
 
    // now write the binary info 
    fwrite(buffer,3,size[0]*size[1],fp);
    fclose(fp);
    }

  delete buffer;
}


// Description:
// Update system if needed due to stereo rendering.
void vlRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType) 
      {
      case VL_STEREO_RED_BLUE:
	{
        this->StereoStatus = 1;
	}
      }
    }
  else if ((!this->StereoRender) && this->StereoStatus)
    {
    switch (this->StereoType) 
      {
      case VL_STEREO_RED_BLUE:
	{
        this->StereoStatus = 0;
	}
      }
    }
}

// Description:
// Handles work required between the left and right eye renders.
void vlRenderWindow::StereoMidpoint(void)
{
  switch (this->StereoType) 
    {
    case VL_STEREO_RED_BLUE:
      {
      int *size;
      // get the size
      size = this->GetSize();
      // get the data
      this->temp_buffer = this->GetPixelData(0,0,size[0]-1,size[1]-1);
      }
    }
}

// Description:
// Handles work required between the left and right eye renders.
void vlRenderWindow::StereoRenderComplete(void)
{
  switch (this->StereoType) 
    {
    case VL_STEREO_RED_BLUE:
      {
      unsigned char *buff;
      unsigned char *p1, *p2, *p3;
      unsigned char* result;
      int *size;
      int x,y;
      int res;

      // get the size
      size = this->GetSize();
      // get the data
      buff = this->GetPixelData(0,0,size[0]-1,size[1]-1);
      p1 = this->temp_buffer;
      p2 = buff;

      // allocate the result
      result = new unsigned char [size[0]*size[1]*3];
      if (!result)
	{
	vlErrorMacro(<<"Couldn't allocate memory for RED BLUE stereo.");
	return;
	}
      p3 = result;

      // now merge the two images 
      for (x = 0; x < size[0]; x++)
	{
	for (y = 0; y < size[1]; y++)
	  {
	  res = p1[0] + p1[1] + p1[2];
	  p3[0] = res/3;
	  res = p2[0] + p2[1] + p2[2];
	  p3[1] = 0;
	  p3[2] = res/3;
	  p1 += 3;
	  p2 += 3;
	  p3 += 3;
	  }
	}
      this->ResultFrame = result;
      delete [] this->temp_buffer;
      this->temp_buffer = NULL;
      delete [] buff;
      }
      break;
    }
}


// Description:
// Handles work required at end of render cycle
void vlRenderWindow::CopyResultFrame(void)
{
  if (this->ResultFrame)
    {
    int *size;

    // get the size
    size = this->GetSize();

    this->SetPixelData(0,0,size[0]-1,size[1]-1,this->ResultFrame);
    delete [] this->ResultFrame;
    this->ResultFrame = NULL;
    }
  else
    {
    this->Frame();
    }
}

// Description:
// Indicates if a StereoOn will require the window to be remapped.
int vlRenderWindow::GetRemapWindow(void)
{
  switch (this->StereoType) 
    {
    case VL_STEREO_RED_BLUE: return 0;
    case VL_STEREO_CRYSTAL_EYES: return 1;
    }
}
