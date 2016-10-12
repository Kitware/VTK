/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TreeLayout.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example shows how to create a simple tree view from an XML file.
// You may specify the label array and color array from the command line.
//

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkDynamic2DLabelMapper.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkInteractorStyleImage.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringToNumeric.h"
#include "vtkTextProperty.h"
#include "vtkTreeLayoutStrategy.h"
#include "vtkXMLTreeReader.h"

void usage()
{
  cerr << endl;
  cerr << "usage: TreeLayout filename [label_attribute] [color_attribute]" << endl;
  cerr << "  filename is an xml file" << endl;
  cerr << "  label_attribute is the attribute to use as labels." << endl;
  cerr << "    Default is .tagname which labels using the element tag." << endl;
  cerr << "  color_attribute is the attribute to color by (numeric)." << endl;
  cerr << "    Default is no color." << endl;
}

int main(int argc, char* argv[])
{
  // Initialize parameters from the command line.
  const char* labelArray = ".tagname";
  const char* colorArray = NULL;
  if (argc < 2)
  {
    usage();
    return 0;
  }
  char* filename = argv[1];
  if (argc >= 3)
  {
    labelArray = argv[2];
  }
  if (argc >= 4)
  {
    colorArray = argv[3];
  }

  // Read in the XML file into a tree.
  // This creates a tree with string columns for every attribute
  // present in the file, plus the special arrays named .tagname
  // (containing the XML tag name) and .chardata (containg the
  // character data within the tag).
  vtkXMLTreeReader* reader = vtkXMLTreeReader::New();
  reader->SetFileName(filename);

  // Automatically convert string columns containing numeric
  // values into integer and double arrays.
  vtkStringToNumeric* stringToNumeric = vtkStringToNumeric::New();
  stringToNumeric->SetInputConnection(reader->GetOutputPort());

  // Retrieve the tree from the pipeline so we can check whether
  // the specified label and color arrays exist.
  stringToNumeric->Update();
  vtkTree* tree = vtkTree::SafeDownCast(stringToNumeric->GetOutput());
  if (tree->GetVertexData()->GetAbstractArray(labelArray) == NULL)
  {
    cerr << "ERROR: The label attribute " << labelArray << " is not defined in the file." << endl;
    reader->Delete();
    stringToNumeric->Delete();
    usage();
    return 0;
  }
  if (colorArray &&
      tree->GetVertexData()->GetAbstractArray(colorArray) == NULL)
  {
    cerr << "ERROR: The color attribute " << colorArray << " is not defined in the file." << endl;
    reader->Delete();
    stringToNumeric->Delete();
    usage();
    return 0;
  }
  if (colorArray &&
      vtkArrayDownCast<vtkDataArray>(tree->GetVertexData()->GetAbstractArray(colorArray)) == NULL)
  {
    cerr << "ERROR: The color attribute " << colorArray << " does not have numeric values." << endl;
    reader->Delete();
    stringToNumeric->Delete();
    usage();
    return 0;
  }

  // If coloring the vertices, get the range of the color array.
  double colorRange[2] = {0, 1};
  if (colorArray)
  {
    vtkDataArray* color = vtkArrayDownCast<vtkDataArray>(
      tree->GetVertexData()->GetAbstractArray(colorArray));
    color->GetRange(colorRange);
  }

  // Layout the tree using vtkGraphLayout.
  vtkGraphLayout* layout = vtkGraphLayout::New();
  layout->SetInputConnection(stringToNumeric->GetOutputPort());

  // Specify that we want to use the tree layout strategy.
  vtkTreeLayoutStrategy* strategy = vtkTreeLayoutStrategy::New();
  strategy->RadialOn();              // Radial layout (as opposed to standard top-down layout)
  strategy->SetAngle(360.0);         // The tree fills a full circular arc.
  layout->SetLayoutStrategy(strategy);

  // vtkGraphToPolyData converts a graph or tree to polydata.
  vtkGraphToPolyData* graphToPoly = vtkGraphToPolyData::New();
  graphToPoly->SetInputConnection(layout->GetOutputPort());

  // Create the standard VTK polydata mapper and actor
  // for the connections (edges) in the tree.
  vtkPolyDataMapper* edgeMapper = vtkPolyDataMapper::New();
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  vtkActor* edgeActor = vtkActor::New();
  edgeActor->SetMapper(edgeMapper);
  edgeActor->GetProperty()->SetColor(0.0, 0.5, 1.0);

  // Glyph the points of the tree polydata to create
  // VTK_VERTEX cells at each vertex in the tree.
  vtkGlyph3D* vertGlyph = vtkGlyph3D::New();
  vertGlyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  vtkGlyphSource2D* glyphSource = vtkGlyphSource2D::New();
  glyphSource->SetGlyphTypeToVertex();
  vertGlyph->SetInputConnection(1, glyphSource->GetOutputPort());

  // Create a mapper for the vertices, and tell the mapper
  // to use the specified color array.
  vtkPolyDataMapper* vertMapper = vtkPolyDataMapper::New();
  vertMapper->SetInputConnection(vertGlyph->GetOutputPort());
  if (colorArray)
  {
    vertMapper->SetScalarModeToUsePointFieldData();
    vertMapper->SelectColorArray(colorArray);
    vertMapper->SetScalarRange(colorRange);
  }

  // Create an actor for the vertices.  Move the actor forward
  // in the z direction so it is drawn on top of the edge actor.
  vtkActor* vertActor = vtkActor::New();
  vertActor->SetMapper(vertMapper);
  vertActor->GetProperty()->SetPointSize(5);
  vertActor->SetPosition(0, 0, 0.001);

  // Use a dynamic label mapper to draw the labels.  This mapper
  // does not allow labels to overlap, as long as the camera is
  // not rotated from pointing down the z axis.
  vtkDynamic2DLabelMapper* labelMapper = vtkDynamic2DLabelMapper::New();
  labelMapper->SetInputConnection(graphToPoly->GetOutputPort());
  labelMapper->GetLabelTextProperty()->SetJustificationToLeft();
  labelMapper->GetLabelTextProperty()->SetColor(0, 0, 0);
  if (labelArray)
  {
    labelMapper->SetLabelModeToLabelFieldData();
    labelMapper->SetFieldDataName(labelArray);
  }
  vtkActor2D* labelActor = vtkActor2D::New();
  labelActor->SetMapper(labelMapper);

  // Add the edges, vertices, and labels to the renderer.
  vtkRenderer* ren = vtkRenderer::New();
  ren->SetBackground(0.8, 0.8, 0.8);
  ren->AddActor(edgeActor);
  ren->AddActor(vertActor);
  ren->AddActor(labelActor);

  // Setup the render window and interactor.
  vtkRenderWindow* win = vtkRenderWindow::New();
  win->AddRenderer(ren);
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(win);

  // Constrain movement to zoom and pan using the image interactor style.
  vtkInteractorStyleImage* style = vtkInteractorStyleImage::New();
  iren->SetInteractorStyle(style);

  // Start the main application loop.
  iren->Initialize();
  iren->Start();

  // Clean up.
  style->Delete();
  iren->Delete();
  win->Delete();
  ren->Delete();
  labelActor->Delete();
  labelMapper->Delete();
  vertActor->Delete();
  vertMapper->Delete();
  glyphSource->Delete();
  vertGlyph->Delete();
  edgeMapper->Delete();
  edgeActor->Delete();
  graphToPoly->Delete();
  strategy->Delete();
  layout->Delete();
  stringToNumeric->Delete();
  reader->Delete();

  return 0;
}
