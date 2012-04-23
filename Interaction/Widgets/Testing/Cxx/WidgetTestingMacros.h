#ifndef __WidgetTestingMacros_h
#define __WidgetTestingMacros_h

#include "vtkDebugLeaks.h"
#include "vtkSmartPointer.h"
#include "vtkMath.h"

#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"

#include <vtkAssemblyPath.h>
#include <vtkPropCollection.h>
#include <vtkInformation.h>
#include <vtkMatrix4x4.h>
#include <vtkProp.h>
#include <vtkActor.h>
#include <vtkPointPlacer.h>

#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>

#include <vtkAbstractTransform.h>
#include <vtkFollower.h>

// to instantiate some variables for testing
#include <vtkPointHandleRepresentation2D.h>
#include <vtkLineWidget2.h>



/// tests basic vtkObject methods
#define EXERCISE_BASIC_OBJECT_METHODS( object ) \
    { \
    if ( object == NULL ) \
      { \
      std::cerr << "EXERCISE_BASIC_OBJECT_METHODS( with NULL object )" << std::endl;  \
      return EXIT_FAILURE;  \
      } \
    object->Print( std::cout );  \
    std::cout << "Name of Class = " << object->GetClassName() << std::endl; \
    std::cout << "Name of Superclass = " << object->Superclass::GetClassName() << std::endl; \
    }

/// test object by calling Set on the variable with false, true, 0, 1, On, Off
#define TEST_SET_GET_BOOLEAN( object, variable ) \
  object->Set##variable( false ); \
  object->Set##variable( true ); \
  if( object->Get##variable() != 1 ) \
    {   \
    std::cerr << "Error in Set/Get"#variable << ", Get"#variable << " is " << object->Get##variable() << " instead of 1" << std::endl; \
    return EXIT_FAILURE; \
    } \
  object->Set##variable( false ); \
  if( object->Get##variable() != 0 ) \
    {   \
    std::cerr << "Error in Set/Get"#variable << ", Get"#variable << " is " << object->Get##variable() << " instead of 0" << std::endl; \
    return EXIT_FAILURE; \
    } \
  object->variable##On(); \
  if( object->Get##variable() != 1 ) \
    {   \
    std::cerr << "Error in On/Get"#variable << ", Get"#variable << " is " << object->Get##variable() << " instead of 1" << std::endl; \
    return EXIT_FAILURE; \
    } \
  object->variable##Off(); \
  if( object->Get##variable() != 0 ) \
    {   \
    std::cerr << "Error in Off/Get"#variable << ", Get"#variable << " is " << object->Get##variable() << " instead of 0" << std::endl; \
    return EXIT_FAILURE; \
    }

/// test an integer variable on the object by setting it to input value using Set, and
/// testing it via the Get
#define TEST_SET_GET_INT( object, variable, value )        \
  {                                                        \
    object->Set##variable( value );                        \
    if( object->Get##variable() != value )                 \
      {                                                    \
      std::cerr << "Error in Set/Get"#variable << " using value " << value << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
  }

/// Test an integer variable on object over the range, calls test set get in
/// with min - epsilon, min, min + epsilon, (min+max)/2, max - epsilon, max,
/// max + epsilon, where first and last test should report errors
/// epsilon defined as 1
#define TEST_SET_GET_INT_RANGE( object, variable, min, max ) \
  {                                                         \
    int epsilon = 1;                                        \
    int val = min - epsilon;                                \
    TEST_SET_GET_INT( object, variable, val);               \
    val = min;                                              \
    TEST_SET_GET_INT( object, variable, val);               \
    val = min + epsilon;                                    \
    TEST_SET_GET_INT( object, variable, val);               \
    val = (min + max) / 2;                                  \
    TEST_SET_GET_INT( object, variable, val);               \
    val = max - epsilon;                                    \
    TEST_SET_GET_INT( object, variable, val);               \
    val = max;                                              \
    TEST_SET_GET_INT( object, variable, val);               \
    val = max + epsilon;                                    \
    TEST_SET_GET_INT( object, variable, val);               \
  }

/// test a double variable on the object by setting it to input value using Set, and
/// testing it via the Get
#define TEST_SET_GET_DOUBLE( object, variable, value )    \
  {                                             \
    object->Set##variable( value );               \
    if( object->Get##variable() != value )        \
      {                                         \
      std::cerr << "Error in Set/Get"#variable << " using value '" << value << "', got '" << object->Get##variable() << "'" << std::endl; \
      return EXIT_FAILURE;                                      \
      }                                                         \
  }

/// Test a double variable on object over the range, calls test set get in
/// with min - epsilon, min, min + epsilon, (min+max)/2, max - epsilon, max,
/// max + epsilon, where first and last test should report errors
/// epsilon set to 1.0
#define TEST_SET_GET_DOUBLE_RANGE( object, variable, min, max )         \
  {                                                                     \
    double epsilon = 1.0;                                               \
    double val = min - epsilon;                                         \
    TEST_SET_GET_DOUBLE( object, variable, val);                        \
    val = min;                                                          \
    TEST_SET_GET_DOUBLE( object, variable, val);                        \
    val = min + epsilon;                                                \
    TEST_SET_GET_DOUBLE( object, variable, val);                        \
    val = (min + max) / 2.0;                                            \
    TEST_SET_GET_DOUBLE( object, variable, val);                        \
    val = max - epsilon;                                                \
    TEST_SET_GET_DOUBLE( object, variable, val);                        \
    val = max;                                                          \
    TEST_SET_GET_DOUBLE( object, variable, val);                        \
    val = max + epsilon;                                                \
    TEST_SET_GET_DOUBLE( object, variable, val);                        \
  }

/// test a vector variable on the object by setting it to a the values x, y, z
/// passed in using Set, and  testing it via the Get
#define TEST_SET_GET_VECTOR3_DOUBLE( object, variable, x, y, z )    \
  {                                                                 \
    object->Set##variable( x, y, z );                               \
    double *val = object->Get##variable();                          \
    if( val == NULL || val[0] != x || val[1] != y || val[2] != z )  \
      {                                                             \
      std::cerr << "Error in Set/Get"#variable << std::endl;        \
      return EXIT_FAILURE;                                          \
      }                                                             \
  }


/// test a vector variable on the object by setting it to a the values x, y
/// passed in using Set, and  testing it via the Get
#define TEST_SET_GET_VECTOR2( object, variable, x, y )    \
  {                                                                 \
    object->Set##variable( x, y );                               \
    int *val = object->Get##variable();                          \
    if( val == NULL || val[0] != x || val[1] != y )  \
      {                                                             \
      std::cerr << "Error in Set/Get"#variable << std::endl;        \
      return EXIT_FAILURE;                                          \
      }                                                             \
  }

/// test an integer  vector2 variable on the object over the range, calls test set get in
/// with min - epsilon, min, min + epsilon, (min+max)/2, max - epsilon, max,
/// max + epsilon, where first and last test should report errors. For now all
/// three elements are set to the same thing each time.
/// epsilon set to 1
#define TEST_SET_GET_VECTOR2_INT_RANGE( object, variable, min, max )    \
  {                                                                     \
    int  epsilon = 1;                                                   \
    TEST_SET_GET_VECTOR2(object, variable, min - epsilon, min - epsilon); \
    TEST_SET_GET_VECTOR2(object, variable, min, min);                   \
    TEST_SET_GET_VECTOR2(object, variable, min + epsilon, min + epsilon); \
    int half = (min+max/2);                                             \
    TEST_SET_GET_VECTOR2(object, variable, half, half);                 \
    TEST_SET_GET_VECTOR2(object, variable, max - epsilon, max - epsilon); \
    TEST_SET_GET_VECTOR2(object, variable, max, max);                   \
    TEST_SET_GET_VECTOR2(object, variable, max + epsilon, max + epsilon); \
  }

/// test an double  vector2 variable on the object over the range, calls test set get in
/// with min - epsilon, min, min + epsilon, (min+max)/2, max - epsilon, max,
/// max + epsilon, where first and last test should report errors. For now all
/// three elements are set to the same thing each time.
/// epsilon set to 1.0
#define TEST_SET_GET_VECTOR2_DOUBLE_RANGE( object, variable, min, max )    \
  {                                                                     \
    double  epsilon = 1.0;                                                   \
    TEST_SET_GET_VECTOR2(object, variable, min - epsilon, min - epsilon); \
    TEST_SET_GET_VECTOR2(object, variable, min, min);          \
    TEST_SET_GET_VECTOR2(object, variable, min + epsilon, min + epsilon); \
    double half = (min+max/2.0);                                             \
    TEST_SET_GET_VECTOR2(object, variable, half, half);       \
    TEST_SET_GET_VECTOR2(object, variable, max - epsilon, max - epsilon); \
    TEST_SET_GET_VECTOR2(object, variable, max, max);          \
    TEST_SET_GET_VECTOR2(object, variable, max + epsilon, max + epsilon); \
  }

/// Test a double vector variable on object over the range, calls test set get in
/// with min - epsilon, min, min + epsilon, (min+max)/2, max - epsilon, max,
/// max + epsilon, where first and last test should report errors. For now all
/// three elements are set to the same thing each time.
/// epsilon set to 1.0
#define TEST_SET_GET_VECTOR3_DOUBLE_RANGE( object, variable, min, max )  \
  {                                                                     \
    double epsilon = 1.0;                                               \
    TEST_SET_GET_VECTOR3_DOUBLE(object, variable, min - epsilon, min - epsilon, min - epsilon); \
    TEST_SET_GET_VECTOR3_DOUBLE(object, variable, min, min, min);       \
    TEST_SET_GET_VECTOR3_DOUBLE(object, variable, min + epsilon, min + epsilon, min + epsilon); \
    double half = (min+max/2.0);                                        \
    TEST_SET_GET_VECTOR3_DOUBLE(object, variable, half, half, half);    \
    TEST_SET_GET_VECTOR3_DOUBLE(object, variable, max - epsilon, max - epsilon, max - epsilon); \
    TEST_SET_GET_VECTOR3_DOUBLE(object, variable, max, max, max);       \
    TEST_SET_GET_VECTOR3_DOUBLE(object, variable, max + epsilon, max + epsilon, max + epsilon); \
  }

/// test a string variable on the object by calling Set/Get
#define TEST_SET_GET_STRING( object, variable )                                       \
  {                                                                     \
    const char * originalStringPointer = object->Get##variable();       \
    std::string originalString;                                         \
    if( originalStringPointer != NULL )                                 \
      {                                                                 \
      originalString = originalStringPointer;                           \
      }                                                                 \
    object->Set##variable( "testing with a const char");                \
    if( strcmp(object->Get##variable(), "testing with a const char") != 0) \
      {                                                                 \
      std::cerr << "Error in Set/Get"#variable << " with a string literal" << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    std::string string1 = "testingIsGood";                              \
    object->Set##variable( string1.c_str() );                           \
    if( object->Get##variable() != string1 )                            \
      {                                                                 \
      std::cerr << "Error in Set/Get"#variable << std::endl;            \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    std::string string2 = "moreTestingIsBetter";                        \
    object->Set##variable( string2.c_str() );                           \
    if( object->Get##variable() != string2 )                            \
      {                                                                 \
      std::cerr << "Error in Set/Get"#variable << std::endl;            \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    if( originalStringPointer != NULL )                                 \
      {                                                                 \
      object->Set##variable( originalString.c_str() );                  \
      }                                                                 \
    else                                                                \
      {                                                                 \
      object->Set##variable( NULL );                                    \
      }                                                                 \
  }

/// test a char variable on the object by calling Set/Get
#define TEST_SET_GET_CHAR( object, variable )                           \
  {                                                                     \
    const char originalChar = object->Get##variable();                  \
    object->Set##variable( 't');                                        \
    if( object->Get##variable() != 't')                                 \
      {                                                                 \
      std::cerr << "Error in Set/Get"#variable << " with a literal 't'" << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    object->Set##variable( '3');                                        \
    if( object->Get##variable() != '3')                                 \
      {                                                                 \
      std::cerr << "Error in Set/Get"#variable << " with a literal '3'" << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    object->Set##variable( originalChar );                              \
  }

/// test vtkInteractorObserver methods
#define EXERCISE_BASIC_INTERACTOR_OBSERVER_METHODS(object)              \
  {                                                                     \
    EXERCISE_BASIC_OBJECT_METHODS(object);                              \
    vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer< vtkRenderer >::New(); \
    vtkSmartPointer< vtkCamera > cam1 =  vtkSmartPointer< vtkCamera >::New(); \
    ren1->SetActiveCamera(cam1);                                        \
    vtkSmartPointer< vtkRenderWindow > renWin = vtkSmartPointer< vtkRenderWindow >::New(); \
    renWin->SetMultiSamples(0);                                         \
    renWin->AddRenderer(ren1);                                          \
    if (object->GetInteractor() != NULL)                                \
      {                                                                 \
      std::cout << "Object has an interactor already defined." << std::endl; \
      }                                                                 \
    vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New(); \
    iren->SetRenderWindow(renWin);                                      \
    object->SetInteractor(iren);                                        \
    if (object->GetInteractor() != iren)                                \
      {                                                                 \
      std::cerr << "Error in Set/GetInteractor" << std::endl;           \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    if (object->GetDefaultRenderer() != NULL)                           \
      {                                                                 \
      std::cout << "Object has default renderer already defined." << std::endl; \
      }                                                                 \
                                                                        \
    vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New(); \
    renWin->AddRenderer(ren);                                           \
    object->SetDefaultRenderer(ren);                                    \
    if (object->GetDefaultRenderer() != ren)                            \
      {                                                                 \
      std::cerr << "Error in Set/GetDefaultRenderer, default renderer is " << (object->GetDefaultRenderer() == NULL ? "NULL" : "not null") << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    object->SetCurrentRenderer(ren);                                    \
    if (object->GetCurrentRenderer() != ren)                            \
      {                                                                 \
      std::cerr << "Get current renderer failed." << std::endl;         \
      }                                                                 \
                                                                        \
    iren->Initialize();                                                 \
    renWin->Render();                                                   \
    if (0)                                                              \
      {                                                                 \
      object->CreateDefaultRepresentation();                            \
      TEST_SET_GET_BOOLEAN( object, Enabled);                           \
      object->On();                                                     \
      if (!object->GetEnabled())                                        \
        {                                                               \
        std::cerr << "Error in On" << std::endl;                        \
        return EXIT_FAILURE;                                            \
        }                                                               \
      object->Off();                                                    \
      if (object->GetEnabled())                                         \
        {                                                               \
        std::cerr << "Error in Off" << std::endl;                       \
        return EXIT_FAILURE;                                            \
        }                                                               \
      }                                                                 \
    TEST_SET_GET_DOUBLE( object, Priority, 0.0);                        \
    float min = object->GetPriorityMinValue();                          \
    float max = object->GetPriorityMaxValue();                          \
    std::cout << "Priority min = " << min << ", max = " << max << std::endl; \
    TEST_SET_GET_DOUBLE( object, Priority, 0.1f);                       \
    TEST_SET_GET_DOUBLE( object, Priority, 0.5f);                       \
    TEST_SET_GET_DOUBLE( object, Priority, 0.9f);                       \
    TEST_SET_GET_DOUBLE( object, Priority, 1.0f);                       \
                                                                        \
    TEST_SET_GET_BOOLEAN( object, KeyPressActivation);                  \
    TEST_SET_GET_CHAR( object, KeyPressActivationValue);                \
                                                                        \
    object->OnChar();                                                   \
    if (0)                                                              \
      {                                                                 \
      double worldPt[4];                                                \
      double x = 1.0, y = 1.0, z = 1.0;                                 \
      object->ComputeDisplayToWorld(ren, x, y, z, worldPt);             \
      std::cout << "Display " << x << "," << y << "," << z << " to world = " << worldPt[0] << "," << worldPt[1] << "," << worldPt[2] << "," << worldPt[3] << std::endl; \
      double displayPt[3];                                              \
      object->ComputeWorldToDisplay(ren, x, y, z, displayPt);           \
      std::cout << "World " << x << "," << y << "," << z << " to display = " << displayPt[0] << "," << displayPt[1] << "," << displayPt[2] << std::endl; \
      }                                                                 \
                                                                        \
    object->GrabFocus(NULL, NULL);                                      \
    object->ReleaseFocus();                                             \
  }

/// test vtkAbstractWidget methods
#define EXERCISE_BASIC_ABSTRACT_METHODS(object)         \
  {                                                     \
    EXERCISE_BASIC_INTERACTOR_OBSERVER_METHODS(object); \
    TEST_SET_GET_BOOLEAN( object, ProcessEvents);       \
    if (object->GetEventTranslator() == NULL)  \
      {                                                 \
      std::cerr << "Error getting event translator, is null." << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    object->CreateDefaultRepresentation();                              \
    object->Render();                                                   \
    if (object->GetParent() != NULL)                                    \
      {                                                                 \
      std::cerr << "Error, parent is not null." << std::endl;           \
      return EXIT_FAILURE;                                              \
      }                                                                 \
  }

/// test vtkBorderWidget methods
#define EXERCISE_BASIC_BORDER_METHODS(object)    \
  {                                              \
    EXERCISE_BASIC_ABSTRACT_METHODS(object);     \
    TEST_SET_GET_BOOLEAN( object, Selectable);   \
    TEST_SET_GET_BOOLEAN( object, Resizable);    \
  }

/// test vtkHoverWidget methods, timer duration is clamped so range macro will fail
#define EXERCISE_BASIC_HOVER_METHODS(object)            \
  {                                                     \
    EXERCISE_BASIC_ABSTRACT_METHODS (object);           \
    TEST_SET_GET_INT( object, TimerDuration, 1);       \
    TEST_SET_GET_INT( object, TimerDuration, 2);       \
    TEST_SET_GET_INT( object, TimerDuration, 50000); \
    TEST_SET_GET_INT( object, TimerDuration, 99999);  \
    TEST_SET_GET_INT( object, TimerDuration, 100000); \
  }

/// test vtkProp methods
#define EXERCISE_BASIC_PROP_METHODS(className, object)  \
  {                                                     \
    EXERCISE_BASIC_OBJECT_METHODS(object);              \
    vtkSmartPointer<vtkPropCollection> propCollection = vtkSmartPointer<vtkPropCollection>::New(); \
    object->GetActors(propCollection);                  \
    object->GetActors2D(propCollection);                \
    object->GetVolumes(propCollection);                 \
                                                        \
    TEST_SET_GET_BOOLEAN( object, Visibility);          \
    TEST_SET_GET_BOOLEAN( object, Pickable);            \
    TEST_SET_GET_BOOLEAN( object, Dragable);            \
    TEST_SET_GET_BOOLEAN( object, UseBounds);           \
    object->UseBoundsOff();                             \
                                                        \
    object->Pick();                                     \
                                                        \
    unsigned long redrawMTime = object->GetRedrawMTime();               \
    std::cout << "Redraw Modified Time = " << redrawMTime << std::endl; \
                                                                        \
    vtkSmartPointer< className > copyProp = vtkSmartPointer< className >::New(); \
    object->ShallowCopy(copyProp);                                      \
                                                                        \
    object->InitPathTraversal();                                        \
                                                                        \
    vtkSmartPointer<vtkAssemblyPath> assemblyPath = vtkSmartPointer<vtkAssemblyPath>::New(); \
    assemblyPath = object->GetNextPath();                               \
    std::cout << "Number of paths = " << object->GetNumberOfPaths() << std::endl; \
                                                                        \
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New(); \
    object->PokeMatrix(mat);                                            \
    mat = object->GetMatrix();                                          \
    if (mat == NULL)                                                    \
      {                                                                 \
      std::cout << "No matrix." << std::endl;                           \
      }                                                                 \
                                                                        \
    vtkSmartPointer<vtkInformation> info =  vtkSmartPointer<vtkInformation>::New(); \
    info = object->GetPropertyKeys();                                   \
    if (info != NULL)                                                   \
      {                                                                 \
      info->Print(std::cout);                                            \
      }                                                                 \
    else                                                                \
      {                                                                 \
      std::cout << "No property keys" << std::endl;                     \
      }                                                                 \
    object->SetPropertyKeys(info);                                      \
    std::cout << "Has null required keys? " << object->HasKeys(NULL) << std::endl; \
                                                                        \
    std::cout << "Skipping the internal render calls, requires vtkViewPort. Testing get macros." << std::endl; \
    std::cout << "HasTranslucentPolygonalGeometry = " << object->HasTranslucentPolygonalGeometry() << std::endl; \
    std::cout << "AllocatedRenderTime = " << object->GetAllocatedRenderTime() << std::endl; \
    std::cout << "RenderTimeMultiplier = " << object->GetRenderTimeMultiplier() << std::endl; \
    std::cout << "SupportsSelection = " << object->GetSupportsSelection() << std::endl; \
    std::cout << "NumberOfConsumers = " << object->GetNumberOfConsumers() << std::endl; \
                                                                        \
  }

#define NOT_DEFINED_CONSUMERS_FAIL()                                    \
  {                                                                     \
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New(); \
    object->AddConsumer(actor);                                         \
    if (object->IsConsumer(actor) != 1)                                 \
      {                                                                 \
      std::cerr << "Failed IsConsumer check for a valid consumer." << std::endl;\
      return EXIT_FAILURE;                                              \
      }                                                                 \
    if (object->IsConsumer(NULL) != 0)                                  \
      {                                                                 \
      std::cerr << "Failed IsConsumer check for a null consumer." << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    vtkSmartPointer<vtkActor> actor2 = object->GetConsumer(0);        \
    if (actor2 != actor)                                                \
      {                                                                 \
      std::cerr << "Failed get consumer check for a valid consumer." << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    object->RemoveConsumer(actor);                                      \
    actor2 = object->GetConsumer(0);                                    \
    if (actor2 != NULL)                                                 \
      {                                                                 \
      std::cerr << "Failed get consumer check for an invalid consumer number 0." << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
  }

/// test vtkWidgetRepresentation methods
#define EXERCISE_BASIC_REPRESENTATION_METHODS(className, object)   \
    EXERCISE_BASIC_PROP_METHODS(className, object);                     \
    std::cout << "Creating a renderer and a default widget..." << std::endl; \
    vtkSmartPointer< vtkCamera > cam1 =  vtkSmartPointer< vtkCamera >::New(); \
    vtkSmartPointer< vtkRenderer > ren1 = vtkSmartPointer< vtkRenderer >::New(); \
    ren1->SetActiveCamera(cam1);                                        \
    vtkSmartPointer< vtkRenderWindow > renWin = vtkSmartPointer< vtkRenderWindow >::New(); \
    renWin->SetMultiSamples(0);                                         \
    renWin->AddRenderer(ren1);                                          \
    vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New(); \
    iren->SetRenderWindow(renWin);                                      \
                                                                        \
                                                                        \
    object->SetRenderer(ren1);                                          \
    vtkSmartPointer<vtkRenderer> ren2 = object->GetRenderer();          \
    if (ren2 != ren1)                                                   \
      {                                                                 \
      std::cerr << "Failure in GetRenderer." << std::endl;              \
      return EXIT_FAILURE;                                              \
      }                                                                 \
                                                                        \
    object->BuildRepresentation();                                      \
                                                                        \
    double bounds[6] = {-1.0, 0.0, -10.0, 10.0, -5.0, 2.0};              \
    object->PlaceWidget(bounds);                                        \
    double *bounds2 = object->GetBounds();                              \
    if (bounds2 == NULL)                                                \
      {                                                                 \
      std::cout << "GetBounds is null." << std::endl;                   \
      }                                                                 \
    else                                                                \
      {                                                                 \
      std::cout << "Bounds = " << bounds[0] << "," << bounds[1] << "," << bounds[2] << "," << bounds[3] << "," << bounds[4] << "," << bounds[5] << std::endl; \
      }                                                                 \
                                                                        \
    double eventPos[2] = {10.0, 10.0};                                  \
    object->StartWidgetInteraction(eventPos);                           \
    object->WidgetInteraction(eventPos);                                \
    object->EndWidgetInteraction(eventPos);                             \
    std::cout << "InteractionState computed to be = " << object->ComputeInteractionState(10, 10, 0) << std::endl; \
    std::cout << "GetInteractionState = " << object->GetInteractionState() << std::endl; \
    object->Highlight(0);                                               \
    object->Highlight(1);                                               \
                                                                        \
    TEST_SET_GET_DOUBLE_RANGE(object, PlaceFactor, 1.01, 1000.0);       \
    TEST_SET_GET_DOUBLE_RANGE(object, HandleSize, 1.002, 999.0);        \
    TEST_SET_GET_BOOLEAN(object, NeedToRender);                         \
                                                                        \
    std::cout << "Trying to get back to init state for further testing." << std::endl; \
    object->SetPlaceFactor(0.5);                                        \
    object->SetHandleSize(0.05);                                        \
    std::cout << "Done basic rep methods" << std::endl;


/// test vtkAngleRepresentation methods
#define EXERCISE_BASIC_ANGLE_REPRESENTATION_METHODS(className, object)  \
  {                                                                     \
    EXERCISE_BASIC_REPRESENTATION_METHODS(className, object); \
                                                                        \
    vtkSmartPointer<vtkPointHandleRepresentation2D> phandle0 = vtkSmartPointer<vtkPointHandleRepresentation2D>::New(); \
    object->SetHandleRepresentation(phandle0);                           \
    object->InstantiateHandleRepresentation();                          \
                                                                        \
    std::cout << "GetAngle = " << object->GetAngle() << std::endl;      \
                                                                        \
    double pos[3];                                                      \
    object->GetPoint1WorldPosition(pos);                                \
    std::cout << "GetPoint1WorldPosition = " << pos[0] << ", " << pos[1] << ", " << pos[2] << std::endl; \
    object->GetCenterWorldPosition(pos);                                \
    std::cout << "GetCenterWorldPosition = " << pos[0] << ", " << pos[1] << ", " << pos[2] << std::endl; \
    object->GetPoint2WorldPosition(pos);                                \
    std::cout << "GetPoint2WorldPosition = " << pos[0] << ", " << pos[1] << ", " << pos[2] << std::endl; \
                                                                        \
    double pos2[3];                                                     \
    pos2[0] = -99.0;                                                    \
    pos2[1] = 99.0;                                                     \
    pos2[2] = 55.0;                                                     \
    object->SetCenterDisplayPosition(pos2);                             \
    object->GetCenterDisplayPosition(pos);                              \
    if (pos[0] != pos2[0] ||                                            \
        pos[0] != pos2[0] ||                                            \
        pos[0] != pos2[0])                                              \
      {                                                                 \
      std::cerr << "Failed to SetCenterDisplayPosition to " << pos2[0] << ", " << pos2[1] << ", " << pos2[2] << ", instead got " << pos[0] << ", " << pos[1] << ", " << pos[2] << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
                                                                        \
    pos[0] = -100.0;                                                    \
    object->SetPoint1DisplayPosition(pos2);                             \
    object->GetPoint1DisplayPosition(pos);                              \
    if (pos[0] != pos2[0] ||                                            \
        pos[0] != pos2[0] ||                                            \
        pos[0] != pos2[0])                                              \
      {                                                                 \
      std::cerr << "Failed to SetPoint1DisplayPosition to " << pos2[0] << ", " << pos2[1] << ", " << pos2[2] << ", instead got " << pos[0] << ", " << pos[1] << ", " << pos[2] << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
                                                                        \
    pos[0] = 101.0;                                                     \
    object->SetPoint2DisplayPosition(pos2);                             \
    object->GetPoint2DisplayPosition(pos);                              \
    if (pos[0] != pos2[0] ||                                            \
        pos[0] != pos2[0] ||                                            \
        pos[0] != pos2[0])                                              \
      {                                                                 \
      std::cerr << "Failed to SetPoint2DisplayPosition to " << pos2[0] << ", " << pos2[1] << ", " << pos2[2] << ", instead got " << pos[0] << ", " << pos[1] << ", " << pos[2] << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
                                                                        \
    vtkSmartPointer<vtkPointHandleRepresentation2D> phandle = vtkSmartPointer<vtkPointHandleRepresentation2D>::New(); \
    object->SetHandleRepresentation(phandle);                           \
    object->InstantiateHandleRepresentation();                          \
                                                                        \
    vtkSmartPointer<vtkHandleRepresentation> handleRep = NULL;          \
    handleRep = object->GetPoint1Representation();                      \
    handleRep = object->GetPoint2Representation();                      \
    handleRep = object->GetCenterRepresentation();                      \
                                                                        \
    TEST_SET_GET_INT_RANGE(object, Tolerance, 2, 99);                   \
    TEST_SET_GET_STRING( object, LabelFormat);                          \
    TEST_SET_GET_BOOLEAN( object, Ray1Visibility);                      \
    TEST_SET_GET_BOOLEAN( object, Ray2Visibility);                      \
    TEST_SET_GET_BOOLEAN( object, ArcVisibility);                       \
                                                                        \
    double e[2] = {5.0, 1.0};                                           \
    object->CenterWidgetInteraction(e);                                 \
  }

/// test vtkBorderRepresentation methods
#define EXERCISE_BASIC_BORDER_REPRESENTATION_METHODS(className, object)  \
  {                                                                     \
    EXERCISE_BASIC_REPRESENTATION_METHODS(className, object); \
                                                                        \
    double pos[2] = {10.0, 11.0};                                       \
    double *pos2 = NULL;                                                \
    object->SetPosition(pos);                                           \
    pos2 = object->GetPosition();                                       \
    if (pos2 == NULL)                                                   \
      {                                                                 \
      std::cerr << "Failure in Get/Set Position pos, got null position back." << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    else if (pos2[0] != pos[0] ||                                       \
        pos2[1] != pos[1])                                              \
      {                                                                 \
      std::cerr << "Failure in Get/Set Position pos, expected " << pos[0] << ", " << pos[1] << ", instead got " << pos2[0] << ", " << pos2[1] << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    else                                                                \
      {                                                                 \
      std::cout << "Set Position to "  << pos2[0] << ", " << pos2[1]  << std::endl; \
      }\
                                                                        \
    pos[0] = 12.0;                                                      \
    object->SetPosition(pos[0], pos[1]);                                \
    pos2 = object->GetPosition();                                       \
    if (pos2 == NULL ||                                                 \
        pos2[0] != pos[0] ||                                            \
        pos2[1] != pos[1])                                              \
      {                                                                 \
      std::cerr << "Failure in Get/Set Position x,y, expected " << pos[0] << ", " << pos[1]  << ", instead got " << pos2[0] << ", " << pos2[1]  << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    vtkSmartPointer<vtkCoordinate> coord = object->GetPositionCoordinate(); \
    pos2 = coord->GetValue();                                           \
    if (pos2 == NULL ||                                                 \
        pos2[0] != pos[0] ||                                            \
        pos2[1] != pos[1])                                              \
      {                                                                 \
      std::cerr << "Failure in Get/ Coordinate, expected " << pos[0] << ", " << pos[1] << ", instead got " << pos2[0] << ", " << pos2[1]  << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
                                                                        \
    pos[0] = 44.0;                                                      \
    object->SetPosition2(pos);                                           \
    pos2 = object->GetPosition2();                                       \
    if (pos2 == NULL ||                                                 \
        pos2[0] != pos[0] ||                                            \
        pos2[1] != pos[1])                                              \
      {                                                                 \
      std::cerr << "Failure in Get/Set Position2 pos, expected " << pos[0] << ", " << pos[1] << ", instead got " << pos2[0] << ", " << pos2[1]  << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    pos[0] = 12.0;                                                      \
    object->SetPosition2(pos[0], pos[1]);                                \
    pos2 = object->GetPosition2();                                       \
    if (pos2 == NULL ||                                                 \
        pos2[0] != pos[0] ||                                            \
        pos2[1] != pos[1])                                              \
      {                                                                 \
      std::cerr << "Failure in Get/Set Position2 x,y, expected " << pos[0] << ", " << pos[1] << ", instead got " << pos2[0] << ", " << pos2[1] << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    coord = object->GetPosition2Coordinate(); \
    pos2 = coord->GetValue();                                           \
    if (pos2 == NULL ||                                                 \
        pos2[0] != pos[0] ||                                            \
        pos2[1] != pos[1])                                              \
      {                                                                 \
      std::cerr << "Failure in Get/ Coordinate 2, expected " << pos[0] << ", " << pos[1] << ", instead got " << pos2[0] << ", " << pos2[1] << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
                                                                        \
    TEST_SET_GET_INT(object, ShowBorder, 0);                            \
    TEST_SET_GET_INT(object, ShowBorder, 1);                            \
    TEST_SET_GET_INT(object, ShowBorder, 2);                            \
    object->SetShowBorderToOff();                                       \
    object->SetShowBorderToOn();                                        \
    object->SetShowBorderToActive();                                    \
                                                                        \
    vtkSmartPointer<vtkProperty2D> borderProperty = object->GetBorderProperty(); \
                                                                        \
    TEST_SET_GET_BOOLEAN( object, ProportionalResize);                  \
                                                                        \
    TEST_SET_GET_VECTOR2_INT_RANGE(object, MinimumSize, 0, 100);        \
    TEST_SET_GET_VECTOR2_INT_RANGE(object, MaximumSize, 0, 100);        \
    TEST_SET_GET_INT_RANGE(object, Tolerance, 2, 9);                    \
                                                                        \
    double *selPoint = object->GetSelectionPoint();                     \
    if (selPoint)                                                       \
      {                                                                 \
      std::cout << "Selection Point = " << selPoint[0] << ", " << selPoint[1] << std::endl; \
      }                                                                 \
                                                                        \
    TEST_SET_GET_BOOLEAN( object,Moving);                               \
                                                                        \
    double size[2];                                                     \
    object->GetSize(size);                                              \
    std::cout << "Size = " << size[0] << ", " << size[1] << std::endl;  \
                                                                        \
    int interactionState = object->ComputeInteractionState(10, 10);     \
    std::cout << "Interaction state = " << interactionState << std::endl; \
  }

/// test vtkAngleRepresentation methods
#define EXERCISE_BASIC_IMPLICIT_PLANE_REPRESENTATION_METHODS(className, object)  \
  {                                                                     \
    EXERCISE_BASIC_REPRESENTATION_METHODS(className, object);           \
                                                                        \
    TEST_SET_GET_VECTOR3_DOUBLE_RANGE(node1, Origin, -100, 100);        \
    TEST_SET_GET_VECTOR3_DOUBLE_RANGE(node1, Normal, -1, 1);            \
    TEST_SET_GET_BOOLEAN( node1, NormalToXAxis);                        \
    TEST_SET_GET_BOOLEAN( node1, NormalToYAxis);                        \
    TEST_SET_GET_BOOLEAN( node1, NormalToZAxis);                        \
    TEST_SET_GET_BOOLEAN( node1, Tubing);                               \
    TEST_SET_GET_BOOLEAN( node1, DrawPlane);                            \
    TEST_SET_GET_BOOLEAN( node1, OutlineTranslation);                   \
    TEST_SET_GET_BOOLEAN( node1, OutsideBounds);                        \
    TEST_SET_GET_BOOLEAN( node1, ScaleEnabled);                         \
  }


/// test objects that have Property and SelectedProperty set/get, with vtkProperty
#define TEST_SET_GET_PROPERTY(object, variable)\
  {                                                                     \
    vtkSmartPointer<vtkProperty> prop1 = vtkSmartPointer<vtkProperty>::New(); \
    double colour[3] = {0.2, 0.3, 0.4};                                 \
    prop1->SetColor(colour);                                            \
    node1->Set##variable(prop1);                                          \
    vtkSmartPointer<vtkProperty> prop = node1->Get##variable();           \
    if (!prop)                                                          \
      {                                                                 \
      std::cerr << "Got null variable property back after setting it!" << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    double *col = prop->GetColor();                                     \
    if (!col)                                                           \
      {                                                                 \
      std::cerr << "Got null colour back!" << std::endl;                \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    if (col[0] != colour[0] ||                                          \
        col[1] != colour[1] ||                                          \
        col[2] != colour[2])                                            \
      {                                                                 \
      std::cerr << "Got wrong colour back after setting it! Expected " << colour[0] << ", " << colour[1] << ", " << colour[2] << ", but got " << col[0] << ", " << col[1] << ", " << col[2] << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
                                                                        \
                                                                        \
  }                                                                     \

/// test vtkHandleRepresentation methods
/// these don't work well in isolation, seg faults when try to get/set display
/// and world positions.
#define EXERCISE_BASIC_HANDLE_REPRESENTATION_METHODS(className, object) \
  {                                                                     \
    EXERCISE_BASIC_REPRESENTATION_METHODS(className, object);          \
                                                                        \
    double pos[3];                                                      \
    pos[0] = 0.1;                                                       \
    pos[1] = -1.0;                                                      \
    pos[2] = 3.6;                                                       \
    double pos2[3];                                                     \
    double *pos3;                                                       \
                                                                        \
                                                                        \
    std::cout << "Testing SetWorldPosition" << std::endl;          \
                                                                        \
    object->SetWorldPosition(pos);                                      \
    std::cout << "Testing GetWorldPosition" << std::endl;          \
    object->GetWorldPosition(pos2);                                     \
    if (pos2[0] != pos[0] ||                                            \
        pos2[1] != pos[1] ||                                            \
        pos2[2] != pos[2])                                              \
      {                                                                 \
      std::cerr << "Failure in Get WorldPosition pos2, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << ", instead got " << pos2[0] << ", " << pos2[1] << ", " << pos2[2] << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    pos3 = object->GetWorldPosition();                                  \
    if (!pos3)                                                          \
      {                                                                 \
      std::cerr << "Failure in double * GetWorldPosition , expected " << pos[0] << ", " << pos[1] <<  ", " << pos[2] << ", instead got a null pointer." << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    if (pos3[0] != pos[0] ||                                            \
        pos3[1] != pos[1] ||                                            \
        pos3[2] != pos[2])                                              \
      {                                                                 \
      std::cerr << "Failure in double * GetWorldyPosition , expected " << pos[0] << ", " << pos[1] <<  ", " << pos[2] << ", instead got " << pos3[0] << ", " << pos3[1] << ", " << pos3[2] << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    std::cout << "Done testing world position." << std::endl;           \
                                                                        \
    std::cout << "Testing Set/Get Display Position" << std::endl;       \
                                                                        \
                                                                        \
    object->GetDisplayPosition(pos2);                                   \
    std::cout << "After GetDisplayPosition." << std::endl;              \
    object->SetDisplayPosition(pos);                                    \
    std::cout << "After SetDisplayPosition" << std::endl;               \
    object->GetDisplayPosition(pos2);                                   \
    std::cout << "After GetDisplayPosition second time." << std::endl;  \
    if (pos2[0] != pos[0] ||                                            \
        pos2[1] != pos[1])                                              \
      {                                                                 \
      std::cerr << "Failure in Get DisplayPosition pos2, expected " << pos[0] << ", " << pos[1] << ", instead got " << pos2[0] << ", " << pos2[1] << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    pos3 = object->GetDisplayPosition();                                \
    if (!pos3)                                                          \
      {                                                                 \
      std::cerr << "Failure in double * GetDisplayPosition , expected " << pos[0] << ", " << pos[1] << ", instead got a null pointer." << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    if (pos3[0] != pos[0] ||                                            \
        pos3[1] != pos[1])                                              \
      {                                                                 \
      std::cerr << "Failure in double * GetDisplayPosition , expected " << pos[0] << ", " << pos[1] << ", instead got " << pos3[0] << ", " << pos3[1] << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    TEST_SET_GET_INT_RANGE(object, Tolerance, 2, 99);                   \
    TEST_SET_GET_BOOLEAN( object, ActiveRepresentation);                \
    TEST_SET_GET_BOOLEAN( object, Constrained);                         \
                                                                        \
    vtkSmartPointer<vtkRenderer> ren2 = object->GetRenderer();          \
    double posToCheck[3] = {0.0, 0.0, 0.0};                             \
    int flag = object->CheckConstraint(ren2, posToCheck);               \
    std::cout << "Check Constraint = " << flag << std::endl;            \
                                                                        \
    std::cout << "MTime = " << object->GetMTime() << std::endl;         \
                                                                        \
    vtkSmartPointer<vtkPointPlacer> pplacer = vtkSmartPointer<vtkPointPlacer>::New(); \
    object->SetPointPlacer(pplacer);                                    \
    vtkSmartPointer<vtkPointPlacer> pplacer2 = object->GetPointPlacer(); \
    if (pplacer2 != pplacer)                                            \
      {                                                                 \
      std::cerr << "Error in Set/Get point placer." << std::endl;       \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    flag = object->CheckConstraint(ren2, posToCheck);                   \
    std::cout << "Check Constraint after setting point placer = " << flag << std::endl; \
  }

/// test vtkAbstractPolygonalHandleRepresentation3D methods
#define EXERCISE_BASIC_ABSTRACT_POLYGONAL_HANDLE_REPRESENTATION3D_METHODS(className, object) \
  {                                                                     \
    EXERCISE_BASIC_HANDLE_REPRESENTATION_METHODS(className, object);    \
                                                                        \
    vtkSmartPointer<vtkPolyData> pd =  vtkSmartPointer<vtkPolyData>::New(); \
    object->SetHandle(pd);                                              \
    vtkSmartPointer<vtkPolyData> pd2 = object->GetHandle();             \
    if (pd2 == NULL)                                                    \
      {                                                                 \
      std::cerr << "Error getting handle, null pointer." << std::endl;  \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    if (pd2 != pd)                                                     \
      {                                                                 \
      std::cerr << "Error getting handle, not the same as set." << std::endl; \
      return EXIT_FAILURE;                                              \
      }                                                                 \
    TEST_SET_GET_PROPERTY(object, Property);                            \
    TEST_SET_GET_PROPERTY(object, SelectedProperty);                    \
                                                                        \
    vtkSmartPointer<vtkAbstractTransform> at = object->GetTransform();  \
                                                                        \
    TEST_SET_GET_BOOLEAN(object, LabelVisibility);                      \
    TEST_SET_GET_STRING(object, LabelText);                              \
    TEST_SET_GET_VECTOR3_DOUBLE_RANGE(object, LabelTextScale, 0.0, 10.0); \
                                                                        \
    vtkSmartPointer<vtkFollower> follower = object->GetLabelTextActor(); \
    if (follower == NULL)                                               \
      {                                                                 \
      std::cout << "Follower is null." << std::endl;                    \
      }                                                                 \
                                                                        \
    object->SetUniformScale(-1.0);                                      \
    object->SetUniformScale(0.0);                                       \
    object->SetUniformScale(1.0);                                       \
    object->SetUniformScale(35.44);                                     \
                                                                        \
    TEST_SET_GET_BOOLEAN(object, HandleVisibility);                     \
  }
#endif
