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
}

// Description:
// Ask each renderer to render an image. Synchronize this process.
void vlRenderWindow::Render()
{
  vlDebugMacro(<< "Starting Render Method.\n");

  if ( this->Interactor && ! this->Interactor->GetInitialized() )
    this->Interactor->Initialize();

  this->Start();
  this->StereoUpdate();
  this->Renderers.Render();
  if (this->StereoRender)
    {
    this->StereoMidpoint();
    this->Renderers.Render();
    this->StereoRenderComplete();
    }
  else
    {
    this->Frame();
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
	  p3[2] = res/3;
	  p3[1] = 0;
	  p1 += 3;
	  p2 += 3;
	  p3 += 3;
	  }
	}
      this->SetPixelData(0,0,size[0]-1,size[1]-1,result);
      delete result;
      delete this->temp_buffer;
      this->temp_buffer = NULL;
      delete buff;
      }
      break;
    default:
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
