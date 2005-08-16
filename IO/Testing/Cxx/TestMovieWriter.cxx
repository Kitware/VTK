#include "vtkGenericMovieWriter.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkImageCast.h"
#include "vtkImageMapToColors.h"
#include "vtkLookupTable.h"
#include "vtkImageData.h"

#ifdef _WIN32
#  include "vtkAVIWriter.h"
#  define WRITER vtkAVIWriter
#  define EXT "avi"
#else
#  include "vtkMPEG2Writer.h"
#  define WRITER vtkMPEG2Writer
#  define EXT "mpg"
#endif

int main(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int cc;
  vtkImageMandelbrotSource* Fractal0 = vtkImageMandelbrotSource::New();
  Fractal0->SetWholeExtent( 0, 250, 0, 250, 0, 0 );
  Fractal0->SetProjectionAxes( 0, 1, 2 );
  Fractal0->SetOriginCX( -1.75, -1.25, 0, 0 );
  Fractal0->SetSizeCX( 2.5, 2.5, 2, 1.5 );
  Fractal0->SetMaximumNumberOfIterations( 100);

  vtkImageCast* cast = vtkImageCast::New();
  cast->SetInput(Fractal0->GetOutput());
  cast->SetOutputScalarTypeToUnsignedChar();

  vtkLookupTable* table = vtkLookupTable::New();
  table->SetTableRange(0, 100);
  table->SetNumberOfColors(100);
  table->Build();
  table->SetTableValue(99, 0, 0, 0);

  vtkImageMapToColors* colorize = vtkImageMapToColors::New();
  colorize->SetOutputFormatToRGB();
  colorize->SetLookupTable(table);
  colorize->SetInput(cast->GetOutput());

  vtkGenericMovieWriter *w = WRITER::New();
  w->SetInput(colorize->GetOutput());
  w->SetFileName("movie." EXT);
  w->Start();
  int cnt = 0;
  for ( cc = 2; cc < 99; cc ++ )
    {
    cout << "Processing image: " << cnt++ << endl;
    Fractal0->SetMaximumNumberOfIterations(cc);
    table->SetTableRange(0, cc);
    table->SetNumberOfColors(cc);
    table->ForceBuild();
    table->SetTableValue(cc-1, 0, 0, 0);
    w->Write();
    }
  w->End();  
  w->Delete();
  
  cnt = 0;

  w = WRITER::New();
  w->SetInput(colorize->GetOutput());
  w->SetFileName("movie1." EXT);
  w->Start();
  for ( cc = 2; cc < 99; cc ++ )
    {
    cout << "Processing image: " << cnt++ << endl;
    Fractal0->SetMaximumNumberOfIterations(cc);
    table->SetTableRange(0, cc);
    table->SetNumberOfColors(cc);
    table->ForceBuild();
    table->SetTableValue(cc-1, 0, 0, 0);
    w->Write();
    }
  w->End();
  w->Delete();

  cast->Delete();
  Fractal0->Delete();
  return 0;
}
