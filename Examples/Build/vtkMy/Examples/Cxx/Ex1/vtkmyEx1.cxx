#include "vtkBar.h"
#include "vtkBar2.h"
#include "vtkImageFoo.h"

int main( int argc, char *argv[] )
{

  cout << "Create vtkBar object and print it." << endl;
  
  vtkBar *bar = vtkBar::New();
  bar->Print(cout);
  bar->Delete();
  
  cout << "Create vtkBar2 object and print it." << endl;
  
  vtkBar2 *bar2 = vtkBar2::New();
  bar2->Print(cout);
  bar2->Delete();
  
  cout << "Create vtkImageFoo object and print it." << endl;
  
  vtkImageFoo *imagefoo = vtkImageFoo::New();
  imagefoo->Print(cout);
  imagefoo->Delete();
  
  cout << "Looks good ?" << endl;

  return 0;
}


