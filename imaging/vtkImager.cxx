#include "vtkImager.h"
#include "vtkImageWindow.h"

// Description:
// Create an imager with viewport (0, 0, 1, 1)
vtkImager::vtkImager()
{
  vtkDebugMacro(<< "vtkImager::vtkImager");

  this->Viewport[XMIN] = 0.0; // min x
  this->Viewport[XMAX] = 1.0; // max x
  this->Viewport[YMIN] = 0.0; // min y
  this->Viewport[YMAX] = 1.0; // max y

}

vtkImager::~vtkImager()
{
  vtkDebugMacro(<< "vtkImager::~vtkImager");
}

void vtkImager::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkViewport::PrintSelf(os, indent);
}

void vtkImager::Render()
{
  vtkActor2D* tempActor;

  vtkDebugMacro (<< "vtkImager::Render");

  if (this->StartRenderMethod) 
    {
    (*this->StartRenderMethod)(this->StartRenderMethodArg);
    }

  int numActors = this->Actors2D.GetNumberOfItems();
  if (numActors == 0)
    {
      vtkDebugMacro (<< "vtkImager::Render - No actors in collection");
      return;
    }
    
  vtkDebugMacro(<<"vtkImager::Render - " << numActors << " actors in collection");
  vtkDebugMacro(<<"vtkImager::Render - Sorting actor collection.");
  
  this->Actors2D.Sort();  

  for ( this->Actors2D.InitTraversal(); 
         (tempActor = this->Actors2D.GetNextItem());)
    {
	  // Make sure that the actor is visible before rendering
	  if (tempActor->GetVisibility() == 1) tempActor->Render(this);
    }

  if (this->EndRenderMethod) 
    {
    (*this->EndRenderMethod)(this->EndRenderMethodArg);
    }

  return;

}



