//
//  VTKViewController.m
//  VTKViewer
//
//  Created by Max Smolens on 6/19/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

#import "VTKViewController.h"

#import "VTKGestureHandler.h"
#import "VTKLoader.h"
#import "VTKView.h"

#include "vtkActor.h"
#include "vtkCubeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

@interface VTKViewController ()

@property (strong, nonatomic) NSArray<NSURL*>* initialUrls;

// Views
@property (strong, nonatomic) IBOutlet VTKView* vtkView;
@property (strong, nonatomic) IBOutlet UIVisualEffectView* headerContainer;

// VTK
@property (nonatomic) vtkSmartPointer<vtkRenderer> renderer;

// VTK Logic
@property (nonatomic) VTKGestureHandler* vtkGestureHandler;

@end

@implementation VTKViewController

// MARK: UIViewController

- (void)viewDidLoad
{
  [super viewDidLoad];

  // UI Gestures
  [self setupGestures];

  // Rendering
  [self setupRenderer];

  // VTK Logic
  self.vtkGestureHandler = [[VTKGestureHandler alloc] initWithVtkView:self.vtkView];
}

- (void)didReceiveMemoryWarning
{
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}

// MARK: Renderer

- (void)setupRenderer
{
  self.renderer = vtkSmartPointer<vtkRenderer>::New();
  self.renderer->SetBackground(0.4, 0.4, 0.4);
  self.renderer->SetBackground2(0.2, 0.2, 0.2);
  self.renderer->GradientBackgroundOn();
  self.vtkView.renderWindow->AddRenderer(self.renderer);

  // Load initial data
  if (self.initialUrls)
  {
    // If URL given when launching app,
    // load that file
    [self loadFiles:self.initialUrls];
    self.initialUrls = nil;
  }
  else
  {
    // If no data is explicitly requested,
    // add dummy cube
    auto cubeSource = vtkSmartPointer<vtkCubeSource>::New();
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(cubeSource->GetOutputPort());
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    self.renderer->AddActor(actor);
    self.renderer->ResetCamera();
  }
}

// MARK: Gestures

- (void)setupGestures
{
  // Add the double tap gesture recognizer
  UITapGestureRecognizer* doubleTapRecognizer =
    [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onDoubleTap:)];
  [doubleTapRecognizer setNumberOfTapsRequired:2];
  [self.view addGestureRecognizer:doubleTapRecognizer];
  doubleTapRecognizer.delegate = self;
}

- (void)onDoubleTap:(UITapGestureRecognizer*)sender
{
  [UIView animateWithDuration:0.2
                   animations:^{
                     BOOL show = self.headerContainer.alpha < 1.0;
                     self.headerContainer.alpha = show ? 1.0 : 0.0;
                   }];
}

// MARK: Files

- (NSArray<NSString*>*)supportedFileTypes
{
  NSArray* documentTypes =
    [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDocumentTypes"];
  NSDictionary* vtkDocumentType = [documentTypes objectAtIndex:0];
  return [vtkDocumentType objectForKey:@"LSItemContentTypes"];
}

- (IBAction)onAddDataButtonPressed:(id)sender
{
  UIDocumentPickerViewController* documentPicker =
    [[UIDocumentPickerViewController alloc] initWithDocumentTypes:[self supportedFileTypes]
                                                           inMode:UIDocumentPickerModeImport];
  documentPicker.delegate = self;
  documentPicker.allowsMultipleSelection = true;
  documentPicker.modalPresentationStyle = UIModalPresentationFormSheet;
  [self presentViewController:documentPicker animated:YES completion:nil];
}

- (void)documentPicker:(UIDocumentPickerViewController*)controller
  didPickDocumentsAtURLs:(nonnull NSArray<NSURL*>*)urls
{
  [self loadFiles:urls];
}

- (void)loadFiles:(nonnull NSArray<NSURL*>*)urls
{
  // If the view is not yet loaded, keep track of the url
  // and load it after everything is initialized
  if (!self.isViewLoaded)
  {
    self.initialUrls = urls;
    return;
  }

  // First Check if scene is empty.
  if (self.renderer->GetViewProps()->GetNumberOfItems() == 0)
  {
    // Directly load the file. Not necessary to reset the scene as there's nothing to reset.
    [self loadFilesInternal:urls];
  }
  else
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      UIAlertController* alertController =
        [UIAlertController alertControllerWithTitle:@"Import"
                                            message:@"There are other objects in the scene."
                                     preferredStyle:UIAlertControllerStyleAlert];

      // Completion handler for selected action in alertController.
      void (^onSelectedAction)(UIAlertAction*) = ^(UIAlertAction* action) {
        // Reset the scene if "Replace" was selected.
        if (action.style == UIAlertActionStyleCancel)
        {
          self.renderer->RemoveAllViewProps();
        }
        [self loadFilesInternal:urls];
      };

      // Two actions : Insert file in scene and Reset the scene.
      [alertController addAction:[UIAlertAction actionWithTitle:@"Add"
                                                          style:UIAlertActionStyleDefault
                                                        handler:onSelectedAction]];
      [alertController addAction:[UIAlertAction actionWithTitle:@"Replace"
                                                          style:UIAlertActionStyleCancel
                                                        handler:onSelectedAction]];

      [self presentViewController:alertController animated:YES completion:nil];
    });
  }
}

- (void)loadFilesInternal:(nonnull NSArray<NSURL*>*)urls
{
  for (NSURL* url in urls)
  {
    [self loadFileInternal:url];
  }
}

- (void)loadFileInternal:(NSURL*)url
{
  vtkSmartPointer<vtkActor> actor = [VTKLoader loadFromURL:url];

  NSString* alertTitle;
  NSString* alertMessage;
  if (actor)
  {
    self.renderer->AddActor(actor);
    self.renderer->ResetCamera();
    [self.vtkView setNeedsDisplay];
    alertTitle = @"Import";
    alertMessage = [NSString stringWithFormat:@"Imported %@", [url lastPathComponent]];
  }
  else
  {
    alertTitle = @"Import Failed";
    alertMessage = [NSString stringWithFormat:@"Could not load %@", [url lastPathComponent]];
  }

  dispatch_async(dispatch_get_main_queue(), ^{
    UIAlertController* alertController =
      [UIAlertController alertControllerWithTitle:alertTitle
                                          message:alertMessage
                                   preferredStyle:UIAlertControllerStyleAlert];
    [alertController addAction:[UIAlertAction actionWithTitle:@"Ok"
                                                        style:UIAlertActionStyleDefault
                                                      handler:nil]];
    [self presentViewController:alertController animated:YES completion:nil];
  });
}

@end
