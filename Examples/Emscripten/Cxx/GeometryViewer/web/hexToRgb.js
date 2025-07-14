/**
 * Converts a hexadecimal color value to an RGB array.
 * @param {number} hex - The hexadecimal color value (e.g., 0xffaabb).
 * @returns {number[]} An array containing the red, green, and blue components.
 */
export function hexToRgb(hex) {
  const r = (hex >> 16) & 255;
  const g = (hex >> 8) & 255;
  const b = hex & 255;

  return [r, g, b];
}
