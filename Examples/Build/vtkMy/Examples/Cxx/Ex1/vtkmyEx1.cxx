#include "vtkBar.h"
#include "vtkBar2.h"
#include "vtkImageFoo.h"

int main( int argc, char *argv[] )
{
 vtkBar *bar = vtkBar::New();
 bar->Print(cout);
 bar->Delete();

 vtkBar2 *bar2 = vtkBar2::New();
 bar2->Print(cout);
 bar2->Delete();

 vtkImageFoo *imagefoo = vtkImageFoo::New();
 imagefoo->Print(cout);
 imagefoo->Delete();

 return 0;
}


