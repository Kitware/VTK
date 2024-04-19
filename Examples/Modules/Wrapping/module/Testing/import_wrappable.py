from wrapping import vtkWrappable

wrap = vtkWrappable.vtkWrapped()
assert wrap.GetString() == 'wrapped'
