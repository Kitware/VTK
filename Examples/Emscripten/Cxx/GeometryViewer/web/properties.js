function isNil(value) {
    return value === undefined || value === null;
}

export function geometryViewerPropertiesFromURL(url, initialProperties) {
    const result = { ...initialProperties };
    if (isNil(url)) {
        return result;
    }
    const paramObject = url instanceof URL
        ? Object.fromEntries(url.searchParams)
        : {};

    // String properties
    ['viewApi', 'url', 'colorByArray', 'colorMapPreset'].forEach(key => {
        if (!isNil(paramObject[key])) result[key] = paramObject[key].toString();
    });

    // Number properties
    [
        'mouseWheelMotionFactor', 'representation', 'pointSize', 'lineWidth',
        'solidColor', 'vertexColor', 'edgeColor', 'opacity', 'edgeOpacity',
        'backgroundColor1', 'backgroundColor2', 'highlightOnHover'
    ].forEach(key => {
        if (!isNil(paramObject[key])) result[key] = Number(paramObject[key]);
    });

    // Boolean properties
    [
        'interpolateScalarsBeforeMapping', 'showControls', 'vertexVisibility',
        'edgeVisibility', 'ditherGradient', 'orthographic',
        'renderPointsAsSpheres', 'renderLinesAsTubes'
    ].forEach(key => {
        if (!isNil(paramObject[key])) result[key] = paramObject[key].toString() === "true";
    });

    return result;
}
