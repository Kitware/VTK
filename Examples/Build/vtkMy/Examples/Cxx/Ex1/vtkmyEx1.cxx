#include "vtkBar.h"
#include "vtkBar2.h"
#include "vtkImageFoo.h"

int main( int argc, char *argv[] )
{
 vtkBar *bar = vtkBar::New();
 bar->PrintSelf();
 bar->Delete();

 vtkBar2 *bar2 = vtkBar2::New();
 bar2->PrintSelf();
 bar2->Delete();

 vtkImageFoo *imagefoo = vtkImageFoo::New();
 imagefoo->PrintSelf();
 imagefoo->Delete();

 return 0;
}




