// reference to a 1D array element; this class is nested within zfp::array1
class reference {
public:
  operator Scalar() const { return array->get(i); }
  reference operator=(const reference& r) { array->set(i, r.operator Scalar()); return *this; }
  reference operator=(Scalar val) { array->set(i, val); return *this; }
  reference operator+=(Scalar val) { array->add(i, val); return *this; }
  reference operator-=(Scalar val) { array->sub(i, val); return *this; }
  reference operator*=(Scalar val) { array->mul(i, val); return *this; }
  reference operator/=(Scalar val) { array->div(i, val); return *this; }
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
  friend class array1;
  friend class iterator;
  explicit reference(array1* array, uint i) : array(array), i(i) {}
  array1* array;
  uint i;
};
