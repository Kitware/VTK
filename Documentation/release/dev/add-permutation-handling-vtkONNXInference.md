## Simplify memory layout mapping between VTK and ONNX models

VTK now provides an API to handle memory layout mapping between VTK arrays and the input required by ONNX models. The mapping operation is designed as a permutation system. A valid permutation has the same number of elements as the model shape, and is composed of integers from 0 to n-1.

### Examples

The shape (a, b, c, d) can be mapped to (c, a, b, d) with permutation (2, 0, 1, 3).

---
VTK AoS array: t tuples, c components<br>
Expected ONNX shape: (c, t)<br>
Here, we need to transpose the array with the permutation (1, 0).
