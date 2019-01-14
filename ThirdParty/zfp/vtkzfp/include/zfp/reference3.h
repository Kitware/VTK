// reference to a 3D array element; this class is nested within zfp::array3
class reference {
public:
  operator Scalar() const { return array->get(i, j, k); }
  reference operator=(const reference& r) { array->set(i, j, k, r.operator Scalar()); return *this; }
  reference operator=(Scalar val) { array->set(i, j, k, val); return *this; }
  reference operator+=(Scalar val) { array->add(i, j, k, val); return *this; }
  reference operator-=(Scalar val) { array->sub(i, j, k, val); return *this; }
  reference operator*=(Scalar val) { array->mul(i, j, k, val); return *this; }
  reference operator/=(Scalar val) { array->div(i, j, k, val); return *this; }
  pointer operator&() const { return pointer(*this); }
  // swap two array elements via proxy references
  friend void swap(reference a, reference b)
  {
    Scalar x = a.operator Scalar();
    Scalar y = b.operator Scalar();
    b.operator=(x);
    a.operator=(y);
  }

protected:
  friend class array3;
  friend class iterator;
  explicit reference(array3* array, uint i, uint j, uint k) : array(array), i(i), j(j), k(k) {}
  array3* array;
  uint i, j, k;
};
