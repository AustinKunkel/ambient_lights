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
  vOffsetInput.addEventListener("keydown", function(event) {
    if (event.key === "Enter") {
      offset = vOffsetInput.value
      requestChangeOffset(offset, "v")
    }
  })

  const hOffsetInput = document.getElementById("h-offset-input")
  hOffsetInput.addEventListener("keydown", function(event) {
    if (event.key === "Enter") {
      offset = hOffsetInput.value
      requestChangeOffset(offset, "h")
    }
  })  

  const avgColorInput = document.getElementById("avg-scrn-color-check")
  avgColorInput.addEventListener("click", function() {
    if(avgColorInput.checked) {
      requestAvgColor(1)
    } else {
      requestAvgColor(0)
    }
  })

  function updateLedSettings(data) {
    brightnessInput.value = Number(data.bri)
    countInput.value = Number(data.cnt)
  }

  const leftCount = document.getElementById("left-led-count")
  const rightCount = document.getElementById("right-led-count")
  const topCount = document.getElementById("top-led-count")
  const bottomCount = document.getElementById("bottom-led-count")
  window.saveCaptSettings = function() {
    const left = leftCount.value
    const right = rightCount.value
    const top = topCount.value
    const bottom = bottomCount.value

    requestChangeCapt(`left-count=${left}&right-count=${right}&top-count=${top}&bottom-count=${bottom}`)
    .then(changes => {
      leftCount.value = changes['left-count']
      rightCount.value = changes['right-count']
      topCount.value = changes['top-count']
      bottomCount.value = changes['bottom-count']
    })

  }

  function updateCaptSettings(data) {
    vOffsetInput.value = data['v-offset']
    hOffsetInput.value = data['h-offset']
    leftCount.value = data['left-count']
    rightCount.value = data['right-count']
    topCount.value = data['top-count']
    bottomCount.value = data['bottom-count']

  }
})