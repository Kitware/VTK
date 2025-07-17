/**
 * Binary search for a max value without knowing the exact value, only that it
 * can be under or over It dose not test every number but instead looks for
 * 1,2,4,8,16,32,64,128,96,95 to figure out that you thought about #96 from
 * 0-infinity
 *
 * @example findFirstPositive(x => matchMedia(`(max-resolution: ${x}dpi)`).matches)
 * @author Jimmy Wärting
 * @see {@link https://stackoverflow.com/a/72124984/1008999}
 * @param {function} f       The function to run the test on (should return truthy or falsy values)
 * @param {bigint} [b=1]  Where to start looking from
 * @param {function} d privately used to calculate the next value to test
 * @returns {bigint} Integer
 */
function findFirstPositive(f, b = 1n, d = (e, g, c) => g < e ? -1 : 0 < f(c = e + g >> 1n) ? c == e || 0 >= f(c - 1n) ? c : d(e, c - 1n) : d(c + 1n, g)) {
  for (; 0 >= f(b); b <<= 1n); return d(b >> 1n, b) - 1n;
}

const tries = [];
export const maxSize = findFirstPositive(x => {
  tries.push(Number(x).toLocaleString())
  try { new ArrayBuffer(Number(x)); return false } catch { return true }
});
