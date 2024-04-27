"""Utility classes to help with the simpler Python interface
for connecting and executing pipelines."""

__all__ = ['select_ports', 'Pipeline', 'Output']

def _call(first, last, inp=None, port=0):
    """Set the input of the first filter, update the pipeline
    and return the output."""
    if inp and not first.GetNumberOfInputPorts():
        raise ValueError(f"{first.GetClassName()} does not have input ports yet an input was passed to the pipeline.")
    in_cons = []
    if first.GetNumberOfInputPorts():
        n_cons = first.GetNumberOfInputConnections(port)
        for i in range(n_cons):
            op = first.GetInputConnection(port, i)
            if op and op.GetProducer():
                op.GetProducer().Register(None)
            in_cons.append(op)
        from vtkmodules.vtkCommonExecutionModel import vtkAlgorithm
        from vtkmodules.vtkCommonExecutionModel import vtkTrivialProducer
        from collections.abc import Sequence
        if isinstance(inp, Sequence):
            if first.GetInputPortInformation(port).Has(
                    vtkAlgorithm.INPUT_IS_REPEATABLE()):
                first.RemoveAllInputConnections(port)
                for aInp in inp:
                    tp = vtkTrivialProducer()
                    tp.SetOutput(aInp)
                    first.AddInputConnection(port, tp.GetOutputPort());
        else:
            tp = vtkTrivialProducer()
            tp.SetOutput(inp)
            first.SetInputConnection(port, tp.GetOutputPort());

    output = last.update().output

    if first.GetNumberOfInputPorts():
        first.RemoveAllInputConnections(port)
        for op in in_cons:
            first.AddInputConnection(port, op)
            if op and op.GetProducer():
                op.GetProducer().UnRegister(None)

    output_copy = []
    if type(output) is not tuple:
        output = (output,)
    for opt in output:
        copy = opt.NewInstance()
        copy.ShallowCopy(opt)
        output_copy.append(copy)
    if len(output_copy) == 1:
        return output_copy[0]
    else:
        return tuple(output_copy)


class select_ports(object):
    """Helper class for selecting input and output ports when
    connecting pipeline objects with the >> operator.
    Example uses:
    # Connect a source to the second input of a filter.
    source >> select_ports(1, filter)
    # Connect the second output of a source to a filter.
    select_ports(source, 1) >> filter
    # Combination of both: Connect source to second
    # input of the filter, then connect the second
    # output of that filter to another one.
    source >>> select_ports(1, filter, 1) >> filter2
    """
    def __init__(self, *args):
        """This constructor takes 2 or 3 arguments.
        The possibilities are:
        select_ports(input_port, algorithm)
        select_ports(algorithm, output_port)
        select_ports(input_port, algorithm, output_port)
        """
        nargs = len(args)
        if nargs < 2 or nargs > 3:
            raise ValueError("Expecting 2 or 3 arguments")

        self.input_port = None
        self.output_port = None
        before_alg = True
        for arg in args:
            if hasattr(arg, "IsA") and arg.IsA("vtkAlgorithm"):
                self.algorithm = arg
                before_alg = False
            else:
                if before_alg:
                    self.input_port = arg
                else:
                    self.output_port = arg
        if not self.input_port:
            self.input_port = 0
        if not self.output_port:
            self.output_port = 0

    def SetInputConnection(self, inp):
        "Forwards to underlying algorithm and port."
        self.algorithm.SetInputConnection(self.input_port, inp)

    def AddInputConnection(self, inp):
        "Forwards to underlying algorithm and port."
        self.algorithm.AddInputConnection(self.input_port, inp)

    def GetOutputPort(self):
        "Returns the output port of the underlying algorithm."
        return self.algorithm.GetOutputPort(self.output_port)

    def GetInputPortInformation(self, port):
        return self.algorithm.GetInputPortInformation(self.input_port)

    def update(self):
        """Execute the algorithm and return the output from the selected
        output port."""
        return self.algorithm.update()

    def __rshift__(self, rhs):
        "Creates a pipeline between the underlying port and an algorithm."
        return Pipeline(self, rhs)

    def __rrshift__(self, lhs):
        """Creates a pipeline between the underlying port and an algorithm.
        This is to handle sequence >> select_ports where the port can
        accept multiple connections."""
        from collections.abc import Sequence
        if lhs is None or (isinstance(lhs, Sequence) and len(lhs == 0)):
            self.algorithm.RemoveAllInputConnections(self.input_port)
            return self
        return Pipeline(lhs, self)

    def __call__(self, inp=None):
        """Executes the underlying algorithm by passing input data to
        the selected input port. Returns a single output or a tuple
        if there are multiple outputs."""
        return _call(self.algorithm, self.algorithm, inp, self.input_port)

class Pipeline(object):
    """Pipeline objects are created when 2 or more algorithms are
    connected with the >> operator. They store the first and last
    algorithms in the pipeline and enable connecting more algorithms
    and executing the pipeline. One should not have to create Pipeline
    objects directly. They are created by the use of the >> operator."""

    PIPELINE = 0
    ALGORITHM = 1
    DATA = 2
    UNKNOWN = 3

    def __init__(self, lhs, rhs):
        """Create a pipeline object that connects two objects of the
        following type: data object, pipeline object, algorithm object."""
        left_type = self._determine_type(lhs)
        right_type = self._determine_type(rhs)
        if right_type == Pipeline.ALGORITHM:
            rhs_alg = rhs
        elif right_type == Pipeline.PIPELINE:
            rhs_alg = rhs.first
        else:
            raise TypeError(
              f"unsupported operand type(s) for >>: {type(lhs).__name__} and {type(rhs).__name__}")

        from collections.abc import Sequence
        if isinstance(lhs, Sequence):
            for inp in lhs:
                self._connect(inp, rhs, rhs_alg, "AddInputConnection")
        else:
            self._connect(lhs, rhs, rhs_alg, "SetInputConnection")

    def _connect(self, lhs, rhs, rhs_alg, connect_method):
        from vtkmodules.vtkCommonExecutionModel import vtkAlgorithm
        inInfo = rhs_alg.GetInputPortInformation(0)
        if inInfo.Has(vtkAlgorithm.INPUT_IS_REPEATABLE()):
            connect_method = 'AddInputConnection'

        left_type = self._determine_type(lhs)
        right_type = self._determine_type(rhs)
        if left_type == Pipeline.UNKNOWN:
            raise TypeError(
              f"unsupported operand type(s) for >>: {type(lhs).__name__} and {type(rhs).__name__}")
        if right_type == Pipeline.ALGORITHM:
            if left_type == Pipeline.ALGORITHM:
                getattr(rhs_alg, connect_method)(lhs.GetOutputPort())
                self.first = lhs
                self.last = rhs
            elif left_type == Pipeline.PIPELINE:
                getattr(rhs_alg, connect_method)(lhs.last.GetOutputPort())
                self.first = lhs.first
                self.last = rhs
            elif left_type == Pipeline.DATA:
                from vtkmodules.vtkCommonExecutionModel import vtkTrivialProducer
                source = vtkTrivialProducer()
                source.SetOutput(lhs)
                getattr(rhs_alg, connect_method)(source.GetOutputPort())
                self.first = source
                self.last = rhs
        elif right_type == Pipeline.PIPELINE:
            if left_type == Pipeline.ALGORITHM:
                self.first = lhs
                self.last = rhs.last
                getattr(rhs_alg, connect_method)(lhs.GetOutputPort())
            elif left_type == Pipeline.PIPELINE:
                getattr(rhs_alg, connect_method)(lhs.last.GetOutputPort())
                self.first = lhs.first
                self.last = rhs.last
            elif left_type == Pipeline.DATA:
                from vtkmodules.vtkCommonExecutionModel import vtkTrivialProducer
                source = vtkTrivialProducer()
                source.SetOutput(lhs)
                getattr(rhs_alg, connect_method)(source.GetOutputPort())
                self.first = source
                self.last = rhs.last

    def _determine_type(self, arg):
        if type(arg) is Pipeline:
            return Pipeline.PIPELINE
        if hasattr(arg, "SetInputConnection"):
            return Pipeline.ALGORITHM
        if hasattr(arg, "IsA") and arg.IsA("vtkDataObject"):
            return Pipeline.DATA
        return Pipeline.UNKNOWN

    def update(self, **kwargs):
        """Update the pipeline and return the last algorithm's
        output."""
        return self.last.update()

    def __call__(self, inp=None):
        """Sets the input of the first filter, update the pipeline
        and returns the output. A single data object or a tuple
        of data objects (when there are multiple outputs) are
        returned."""
        return _call(self.first, self.last, inp)

    def __rshift__(self, rhs):
        """Used to connect two pipeline items. The left side can
        be a data object, an algorithm or a pipeline. The right
        side can be an algorithm or a pipeline."""
        return Pipeline(self, rhs)

    def __rrshift__(self, lhs):
        """Creates a pipeline between a sequence input and a pipeline."""
        from collections.abc import Sequence
        if lhs is None or (isinstance(lhs, Sequence) and len(lhs) == 0):
            self.first.RemoveAllInputConnections(0)
            return self
        return Pipeline(lhs, self)

class Output(object):
    """Helper object to represent the output of an algorithms as
    returned by the update() method. Implements the output property
    enabling calling update().output."""
    def __init__(self, algorithm, **kwargs):
        self.algorithm = algorithm
        self.algorithm.Update()

    @property
    def output(self):
        """Returns a single data object or a tuple of data objects
        if there are multiple outputs."""
        if self.algorithm.GetNumberOfOutputPorts() == 1:
            return self.algorithm.GetOutputDataObject(0)
        else:
            outputs = []
            nOutputs = self.algorithm.GetNumberOfOutputPorts()
            for i in range(nOutputs):
                outputs.append(self.algorithm.GetOutputDataObject(i))
        return tuple(outputs)
