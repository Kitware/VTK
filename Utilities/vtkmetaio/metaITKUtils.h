/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
// Utility file - definition of loadImage
// Templated over the Pixel Type

#include "metaTypes.h"

#ifndef ITKMetaIO_METAITKUTILS_H
#define ITKMetaIO_METAITKUTILS_H

#include "metaImage.h"
#include "itkImage.h"
#include "itkProcessObject.h"
#include "itkImageRegionIterator.h"

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

template <class T>
typename itk::Image<T, 3>::Pointer
metaITKUtilLoadImage3D(const char *fname, MET_ValueEnumType _toType,
                       double _toMinValue=0, double _toMaxValue=0)
  {
  MetaImage *imIO = new MetaImage();
  imIO->Read(fname);
  imIO->PrintInfo();
  imIO->ConvertElementDataTo(_toType, _toMinValue, _toMaxValue);

  typedef itk::Image<T,3>  ImageType;

  typedef typename ImageType::Pointer     ImagePointer;
  typedef typename ImageType::SizeType    SizeType;
  typedef typename ImageType::IndexType   IndexType;
  typedef typename ImageType::RegionType  RegionType;

  ImagePointer image = ImageType::New();

  SizeType size;

  double spacing[3];

  size[0] = imIO->DimSize()[0];
  size[1] = imIO->DimSize()[1];
  if(imIO->NDims()>2)
    size[2] = imIO->DimSize()[2];
  else
    size[2] = 1;

  spacing[0] = imIO->ElementSpacing()[0];
  spacing[1] = imIO->ElementSpacing()[1];
  if(imIO->NDims()>2)
    spacing[2] = imIO->ElementSpacing()[2];
  else
    spacing[2] = imIO->ElementSpacing()[1];

  if (spacing[0] == 0)
    {
    spacing[0] = 1;
    }
  if (spacing[1] == 0)
    {
    spacing[1] = 1;
    }
  if (spacing[2] == 0)
    {
    spacing[2] = 1;
    }

  IndexType start;
  start.Fill(0);

  RegionType region;
  region.SetSize(size);
  region.SetIndex( start );
  image->SetLargestPossibleRegion(region);
  image->SetBufferedRegion(region);
  image->SetRequestedRegion(region);
  image->SetSpacing(spacing);
  image->Allocate();


  itk::ImageRegionIterator< ImageType > it(image, region);
  it.Begin();
  for(unsigned int i = 0; !it.IsAtEnd(); i++, ++it)
    {
    it.Set( static_cast< typename ImageType::PixelType >( imIO->ElementData(i) ));
    }


  return image;
  }

template <class imageT>
bool metaITKUtilSaveImage(const char *fname, const char *dname,
                          typename imageT::Pointer _im,
                          MET_ValueEnumType _fromType,
                          int _numberOfChannels,
                          MET_ValueEnumType _toType,
                          double _toMinValue=0, double _toMaxValue=0)
  {
  int i;
  int nd = _im->GetImageDimension();
  int * si = new int[nd];
  float * sp = new float[nd];
  for(i=0; i<nd; i++)
    {
    si[i] = _im->GetLargestPossibleRegion().GetSize()[i];
    sp[i] = _im->GetSpacing()[i];
    }
  MetaImage imIO(_im->GetImageDimension(), si, sp,
                 _fromType, _numberOfChannels,
                 (void *)_im->GetBufferPointer());
  delete si;
  delete sp;

  imIO.ConvertElementDataTo(_toType, _toMinValue, _toMaxValue);

  bool res = imIO.Write(fname, dname);

  return res;
  }

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
