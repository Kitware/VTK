/**
 * Break a Blob into individual blobs of `chunkSize` number of bytes.
 * @param {Blob} blob - The Blob to be chunked.
 * @param {number} chunkSize - The size of each chunk in bytes.
 * @returns {Blob[]} An array of Blob chunks.
 */
export function chunkify(blob, chunkSize) {
  const numChunks = Math.ceil(blob.size / chunkSize);
  let i = 0;
  const chunks = [];
  while (i < numChunks) {
    const offset = (i++) * chunkSize;
    chunks.push(blob.slice(offset, offset + chunkSize));
  }
  return chunks;
}
