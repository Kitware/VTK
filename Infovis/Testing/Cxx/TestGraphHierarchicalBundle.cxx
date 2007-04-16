#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCommand.h"
#include "vtkGraph.h"
#include "vtkGraphHierarchicalBundle.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkInteractorStyleImage.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRandomGraphSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSplineFilter.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkTree.h"
#include "vtkTreeLayoutStrategy.h"
#include "vtkVariant.h"
//#include "vtkXMLTreeReader.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


#define RANDOM_TREE       0
#define STRUCTURED_TREE   1
#define VTK_SOURCES_TREE  2

int TestGraphHierarchicalBundle(int argc, char* argv[])
{
  int treeType = STRUCTURED_TREE;
  const char* file;
  bool showTree = false;
  vtkIdType numVertices = 200;
  vtkIdType numEdges = 100;
  double bundlingStrength = 0.9;
  bool radial = true;
  double angle = 360;
  double logSpacing = 0.8;

  for (int i = 1; i < argc; i++)
    {
    if (!strcmp(argv[i], "-I")) 
      {
      continue;
      }
    if (!strcmp(argv[i], "-D")) 
      {
      i++;
      continue;
      }
    if (!strcmp(argv[i], "-T")) 
      {
      i++;
      continue;
      }
    if (!strcmp(argv[i], "-V")) 
      {
      i++;
      continue;
      }
    if (!strcmp(argv[i], "-t")) 
      {
      showTree = true;
      continue;
      }
    if (!strcmp(argv[i], "-S")) 
      {
      radial = false;
      continue;
      }
    if (!strcmp(argv[i], "-A")) 
      {
      i++;
      angle = atof(argv[i]);
      continue;
      }
    if (!strcmp(argv[i], "-L"))
      {
      i++;
      logSpacing = atof(argv[i]);
      continue;
      }
    if (!strcmp(argv[i], "-r"))
      {
      treeType = RANDOM_TREE;
      i++;
      numVertices = atoi(argv[i]);
      i++;
      numEdges = atoi(argv[i]);
      continue;
      }
    if (!strcmp(argv[i], "-s"))
      {
      treeType = STRUCTURED_TREE;
      i++;
      numVertices = atoi(argv[i]);
      i++;
      numEdges = atoi(argv[i]);
      continue;
      }
    if (!strcmp(argv[i], "-v"))
      {
      treeType = VTK_SOURCES_TREE;
      i++;
      file = argv[i];
      continue;
      }
    if (!strcmp(argv[i], "-b"))
      {
      i++;
      bundlingStrength = atof(argv[i]);
      continue;
      }

    cerr << argv[0] << " Options:\n" 
      << " -I : interactive\n"
      << " -r #vertices #edges: show random tree with random edges\n"
      << " -s #vertices #edges: show structured tree with structured edges\n"
      //<< " -v filename : show VTK data in XML file\n"
      << " -b strength : bundling strength (0.0 to 1.0; default 0.8)\n"
      << " -S : standard tree layout (default radial)\n"
      << " -A angle : tree sweep angle (default 360)\n"
      << " -L logspacing : tree logspacing (0.0 to 1.0; default 0.8)\n"
      << " -t : show tree instead of edge bundles\n";
    return 0;
    }
  vtkIdType levelOneVertices = static_cast<vtkIdType>(sqrt(static_cast<float>(numVertices)));

  // Create the graph.
  
  vtkAbstractGraph* graph;
  if (treeType == RANDOM_TREE)
    {
    VTK_CREATE(vtkRandomGraphSource, source);
    source->SetNumberOfVertices(numVertices);
    source->SetNumberOfEdges(numEdges);
    source->SetStartWithTree(false);
    source->Update();
    graph = source->GetOutput();

    VTK_CREATE(vtkStringArray, nameArray);
    nameArray->SetName("name");
    for (vtkIdType i = 0; i < graph->GetNumberOfVertices(); i++)
      {
      nameArray->InsertNextValue(vtkVariant(i).ToString());
      }
    graph->GetVertexData()->AddArray(nameArray);
    graph->Register(0);
    }
  else if (treeType == STRUCTURED_TREE)
    {
    vtkGraph* g = vtkGraph::New();
    for (vtkIdType v = 0; v < numVertices; v++)
      {
      g->AddVertex();
      }
    for (vtkIdType e = 0; e < numEdges; e++)
      {
      g->AddEdge(e%numVertices, (e*e)%numVertices);
      }
    graph = g;
    }
#if 0
  else
    {
    VTK_CREATE(vtkXMLTreeReader, reader);
    reader->SetFileName(file);
    reader->Update();
    graph = reader->GetOutput();
    graph->Register(0);
    }
#endif

  //for (vtkIdType a = 0; a < graph->GetNumberOfEdges(); a++)
  //  {
  //  cerr << "edge " << a << ": " << graph->GetSourceVertex(a) << "," << graph->GetTargetVertex(a) << endl;
  //  }

  // Create the tree.
  VTK_CREATE(vtkTree, tree);
  if (treeType == RANDOM_TREE)
    {
    tree->AddRoot();
    for (vtkIdType i = 1; i < numVertices; i++)
      {
      vtkIdType parent = static_cast<vtkIdType>(vtkMath::Random(0, tree->GetNumberOfVertices()));
      tree->AddChild(parent);
      }
    tree->GetVertexData()->AddArray(graph->GetVertexData()->GetAbstractArray("name"));
    }
  else if (treeType == STRUCTURED_TREE)
    {
    vtkIdType i;
    tree->AddRoot();
    for (i = 0; i < levelOneVertices; i++)
      {
      tree->AddChild(0);
      }
    vtkIdType levelTwoVertices = numVertices - levelOneVertices - 1;
    for (i = 0; i < levelTwoVertices; i++)
      {
      tree->AddChild(static_cast<vtkIdType>(i / (levelTwoVertices / static_cast<double>(levelOneVertices)) + 1.5));
      }
    tree->GetVertexData()->AddArray(graph->GetVertexData()->GetAbstractArray("name"));
    }
  else
    {
    VTK_CREATE(vtkStringArray, kitNames);
    kitNames->InsertNextValue("Common");
    kitNames->InsertNextValue("Filtering");
    kitNames->InsertNextValue("GenericFiltering");
    kitNames->InsertNextValue("Graphics");
    kitNames->InsertNextValue("Hybrid");
    kitNames->InsertNextValue("Imaging");
    kitNames->InsertNextValue("Infovis");
    kitNames->InsertNextValue("IO");
    kitNames->InsertNextValue("Parallel");
    kitNames->InsertNextValue("Rendering");
    kitNames->InsertNextValue("VolumeRendering");
    kitNames->InsertNextValue("Widgets");

    tree->AddRoot();
    vtkIdType i;
    for (i = 1; i < graph->GetNumberOfVertices(); i++)
      {
      tree->AddChild(i-1);
      }
    VTK_CREATE(vtkStringArray, extendedNameArray);
    extendedNameArray->DeepCopy(graph->GetVertexData()->GetAbstractArray("name"));
    extendedNameArray->SetName("name");
    tree->AddChild(i-1);
    extendedNameArray->InsertNextValue("VTK");
    // Reverse the entire path
    tree->SetRoot(i);
    vtkIdType root = tree->GetRoot();
    for (vtkIdType k = 0; k < kitNames->GetNumberOfValues(); k++)
      {
      tree->AddChild(root);
      extendedNameArray->InsertNextValue(kitNames->GetValue(k));
      }
    vtkStringArray* fileArray = dynamic_cast<vtkStringArray*>(graph->GetVertexData()->GetAbstractArray("filename"));
    for (i = 0; i < graph->GetNumberOfVertices(); i++)
      {
      vtkStdString curFile = fileArray->GetValue(i);
      bool found = false;
      vtkIdType k;
      for (k = 0; k < kitNames->GetNumberOfValues(); k++)
        {
        vtkStdString kit = kitNames->GetValue(k);
        if (!curFile.compare(0, kit.length(), kit))
          {
          tree->SetParent(i, root + 1 + k);
          found = true;
          break;
          }
        }
      if (!found)
        {
        cerr << "cannot find match for filename " << file << endl;
        }
      }

    tree->GetVertexData()->AddArray(extendedNameArray);
    }

  VTK_CREATE(vtkTreeLayoutStrategy, treeStrategy);
  treeStrategy->SetAngle(angle);
  treeStrategy->SetRadial(radial);
  treeStrategy->SetLogSpacingValue(logSpacing);

  VTK_CREATE(vtkGraphLayout, treeLayout);
  treeLayout->SetInput(tree);
  treeLayout->SetLayoutStrategy(treeStrategy);

  VTK_CREATE(vtkGraphHierarchicalBundle, bundle);
  bundle->SetInput(0, graph);
  bundle->SetInputConnection(1, treeLayout->GetOutputPort(0));
  bundle->SetBundlingStrength(bundlingStrength);
  bundle->SetDirectMapping(true);

  VTK_CREATE(vtkSplineFilter, spline);
  spline->SetInputConnection(0, bundle->GetOutputPort(0));

  VTK_CREATE(vtkLookupTable, lut);
  int numValues = 100;
  lut->SetNumberOfTableValues(numValues);
  lut->Build();
  for (int i = 0; i < numValues; i++)
    {
    double frac = static_cast<double>(i)/numValues;
    lut->SetTableValue(i, 1.0 - frac, frac, 0.0);
    }

  VTK_CREATE(vtkPolyDataMapper, polyMapper);
  polyMapper->SetInputConnection(0, spline->GetOutputPort(0));
  polyMapper->SetScalarModeToUsePointFieldData();
  polyMapper->SetLookupTable(lut);
  polyMapper->SelectColorArray("fraction");

  VTK_CREATE(vtkActor, polyActor);
  polyActor->SetMapper(polyMapper);
  polyActor->GetProperty()->SetOpacity(0.5);

  VTK_CREATE(vtkGraphToPolyData, treePoly);
  //treePoly->SetInput(tree);
  treePoly->SetInputConnection(0, treeLayout->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, treeMapper);
  treeMapper->SetInputConnection(0, treePoly->GetOutputPort(0));

  VTK_CREATE(vtkActor, treeActor);
  treeActor->SetMapper(treeMapper);
  treeActor->GetProperty()->SetColor(0.4, 0.6, 1.0);

#if 0
  VTK_CREATE(vtkDynamic2DLabelMapper, labelMapper);
  labelMapper->SetInputConnection(0, treePoly->GetOutputPort(0));
  labelMapper->SetLabelModeToLabelFieldData();
  labelMapper->SetFieldDataName("name");
  labelMapper->SetLabelFormat("%s");
  labelMapper->GetLabelTextProperty()->SetColor(0.0, 0.0, 0.0);

  VTK_CREATE(vtkActor2D, labelActor);
  labelActor->SetMapper(labelMapper);
#endif

  VTK_CREATE(vtkRenderer, ren);
  ren->SetBackground(1.0, 1.0, 1.0);

  if (showTree)
    {
    ren->AddActor(treeActor);
    }
  else
    {
    ren->AddActor(polyActor);
    }
  //ren->AddActor2D(labelActor);
  
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkInteractorStyleImage, style);
  VTK_CREATE(vtkRenderWindow, win);
  iren->SetInteractorStyle(style);
  win->AddRenderer(ren);
  win->SetInteractor(iren);
  //win->LineSmoothingOn();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    win->Render();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }

  // Clean up
  graph->Delete();

  return !retVal;
}
