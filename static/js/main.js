document.addEventListener("DOMContentLoaded", async function() {

  requestGetLedSettings().then( data => {
    updateLedSettings(data)
  })

  requestGetCaptSettings().then (data => {
    updateCaptSettings(data)
  })

  const brightnessInput = document.getElementById("brightness-input")
  brightnessInput.addEventListener("input", () => {
      brightness = brightnessInput.value
      requestChangeBrightness(brightness)
  })

  const countInput = document.getElementById("count-input")
  countInput.addEventListener("keydown", function(event) {
    if (event.key === "Enter") {
      count = countInput.value
      requestChangeLedCount(count)
    }
  })

  const vOffsetInput = document.getElementById("v-offset-input")
  const hOffsetInput = document.getElementById("h-offset-input")

  const avgColorInput = document.getElementById("avg-scrn-color-check")
  const soundWithScreenInput = document.getElementById("snd-rct-check")
  const captureButton = document.getElementById('capture_button');

  function updateLedSettings(data) {
    brightnessInput.value = Number(data.bri)
    countInput.value = Number(data.cnt)
    soundWithScreenInput.checked = data.srea >= 1;
    isCapturing = data.capt >= 1;
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
    const left = leftCount.value
    const right = rightCount.value
    const top = topCount.value
    const bottom = bottomCount.value
    const isAvgColor = getAvgColor()
    const isSoundWithScreen = soundWithScreenInput.checked ? 1 : 0;
    const hOffset = hOffsetInput.value
    const vOffset = vOffsetInput.value

    if(isUsingCustomRes) {
      resX = customResXInput.value
      resY = customResYInput.value
    }

    requestChangeCapt(`left-count=${left}&right-count=${right}&top-count=${top}&bottom-count=${bottom}&avg-color=${isAvgColor}&h-offset=${hOffset}&v-offset=${vOffset}&res-x=${resX}&res-y=${resY}`)
    .then(changes => {
      updateCaptSettings(changes)
      requestChangeSoundReact(isSoundWithScreen)
      .then(changes => {
        soundWithScreenInput.checked = changes['srea'] >= 1;
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

    resX = data['res-x']
    resY = data['res-y']
    customResXInput.value = resX;
    customResYInput.value = resY;
    
    setAvgColor(+data['avg-color'])
  }

  function getAvgColor() {
    if(avgColorInput.checked) {
      return 1
    }
    return 0
  }

  function setAvgColor(value) {
    if(value > 0){
      avgColorInput.checked = true
    } else {
      avgColorInput.checked = false
    }
  }

  window.showScrnAndSndReactOptions = function() {
    const checkbox = document.getElementById("snd-rct-check");
    const options = document.getElementById("scrn-sound-react-options");
    if(checkbox.checked) {
      options.style.display = 'block';
    } else {
      options.style.display = 'none';
    }
  }
})