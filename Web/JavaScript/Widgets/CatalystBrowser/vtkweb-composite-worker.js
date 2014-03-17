var layerVisibility = {},
orderMappping = {},
layerCount = {},
requestMap = {
    '_': updateVisibility,
    'w': processComposites,
    'd': registerData,
    's': updatePixelCount
},
compositeData = {},
needToUpdateOrderTable = true,
missingOrderKeys = true,
nbPixels = 100;

// ------------------------------------------------------------------

function updatePixelCount(nbPixelsSTR) {
    nbPixels = Number(nbPixelsSTR);
}

// ------------------------------------------------------------------

function updateVisibility(query) {
    var count = query.length;
    for(var idx = 0; idx < count; idx += 2) {
        layerVisibility[query[idx]] = (query[idx+1] != '_');
        layerCount[query[idx]] = 0;
    }
    needToUpdateOrderTable = true;
}

// ------------------------------------------------------------------

function getLayer(order) {
    var count = order.length;
    for(var i = 0; i < count; ++i) {
        if(layerVisibility[order[i]]) {
            return order[i];
        }
    }
    return '+';
}

// ------------------------------------------------------------------

function registerData(dataStr) {
    var id_fields_data = dataStr.split('|');
    compositeData[id_fields_data[0]] = { fields: JSON.parse(id_fields_data[1]), order: JSON.parse(id_fields_data[2]) };
    missingOrderKeys = true;
}

// ------------------------------------------------------------------

function updateOrderKeys() {
    if(missingOrderKeys) {
        missingOrderKeys = false;
        needToUpdateOrderTable = true;

        for(var id in compositeData) {
            var composite = compositeData[id].order;
            for(var order in composite) {
                orderMappping[order] = '+';
            }
        }
    }
}

// ------------------------------------------------------------------

function computeOrderMap() {
    if(needToUpdateOrderTable) {
        needToUpdateOrderTable = false;
        for(var order in orderMappping) {
            orderMappping[order] = getLayer(order);
        }
    }
}

// ------------------------------------------------------------------

function resetLayerCount() {
    for(var key in layerCount) {
        layerCount[key] = 0;
    }
    return layerCount;
}

// ------------------------------------------------------------------

function processComposites() {
    updateOrderKeys();
    computeOrderMap();

    for(var id in compositeData) {
        var composite = compositeData[id].order,
        layerCount = resetLayerCount();

        for(var order in composite) {
            layerCount[orderMappping[order]] += composite[order];
        }

        // Compute percentage
        var ratio = 100 / nbPixels;
        for(var layer in layerCount) {
            layerCount[layer] *= ratio;
        }

        postMessage({id:id, count:layerCount, fields: compositeData[id].fields});
    }
}

// ------------------------------------------------------------------

onmessage = function (event) {
    requestMap[event.data[0]](event.data.substr(1));
}