#ifndef DIY_PARTNERS_COMMON_HPP
#define DIY_PARTNERS_COMMON_HPP

#include "../decomposition.hpp"
#include "../types.hpp"

namespace diy
{

struct RegularPartners
{
  // The record of group size per round in a dimension
  struct DimK
  {
            DimK(int dim_, int k_):
                dim(dim_), size(k_)               {}

    int dim;
    int size;           // group size
  };

  typedef       std::vector<int>                    CoordVector;
  typedef       std::vector<int>                    DivisionVector;
  typedef       std::vector<DimK>                   KVSVector;

  // The part of RegularDecomposer that we need works the same with either Bounds (so we fix them arbitrarily)
  typedef       DiscreteBounds                      Bounds;
  typedef       RegularDecomposer<Bounds>           Decomposer;

  template<class Decomposer_>
                RegularPartners(const Decomposer_& decomposer, int k, bool contiguous = true):
                  divisions_(decomposer.divisions),
                  contiguous_(contiguous)                       { factor(k, divisions_, kvs_); fill_steps(); }
                RegularPartners(const DivisionVector&   divs,
                                const KVSVector&        kvs,
                                bool  contiguous = true):
                  divisions_(divs), kvs_(kvs),
                  contiguous_(contiguous)                       { fill_steps(); }

  size_t        rounds() const                                  { return kvs_.size(); }
  int           size(int round) const                           { return kvs_[round].size; }
  int           dim(int round) const                            { return kvs_[round].dim; }

  int           step(int round) const                           { return steps_[round]; }

  const DivisionVector&     divisions() const                   { return divisions_; }
  const KVSVector&          kvs() const                         { return kvs_; }
  bool                      contiguous() const                  { return contiguous_; }

  static
  inline void   factor(int k, const DivisionVector& divisions, KVSVector& kvs);

  inline void   fill(int round, int gid, std::vector<int>& partners) const;
  inline int    group_position(int round, int c, int step) const;

  private:
    inline void fill_steps();
    static
    inline void factor(int k, int tot_b, std::vector<int>& kvs);

    DivisionVector      divisions_;
    KVSVector           kvs_;
    bool                contiguous_;
    std::vector<int>    steps_;
};

}

void
diy::RegularPartners::
fill_steps()
{
  if (contiguous_)
  {
    std::vector<int>    cur_steps(divisions().size(), 1);

    for (size_t r = 0; r < rounds(); ++r)
    {
      steps_.push_back(cur_steps[kvs_[r].dim]);
      cur_steps[kvs_[r].dim] *= kvs_[r].size;
    }
  } else
  {
    std::vector<int>    cur_steps(divisions().begin(), divisions().end());
    for (size_t r = 0; r < rounds(); ++r)
    {
      cur_steps[kvs_[r].dim] /= kvs_[r].size;
      steps_.push_back(cur_steps[kvs_[r].dim]);
    }
  }
}

void
diy::RegularPartners::
fill(int round, int gid, std::vector<int>& partners) const
{
  const DimK&   kv  = kvs_[round];
  partners.reserve(kv.size);

  int step = this->step(round);       // gids jump by this much in the current round

  CoordVector   coords;
  Decomposer::gid_to_coords(gid, coords, divisions_);
  int c   = coords[kv.dim];
  int pos = group_position(round, c, step);

  int partner = c - pos * step;
  coords[kv.dim] = partner;
  int partner_gid = Decomposer::coords_to_gid(coords, divisions_);
  partners.push_back(partner_gid);

  for (int k = 1; k < kv.size; ++k)
  {
    partner += step;
    coords[kv.dim] = partner;
    int partner_gid = Decomposer::coords_to_gid(coords, divisions_);
    partners.push_back(partner_gid);
  }
}

// Tom's GetGrpPos
int
diy::RegularPartners::
group_position(int round, int c, int step) const
{
  // the second term in the following expression does not simplify to
  // (gid - start_b) / kv[r]
  // because the division gid / (step * kv[r]) is integer and truncates
  // this is exactly what we want
  int g = c % step + c / (step * kvs_[round].size) * step;
  int p = c / step % kvs_[round].size;
  static_cast<void>(g);        // shut up the compiler

  // g: group number (output)
  // p: position number within the group (output)
  return p;
}

void
diy::RegularPartners::
factor(int k, const DivisionVector& divisions, KVSVector& kvs)
{
  // factor in each dimension
  std::vector< std::vector<int> >       tmp_kvs(divisions.size());
  for (unsigned i = 0; i < divisions.size(); ++i)
    factor(k, divisions[i], tmp_kvs[i]);

  // interleave the dimensions
  std::vector<int>  round_per_dim(divisions.size(), 0);
  while(true)
  {
    // TODO: not the most efficient way to do this
    bool changed = false;
    for (unsigned i = 0; i < divisions.size(); ++i)
    {
      if (round_per_dim[i] == (int) tmp_kvs[i].size())
        continue;
      kvs.push_back(DimK(i, tmp_kvs[i][round_per_dim[i]++]));
      changed = true;
    }
    if (!changed)
        break;
  }
}

// Tom's FactorK
void
diy::RegularPartners::
factor(int k, int tot_b, std::vector<int>& kv)
{
  int rem = tot_b; // unfactored remaining portion of tot_b
  int j;

  while (rem > 1)
  {
    // remainder is divisible by k
    if (rem % k == 0)
    {
      kv.push_back(k);
      rem /= k;
    }
    // if not, start at k and linearly look for smaller factors down to 2
    else
    {
      for (j = k - 1; j > 1; j--)
      {
        if (rem % j == 0)
        {
          kv.push_back(j);
          rem /= k;
          break;
        }
      }
      if (j == 1)
      {
        kv.push_back(rem);
        rem = 1;
      }
    } // else
  } // while
}


#endif
