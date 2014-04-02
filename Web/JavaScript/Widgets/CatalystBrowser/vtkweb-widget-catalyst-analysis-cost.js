(function ($, GLOBAL) {
    var TYPE_CONVERTER = {
        "catalyst-viewer": "vtk-icon-loop-alt",
        "catalyst-resample-viewer" : "vtk-icon-chart-line",
        "composite-image-stack" : "vtk-icon-list-add",
        "catalyst-pvweb" : "vtk-icon-laptop"
    };

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

    function buildBillingPage(info, path, formula) {
        var content = [ "<table class='catalyst-bill'><tr class='head'></td><td class='empty title'></td><td><span class='vtk-icon-resize-horizontal-1'/></td><td><span class='vtk-icon-clock'/></td><td><span class='vtk-icon-database'/></td><td><span class='vtk-icon-picture-1'/></td><td><span class='vtk-icon-dollar'/></td></tr>" ],
        total = { "space": 0, "images": 0, "time": 0 , "dollars": 0},
        analysisCount = info['analysis'].length;

        // Add each analysis
        while(analysisCount--) {
            var item = info['analysis'][analysisCount],
            cost = item['cost'],
            dollars = formula(cost);

            total['space'] += cost['space'];
            total['images'] += cost['images'];
            total['time'] += cost['time'];
            total['dollars'] += dollars;

            content.push(buildBillEntry(item, cost, dollars));
        }

        // Add total
        content.push("<tr class='sum'><td>Total</td><td></td><td>"+ formatTime(total["time"]) +"</td><td>"+ formatSpace(total["space"]) +"</td><td>"+ total["images"] +"</td><td>"+ formatDollars(total["dollars"]) +"</td></tr></table>")

        return "<div class='view cost'>" + content.join('') + "</div>";
    }

    // ------------------------------------------------------------------------

    function buildBillEntry(item, cost, dollars) {
        var classType = TYPE_CONVERTER[item["type"]],
        title = item["title"],
        time = cost["time"],
        space = cost["space"],
        images = cost["images"],
        width = cost.hasOwnProperty('image-width') ? cost["image-width"] : "";

        return "<tr><td class='title'><span class='" + classType + "'/>" + title + "</td><td class='image-width value'>"+width+"</td><td class='time value'>" + formatTime(time) + "</td><td class='space value'>" + formatSpace(space) + "</td><td class='images value'>" + images + "</td><td class='dollars value'>" + formatDollars(dollars) + "</td></tr>";
    }

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystViewer
     * @param project
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystAnalysisCost = function(project, dataBasePath) {
        return this.each(function() {
            var me = $(this).unbind().empty().html(buildBillingPage(project, dataBasePath, formula));
        });
    }

}(jQuery, window));