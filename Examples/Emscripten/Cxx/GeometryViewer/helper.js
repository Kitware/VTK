(function () {
    var GeometryViewerHelper = {
        sendMessageToTooltipAt__sig: 'vppii',
        sendMessageToTooltipAt: function (text, length, x, y) {
            let el = document.getElementsByClassName("tooltip").item(0);
            el.textContent = Module.UTF8ToString(text, length);
            el.style.left = `${x / window.devicePixelRatio}px`;
            el.style.top = `${window.innerHeight -
                y / window.devicePixelRatio}px`;
            el.style.visibility = 'visible';
        },
    };
    mergeInto(LibraryManager.library, GeometryViewerHelper);
})();
