// forward iterator that visits 3D array block by block; this class is nested within zfp::array3
class iterator {
public:
  // typedefs for STL compatibility
  typedef Scalar value_type;
  typedef ptrdiff_t difference_type;
  typedef typename array3::reference reference;
  typedef typename array3::pointer pointer;
  typedef std::forward_iterator_tag iterator_category;

  iterator() : ref(0, 0, 0, 0) {}
  iterator operator=(const iterator& it) { ref.array = it.ref.array; ref.i = it.ref.i; ref.j = it.ref.j; ref.k = it.ref.k; return *this; }
  reference operator*() const { return ref; }
  iterator& operator++() { increment(); return *this; }
  iterator operator++(int) { iterator it = *this; increment(); return it; }
  bool operator==(const iterator& it) const { return ref.array == it.ref.array && ref.i == it.ref.i && ref.j == it.ref.j && ref.k == it.ref.k; }
  bool operator!=(const iterator& it) const { return !operator==(it); }
  uint i() const { return ref.i; }
  uint j() const { return ref.j; }
  uint k() const { return ref.k; }

protected:
  friend class array3;
  explicit iterator(array3* array, uint i, uint j, uint k) : ref(array, i, j, k) {}
  void increment()
  {
    ref.i++;
    if (!(ref.i & 3u) || ref.i == ref.array->nx) {
      ref.i = (ref.i - 1) & ~3u;
      ref.j++;
      if (!(ref.j & 3u) || ref.j == ref.array->ny) {
        ref.j = (ref.j - 1) & ~3u;
        ref.k++;
        if (!(ref.k & 3u) || ref.k == ref.array->nz) {
          ref.k = (ref.k - 1) & ~3u;
          // done with block; advance to next
          if ((ref.i += 4) >= ref.array->nx) {
            ref.i = 0;
            if ((ref.j += 4) >= ref.array->ny) {
              ref.j = 0;
              if ((ref.k += 4) >= ref.array->nz)
                ref.k = ref.array->nz;
            }
          }
        }
      }
    }
  }
  reference ref;
};
