//
// Create bar charts of frequency of letters. 
//
#include "vtk.h"
#include <stdio.h>
#include <ctype.h>

main (int argc, char *argv[])
{
  vtkVectorText *letters[26];
  vtkLinearExtrusionFilter *extrude[26];
  vtkPolyDataMapper *mappers[26];
  vtkActor *actors[26];
  char text[2];
  static char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i, j, freq[26], maxFreq;
  float x, y;
  FILE *fPtr;
  int c;
//
// count the letters
//
  if ( argc < 2 )
    {
    cerr << "Please provide filename: " << argv[0] << " filename\n";
    exit(1);
    }

  if ( (fPtr = fopen (argv[1], "r")) == NULL )
    {
    cerr << "Cannot open file: " <<  argv[1] << "\n";
    exit(1);
    }

  for (i=0; i<26; i++) freq[i] = 0;
  while ( (c=fgetc(fPtr)) != EOF )
    {
    if ( isalpha(c) )
      {
      c = tolower(c);
      freq[c-97]++;
      }
    }

  for (maxFreq=0, i=0; i<26; i++)
    if ( freq[i] > maxFreq ) 
      maxFreq = freq[i];
//
// graphics stuff
//
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
//
// Setup letters
//
  text[1] = '\0';
  for (i=0; i<26; i++)
    {
    text[0] = alphabet[i];
    letters[i] = vtkVectorText::New();
      letters[i]->SetText(text);
    extrude[i] = vtkLinearExtrusionFilter::New();
      extrude[i]->SetInput(letters[i]->GetOutput());
      extrude[i]->SetExtrusionType(VTK_VECTOR_EXTRUSION);
      extrude[i]->SetVector(0,0,1.0);
      extrude[i]->SetScaleFactor((float)freq[i] / maxFreq * 2.50);
    mappers[i] = vtkPolyDataMapper::New();
      mappers[i]->SetInput(extrude[i]->GetOutput());
      mappers[i]->ScalarVisibilityOff();
    actors[i] = vtkActor::New();
      actors[i]->SetMapper(mappers[i]);
      actors[i]->GetProperty()->SetColor(0.2000, 0.6300, 0.7900);
      if ( freq[i] <= 0 ) actors[i]->VisibilityOff();
    ren->AddActor(actors[i]);
    }
//
// Position actors
//
  for (y=0.0, j=0; j<2; j++, y+=(-2.0))
    for (x=0.0, i=0; i<13; i++, x+=1.0)
      actors[j*13 + i]->SetPosition(x, y, 0.0);

  ren->SetBackground(1,1,1);
  renWin->SetSize(1250,750);
  renWin->Render();

  // interact with data
  iren->Start();

  // Clean up
  ren->Delete();
  renWin->Delete();
  iren->Delete();
  for (i=0; i<26; i++)
    {
    letters[i]->Delete();
    extrude[i]->Delete();
    mappers[i]->Delete();
    actors[i]->Delete();
    }
}
