
#include "vtkMapper2D.h"


void vtkMapper2D::GetViewportClipSize(vtkViewport* viewport, int* clipWidth, int* clipHeight)
{
  // Get the viewport coordinates
  float* vpt = viewport->GetViewport(); 

  // Get the window size
  vtkWindow* window = viewport->GetVTKWindow();
  int* winSize = window->GetSize();

  // Calculate a clip width and height for the image based on the 
  // size of the viewport
  float vptWidth = vpt[XMAX] - vpt[XMIN];
  float vptHeight = vpt[YMAX] - vpt[YMIN];
  *clipWidth = (int) (vptWidth * (float) winSize[0]);
  *clipHeight = (int) (vptHeight * (float) winSize[1]);

}


void vtkMapper2D::GetActorClipSize(vtkViewport* viewport, vtkActor2D* actor, int* clipWidth, int* clipHeight)
{
  float actorPos[2] = {0};

  // Get the clipping width and height of the viewport
  this->GetViewportClipSize(viewport, clipWidth, clipHeight);

  // Now get the actor position
  float* actorPos1 = actor->GetViewPosition();

  // Change the view coordinates to normalized device coordinates
  actorPos[0] = (actorPos1[0] + 1.0) / 2.0;
  actorPos[1] = (actorPos1[1] + 1.0) / 2.0;

  vtkDebugMacro(<<"NDC actorPos: " << actorPos[0] << "," << actorPos[1]);

  // Change the clipping size based on the width of the viewport
  // and the position of the actor (actorPos).  The region needed is 
  // the size of the viewport minus the starting coordinate of the
  // actor
  //  0 <= actorPos <= 1, so multiply by viewport width

  *clipWidth = *clipWidth - actorPos[0] * (*clipWidth);
  *clipHeight = *clipHeight - actorPos[1] * (*clipHeight);
  
  // Now take into account the actor's scale

  float* actorScale = actor->GetScale();

  // The region of data we need is the width_of_viewport / scale_in_x
  // and height_of_viewport / scale_in_y since 
  // region_width * scale_in_x = width_of_viewport and
  // region_height * scale_in_y = height_of_viewport

  *clipWidth = ((float) *clipWidth) / actorScale[0];
  *clipHeight = ((float) *clipHeight) / actorScale[1];
}

