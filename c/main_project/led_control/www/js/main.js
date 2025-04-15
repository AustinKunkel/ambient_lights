led_settings = {
  'brightness' : 100,
  'color' : "#FFFFFF",
  'capture_screen' : 0,
  'sound_react' : 0,
  'fx_num' : 0,
  'count' : 206,
  'id' : 2
}

function sendGetLedSettings() {
  sendLedSettingsGet().then((data) => {
    console.log(data);
  })
}

document.addEventListener("DOMContentLoaded", async function() {

  requestGetLedSettings().then( data => {
    updateLedSettings(data)
  })

  requestGetCaptSettings().then (data => {
    updateCaptSettings(data)
  })

  const brightnessInput = document.getElementById("brightness-input")
  brightnessInput.addEventListener("input", () => {
      led_settings.brightness = Number(brightnessInput.value)
      sendLedSettingsPost(led_settings).then(()=> {
        // request get settings and update led_settings
      });
  })

  const countInput = document.getElementById("count-input")
  countInput.addEventListener("keydown", function(event) {
    if (event.key === "Enter") {
      led_settings.count = Number(countInput.value);
      sendLedSettingsPost(led_settings).then(() => {
        // request get settings and update led_settings
      });
    }
  })

  const vOffsetInput = document.getElementById("v-offset-input")
  const hOffsetInput = document.getElementById("h-offset-input")

  const avgColorInput = document.getElementById("avg-scrn-color-check")
  const soundReact = document.getElementById("snd-rct-check")
  const blendModeActive = document.getElementById('blend-mode-check');
  const captureButton = document.getElementById('capture_button');
  const blendDepthInput = document.getElementById('blend-depth-input');

  function updateLedSettings(data) {
    brightnessInput.value = Number(data.brightness)
    countInput.value = Number(data.count)
    soundReact.checked = data.sound_react >= 1;
    showScrnAndSndReactOptions()
    isCapturing = data.capture_screen >= 1;
    updateCaptureButton(isCapturing)
  }

  const leftCount = document.getElementById("left-led-count")
  const rightCount = document.getElementById("right-led-count")
  const topCount = document.getElementById("top-led-count")
  const bottomCount = document.getElementById("bottom-led-count")

  let customResXInput = document.getElementById('custom-res-width');
  let customResYInput = document.getElementById('custom-res-height');
  let resX = 0;
  let resY = 0;
  let isUsingCustomRes = false;

  window.handleDefaultResolutionChange = function(selected) {
    selectedValue = selected.value;
    full = selectedValue.split('x');

    resX = full[0];
    resY = full[1];

    isUsingCustomRes = false;
  }

  window.handleCustomResolutionChange = function() {
    isUsingCustomRes = true;
  }

  window.saveCaptSettings = function() {
    const left = leftCount.value;
    const right = rightCount.value;
    const top = topCount.value;
    const bottom = bottomCount.value;
    const isAvgColor = getAvgColor();
    const blendMode = getBlendMode();
    const isSoundWithScreen = soundReact.checked ? 1 : 0;
    const hOffset = hOffsetInput.value;
    const vOffset = vOffsetInput.value;
    const blendDepth = blendDepthInput.value;

    if(isUsingCustomRes) {
      resX = customResXInput.value
      resY = customResYInput.value
    }

    requestChangeCapt(`left-count=${left}&right-count=${right}&top-count=${top}&bottom-count=${bottom}&avg-color=${isAvgColor}&h-offset=${hOffset}&v-offset=${vOffset}&res-x=${resX}&res-y=${resY}&blend-mode=${blendMode}&blend-depth=${blendDepth}`)
    .then(changes => {
      updateCaptSettings(changes)
      requestChangeSoundReact(isSoundWithScreen)
      .then(changes => {
        soundReact.checked = changes['srea'] >= 1;
        showScrnAndSndReactOptions()
      })
    })

  }

  window.updateCaptureButton = function(isCapturing) {
    if(isCapturing) { // if it is current capturing
      captureButton.innerHTML = 'Stop Capturing Screen';
      captureButton.classList.add('stop-option');
    } else {
        captureButton.innerHTML = 'Capture Screen';
        captureButton.classList.remove('stop-option');
    }
  }

  function updateCaptSettings(data) {
    vOffsetInput.value = data['v-offset']
    hOffsetInput.value = data['h-offset']
    leftCount.value = data['left-count']
    rightCount.value = data['right-count']
    topCount.value = data['top-count']
    bottomCount.value = data['bottom-count']
    blendDepthInput.value = data['blend-depth']

    resX = data['res-x']
    resY = data['res-y']
    customResXInput.value = resX;
    customResYInput.value = resY;
    
    setAvgColor(+data['avg-color']);
    setBlendMode(+data['blend-mode']);
  }

  function getAvgColor() {
    return avgColorInput.checked ? 1 : 0;
  }

  function setAvgColor(value) {
    avgColorInput.checked = value > 0;
  }

  function getBlendMode() {
    return blendModeActive.checked ? 1 : 0;
  }

  function setBlendMode(value) {
    blendModeActive.checked = value > 0;
  }
})