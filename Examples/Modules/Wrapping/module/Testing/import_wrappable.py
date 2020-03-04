from wrapping import vtkWrappable

wrap = vtkWrappable.vtkWrappable()
assert wrap.GetString() == 'wrapped'
