const defaultOpts = {
    nx: '2',
    ny: '2',
    nz: '2',
    mapperIsStatic: '1'
};

let searchParams = new URLSearchParams(window.location.search);
export const options = {
    nx: searchParams.get('nx') || defaultOpts.nx,
    ny: searchParams.get('ny') || defaultOpts.ny,
    nz: searchParams.get('nz') || defaultOpts.nz,
    mapperIsStatic: searchParams.get('mapperIsStatic') || defaultOpts.mapperIsStatic
}
