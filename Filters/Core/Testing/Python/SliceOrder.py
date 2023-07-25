#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkCommonTransforms import vtkTransform

class SliceOrder(object):
    '''
        These transformations permute medical image data to maintain proper
        orientation regardless of the acquisition order.
        After applying these transforms with vtkTransformFilter,
        a view up of 0,-1,0 will result in the body part
        facing the viewer.
        NOTE: some transformations have a -1 scale factor
              for one of the components.
              To ensure proper polygon orientation and normal direction,
              you must apply the vtkPolyDataNormals filter.

        Naming:
        si - superior to inferior (top to bottom)
        iss - inferior to superior (bottom to top)
        ap - anterior to posterior (front to back)
        pa - posterior to anterior (back to front)
        lr - left to right
        rl - right to left

    '''

    si = vtkTransform()
    si.SetMatrix([1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1])

    # 'is' is a reserved word in Python so use 'iss'
    iss = vtkTransform()
    iss.SetMatrix([1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 1])

    ap = vtkTransform()
    ap.Scale(1, -1, 1)

    pa = vtkTransform()
    pa.Scale(1, -1, -1)

    lr = vtkTransform()
    lr.SetMatrix([0, 0, -1, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1])

    rl = vtkTransform()
    rl.SetMatrix([0, 0, 1, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1])

    #
    # the previous transforms assume radiological views of the slices
    # (viewed from the feet).
    #  Othermodalities such as physical sectioning may view from the head.
    # These transforms modify the original with a 180 rotation about y
    #
    hf = vtkTransform()
    hf.SetMatrix([-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1])

    hfsi = vtkTransform()
    hfsi.Concatenate(hf.GetMatrix())
    hfsi.Concatenate(si.GetMatrix())

    hfis = vtkTransform()
    hfis.Concatenate(hf.GetMatrix())
    hfis.Concatenate(iss.GetMatrix())

    hfap = vtkTransform()
    hfap.Concatenate(hf.GetMatrix())
    hfap.Concatenate(ap.GetMatrix())

    hfpa = vtkTransform()
    hfpa.Concatenate(hf.GetMatrix())
    hfpa.Concatenate(pa.GetMatrix())

    hflr = vtkTransform()
    hflr.Concatenate(hf.GetMatrix())
    hflr.Concatenate(lr.GetMatrix())

    hfrl = vtkTransform()
    hfrl.Concatenate(hf.GetMatrix())
    hfrl.Concatenate(rl.GetMatrix())
