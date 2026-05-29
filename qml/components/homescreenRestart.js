.pragma library

function restartWithRemorse(remorsePopup, themePack, remorseText) {
    remorsePopup.execute(remorseText, function() {
        themePack.restartHomescreen()
    })
}
