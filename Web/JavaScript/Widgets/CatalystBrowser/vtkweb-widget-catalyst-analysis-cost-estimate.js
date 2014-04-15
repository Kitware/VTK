(function ($, GLOBAL) {
    var EXPLORATION_TEMPLATE = "<div class='input exploration-config'><div class='title'>Exploration settings</div><ul><li><div class='label'>Exploration type</div><select><option>Composite</option><option>Image base</option></select></li><li class='for-composite'><div class='label'>Number of geometries<span class='value'></span></div><input class='slider nb-geo' type='range' value='11' min='1' max='62'/></li><li class='for-composite'><div class='label'>Number of captures <span class='value'></span></div><input class='slider nb-all' type='range' value='21' min='1' max='128'/></li><li><div class='label'>Parameter range</div><input name='params' type='text' value='1,1'/></li></ul></div>",
    CAMERA_TEMPLATE = "<div class='input right camera-handler'><div class='title'>Camera settings</div><ul><li><div class='label'>Camera manager</div><select><option value='360'>360+</option><option value='w'>Wobble</option><option value='f'>Fix</option></select></li><li class='360 w'><div class='label'>Sampling Phi angle <span class='value'></span></div><input class='slider' type='range' min='5' max='180' value='18' name='phi'/></li><li class='360'><div class='label'>Sampling Theta angle <span class='value'></span></div><input class='slider' type='range' min='5' value='30' max='85' name='theta'/></li><li><div class='label'>Total number of viewpoint</div><span class='nb-view-points'></span></li></ul></div>",
    IMAGE_TEMPLATE = "<div class='input image-config'><div class='title'>Image settings</div><ul><li><div class='label'>Image type</div><select><option>JPG</option><option>PNG</option><option>TIFF</option></select></li><li><div class='label'>Image resolution</div><input type='text' value='500' name='width' class='half'/> x <input type='text' value='500' name='height' class='half'/></li></ul></div>",
    RESULT_TEMPLATE = "<div class='output estimate-result'><div class='title'>Cost estimate</div><table><tr><td>Average render time for the scene</td><td> : <input type='text' name='avg-render-time' value='200'/> ms</td></tr><tr><td>Total number of images</td><td> : <span class='total-nb-images'></span></td></tr><tr><td>Estimate image size</td><td> : <span class='image-size'></span></td></tr><tr><td>Total data size</td><td> : <span class='total-disk-usage'></span></td></tr><tr><td>Estimated time cost</td><td> : <span class='time-cost'></span></td></tr></table></div>",
    PAGE_CONTENT = [CAMERA_TEMPLATE, EXPLORATION_TEMPLATE, IMAGE_TEMPLATE, RESULT_TEMPLATE],
    MagicNumbers = {
        "PNG": {
            'space': function(nbPixels) { return nbPixels * 0.6; },
            'time' : function(nbPixels) { return nbPixels * 0.000000204032; }
        },
        "JPG": {
            'space': function(nbPixels) { return nbPixels * 0.24; }, // Max noticed and PNG is usally 2.5 bigger
            'time' : function(nbPixels) { return nbPixels * 0.000000131541818181818; }
        },
        "TIFF": {
            'space': function(nbPixels) { return nbPixels * 3.028; },
            'time' : function(nbPixels) {
                var imageSize = nbPixels * 3.028,
                bufferDisk = 16000000;
                return (imageSize < bufferDisk) ? 0.0000000241866666666667 * nbPixels : 0.000000102088 * nbPixels;
            }
        },
        "COMPOSITE": {
            'space': function(nbPixels) { return nbPixels * 0.448; },
            'time' : function(nbPixels) { return nbPixels * 0.000000642785454545454;}
        },
        "RGB_CAPTURE": {
            'space': function(nbPixels) { return 0; },
            'time' : function(nbPixels) { return nbPixels * 0.00000898981818181818;}
        }
    };

    // ========================================================================

    function getNbPixels(container) {
        var width = Number($('input[name="width"]', container).val()),
        height = Number($('input[name="height"]', container).val()),
        nbObjects = ($('.exploration-config select', container).val() == "Composite") ?  (1+Number($('.exploration-config .slider.nb-all', container).val())) : 1;

        return (width*height*nbObjects);
    }

    // ========================================================================

    function getNbZPixels(container) {
        var width = Number($('input[name="width"]', container).val()),
        height = Number($('input[name="height"]', container).val()),
        format = $('.image-config select', container).val(),
        nbObjects = ($('.exploration-config select', container).val() == "Composite") ?  (1+Number($('.exploration-config .slider.nb-geo', container).val())) : 1;

        return (width*height*nbObjects);
    }

    // ========================================================================

    function getImageSize(container) {
        var format = $('.image-config select', container).val();
        return MagicNumbers[format]['space'](getNbPixels(container));
    }

    // ========================================================================

    function getImageTime(container) {
        var format = $('.image-config select', container).val();
        return MagicNumbers[format]['time'](getNbPixels(container));
    }

    // ========================================================================

    function getCompositeSize(container) {
        return MagicNumbers["COMPOSITE"]['space'](getNbZPixels(container));
    }

    // ========================================================================

    function getCompositeTime(container) {
        return MagicNumbers["COMPOSITE"]['time'](getNbZPixels(container));
    }

    // ========================================================================

    function formula(cost) {
        var dollarsAmount = 0;
        if(cost) {
            if(cost["time"]) {
                dollarsAmount += 0.001 * cost["time"];
            }
            if(cost["space"]) {
                dollarsAmount += 0.000000002 * cost["space"];
            }
            if(cost["images"]) {
                dollarsAmount += 0.001 * cost["images"];
            }
        }
        return dollarsAmount;
    }

    // ========================================================================

    function formatTime(t) {
        var seconds = Number(t),
        minutes = Math.floor(seconds / 60),
        hours = Math.floor(minutes / 60),
        buffer = [];

        seconds %= 60;
        seconds = Math.floor(seconds);
        minutes %= 60;
        minutes = Math.floor(minutes);

        if(hours > 0) {
           buffer.push(hours);
        }
        if(minutes > 0 || hours > 0) {
           buffer.push(("00" + minutes).slice (-2));
        }
        if(seconds > 0 || minutes > 0 || hours > 0) {
            buffer.push(("00" + seconds).slice (-2));
        }

        return buffer.join(':');
    }

    // ========================================================================

    function formatSpace(t) {
        var space = Number(t), unit = [ ' B', ' K', ' M', ' G', ' T'], currentUnit = 0;
        while(space > 1000) {
            space /= 1000;
            currentUnit++;
        }
        return space.toFixed(2) + unit[currentUnit];
    }

    // ========================================================================

    function formatDollars(v) {
        x = v.toFixed(2).toString();
        var pattern = /(-?\d+)(\d{3})/;
        while (pattern.test(x)) {
            x = x.replace(pattern, "$1,$2");
        }
        return x;
    }

    // ------------------------------------------------------------------------

    function updateNumberOfImages(container) {
        var nbImagesContainer = $('.total-nb-images', container),
        parameters = $('.exploration-config input[name="params"]', container).val().split(','),
        nbViews = Number($('.nb-view-points', container).attr('data-value')),
        nbParams = 1;

        for(var idx in parameters) {
           nbParams *=  Number(parameters[idx]);
        }

        nbImagesContainer.html(nbParams*nbViews).attr('data-value', nbParams*nbViews);
        updateTotalEstimate(container);
    }

    // ------------------------------------------------------------------------

    function updateImageSize(container) {
        $('.output .image-size', container).html(formatSpace(getImageSize(container)));
        updateTotalEstimate(container);
    }

    // ------------------------------------------------------------------------

    function updateCameraEstimate(container) {
        var phiContainer = $('.w.360 span.value', container),
        phi = Number($('input[name="phi"]', container).val()),
        theta = Number($('input[name="theta"]', container).val()),
        resultContainer = $('.nb-view-points', container),
        cameraType = $('.camera-handler select', container).val();

        $('.360,.w', container).hide();

        if (cameraType == 'f') {
            resultContainer.html("1").attr('data-value', 1);
        } else if (cameraType == '360') {
            $('.360', container).show();
            var a = Math.floor(360/phi), b = (1 + 2*Math.floor(89/theta));
            resultContainer.html(a + " x " + b + " = " + (a*b)).attr('data-value', (a*b));
            if(360%phi === 0) {
                phiContainer.css('color', 'black');
            } else {
                phiContainer.css('color', 'red');
            }
        } else if (cameraType == 'w') {
            $('.w', container).show();
            resultContainer.html("9").attr('data-value', 9);
        }

        updateNumberOfImages(container);
    }

    // ------------------------------------------------------------------------

    function updateTotalEstimate(container) {
        var totalSizeContainer = $('.total-disk-usage', container),
        totalTimeContainer = $('.time-cost', container),
        nbImages = Number( $('.total-nb-images', container).attr('data-value')),
        dataSizePerImage = getImageSize(container),
        timePerImage = getImageTime(container),
        nbRender = nbImages,
        renderTime = Number($('.output input', container).val()) / 1000,
        rgbCaptureTime = MagicNumbers["RGB_CAPTURE"]['time'](getNbPixels(container));

        if ($('.exploration-config select', container).val() == "Composite") {
            dataSizePerImage += getCompositeSize(container);
            timePerImage += getCompositeTime(container);
            nbRender *= 1 + Number($('.exploration-config .slider.nb-all', container).val());
        }

        // console.log('nb pix: ' + getNbPixels(container));
        // console.log('image size: ' + getImageSize(container));
        // console.log('composite size: ' + getCompositeSize(container));
        // console.log('composite time: ' + getCompositeTime(container));

        totalSizeContainer.html(formatSpace(dataSizePerImage*nbImages));
        totalTimeContainer.html(formatTime(timePerImage*nbImages + (renderTime*nbRender) + rgbCaptureTime));
    }

    // ------------------------------------------------------------------------

    function initializeListeners(container) {
        $('.slider', container).bind('keyup change',function(){
            var me = $(this);
            $('.value', me.parent()).html(me.val());
            updateCameraEstimate(container);
        }).trigger('change');

        $('.camera-handler select', container).change(function(){
            updateCameraEstimate(container);
        }).trigger('change');

        $('.exploration-config input[name="params"]', container).change(function(){
            updateNumberOfImages(container);
        });

        $('.exploration-config select', container).change(function(){
            var me = $(this),
            isComposite = (me.val() == "Composite");

            if(isComposite) {
                $('.for-composite', container).show();
            } else {
                $('.for-composite', container).hide();
            }

            updateImageSize(container);
        });

        $('.image-config select, .exploration-config .slider, .image-config input').bind('keyup change', function(){
            updateImageSize(container);
        }).trigger('change');

        $('.output input').change(function(){
            updateTotalEstimate(container);
        });
    }

    // ------------------------------------------------------------------------

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystViewer
     * @param project
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystAnalysisCostEstimate = function() {
        return this.each(function() {
            var me = $(this).unbind().empty().addClass('cost-estimate').html(PAGE_CONTENT.join(''));

            initializeListeners(me);
        });
    }

}(jQuery, window));