//
//  VTKLoader.m
//  VTKViewer
//
//  Created by Alexis Girault on 11/17/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

#import "VTKLoader.h"

#include "vtkCubeSource.h"
#include "vtkDataSetMapper.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkOBJReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataReader.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSTLReader.h"
#include "vtkXMLGenericDataObjectReader.h"

@implementation VTKLoader

+ (vtkSmartPointer<vtkActor>)loadFromURL:(NSURL*)url
{
  // Setup file path
  const char* cpath = [[url path] UTF8String];

  // Setup proper reader
  BOOL polydata = NO;
  vtkSmartPointer<vtkAlgorithm> source;
  if ([self fileAtURL:url matchesExtension:@[ @"vtk" ]])
  {
    auto reader = vtkSmartPointer<vtkGenericDataObjectReader>::New();
    reader->SetFileName(cpath);
    polydata = (reader->GetPolyDataOutput() != nullptr);
    source = reader;
  }
  else if ([self fileAtURL:url matchesExtension:@[ @"vti", @"vtp", @"vtr", @"vts", @"vtu" ]])
  {
    auto reader = vtkSmartPointer<vtkXMLGenericDataObjectReader>::New();
    reader->SetFileName(cpath);
    polydata = (reader->GetPolyDataOutput() != nullptr);
    source = reader;
  }
  else if ([self fileAtURL:url matchesExtension:@[ @"obj" ]])
  {
    auto reader = vtkSmartPointer<vtkOBJReader>::New();
    reader->SetFileName(cpath);
    polydata = YES;
    source = reader;
  }
  else if ([self fileAtURL:url matchesExtension:@[ @"stl" ]])
  {
    auto reader = vtkSmartPointer<vtkSTLReader>::New();
    reader->SetFileName(cpath);
    polydata = YES;
    source = reader;
  }
  else
  {
    NSLog(@"No reader found for extension: %@", [url pathExtension]);
    return nil;
  }

  // Check reader worked
  source->Update();
  if (source->GetErrorCode())
  {
    NSLog(@"Error loading file: %@", [url lastPathComponent]);
    return nil;
  }

  // Setup mapper
  vtkSmartPointer<vtkMapper> mapper;
  if (polydata)
  {
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  }
  else
  {
    mapper = vtkSmartPointer<vtkDataSetMapper>::New();
  }
  mapper->SetInputConnection(source->GetOutputPort());

  // Setup actor
  auto actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  return actor;
}

+ (BOOL)fileAtURL:(NSURL*)url matchesExtension:(NSArray<NSString*>*)validExtensions
{
  // Actual extension
  NSString* fileExt = [url pathExtension];

  // Check if one of the valid extensions
  for (NSString* validExt in validExtensions)
  {
    // Case insensitive comparison
    if ([fileExt caseInsensitiveCompare:validExt] == NSOrderedSame)
    {
      return YES;
    }
  }
  return NO;
}

@end
