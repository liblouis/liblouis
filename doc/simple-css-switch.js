/* taken from https://magit.vc/assets/js/simple-css-switch.js */


function simpleCssSwitch() {
    var stylesMenuContainer = document.getElementById('s-css-s--menu');
    var styleButtons = [];
    var stylesAvailable = Array.prototype.slice.call(
        document.querySelectorAll('.s-css-s--style')
    );
    stylesAvailable.forEach(function(style, idx) {
        var cssFileAsClass = 's-css-s-' + style.href.split('/').pop().replace('.', '-');
        var styleOptionButton = document.createElement('button');
        styleOptionButton.setAttribute('title', style.title);
        styleOptionButton.classList.add('s-css-s-button', cssFileAsClass);
        styleOptionButton.innerText = idx;
        styleOptionButton.addEventListener(
            'click', function(e) {
                chooseStyle(style.title);
                markActiveButton(e.target);
            });
        styleButtons.push(styleOptionButton);
        stylesMenuContainer.appendChild(styleOptionButton);
    });

    var activeStyle = localStorage.getItem('s-css-s--active-style');
    if (activeStyle) {
        styleButtons.forEach(function(btn){
            if (btn.title === activeStyle) {
                btn.click();
            }
        });
    }

    function markActiveButton(clicked) {
        styleButtons.forEach(function(btn){
            btn.classList.remove('s-css-s--active-style');
        });
        clicked.classList.add('s-css-s--active-style');
    }

    function chooseStyle(styleTitle) {
        stylesAvailable.forEach(function(style){
            style.disabled = true;
            if (style.title === styleTitle) {
                style.disabled = false;
                localStorage.setItem('s-css-s--active-style', styleTitle);
            }
        });
    }

}
