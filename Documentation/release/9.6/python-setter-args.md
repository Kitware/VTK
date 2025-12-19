## Python object property setter argument types

Previously, the setter method that was called when setting an object property
in Python would depend on whether it was set with a tuple vs. a list (or some
other sequence type):

    sphere.center = (x,y,z) # called sphere.SetCenter(x,y,z)
    sphere.center = [x,y,z] # called sphere.SetCenter([x,y,z])

Now, the call is the same regardless of whether a tuple or a list is used.
If the setter method takes multiple arguments, then the multi-argument
call is always used:

    sphere.center = (x,y,z) # calls sphere.SetCenter(x,y,z)
    sphere.center = [x,y,z] # calls sphere.SetCenter(x,y,z)

If there is no multi-argument setter, then the value is always passed as-is:

    sphere.center = (x,y,z) # calls sphere.SetCenter((x,y,z))
    sphere.center = [x,y,z] # calls sphere.SetCenter([x,y,z])

The new implementation also fixes an error where setting certain properties
with a tuple would fail if there was no multi-argument setter method:

    cell = vtkPolyhedron()
    # this failed because there is no SetFaces(a,b,c,d) method
    cell.faces = (0,1,2,3)
    # now it succeeds because SetFaces((0,1,2,3)) is called instead
