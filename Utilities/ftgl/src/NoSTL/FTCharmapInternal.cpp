#include  "FTCharmapInternal.h"
#include  <stdlib.h>
#include  <stdio.h>

const int FTCharmapInternal::NumberOfBuckets = 256;
const int FTCharmapInternal::BucketSize = 256;
const FTCharmapInternal::GlyphIndex FTCharmapInternal::IndexNotFound = (FTCharmapInternal::GlyphIndex)-1;


FTCharmapInternal::FTCharmapInternal()
{
  this->Indices = 0;
}


FTCharmapInternal::~FTCharmapInternal()
{
  if (this->Indices)
    {
    // Free all buckets

    this->clear();

    // Free main structure

    delete [] this->Indices;
    this->Indices = 0;
    }
}

// Free all buckets

void FTCharmapInternal::clear()
{
  if (this->Indices)
    {
    for (int i = 0; i < FTCharmapInternal::NumberOfBuckets; i++)
      {
      if (this->Indices[i])
        {
        delete [] this->Indices[i];
        this->Indices[i] = 0;
        }
      }
    }
}

// Find index corresponding to char code

const FTCharmapInternal::GlyphIndex* FTCharmapInternal::find(FTCharmapInternal::CharacterCode c)
{
  if (!this->Indices)
    {
    return 0;
    }

  // Find position of char code in buckets

  div_t pos = div(c, FTCharmapInternal::BucketSize);

  if (!this->Indices[pos.quot])
    {
    return 0;
    }

  const FTCharmapInternal::GlyphIndex *ptr = &this->Indices[pos.quot][pos.rem];
  if (*ptr == FTCharmapInternal::IndexNotFound)
    {
    return 0;
    }

  return ptr;
}

// Insert index corresponding to char code

void FTCharmapInternal::insert(FTCharmapInternal::CharacterCode c,
                               FTCharmapInternal::GlyphIndex g)
{
  if (!this->Indices)
    {
    this->Indices = new GlyphIndex* [FTCharmapInternal::NumberOfBuckets];
    for (int i = 0; i < FTCharmapInternal::NumberOfBuckets; i++)
      {
        this->Indices[i] = 0;
      }
    }

  // Find position of char code in buckets

  div_t pos = div(c, FTCharmapInternal::BucketSize);

  // Allocate bucket if does not exist yet

  if (!this->Indices[pos.quot])
    {
    this->Indices[pos.quot] = new GlyphIndex [FTCharmapInternal::BucketSize];
    for (int i = 0; i < FTCharmapInternal::BucketSize; i++)
      {
      this->Indices[pos.quot][i] = FTCharmapInternal::IndexNotFound;
      }
    }
  
  this->Indices[pos.quot][pos.rem] = g;
}
