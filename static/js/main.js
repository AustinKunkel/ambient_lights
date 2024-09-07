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
    const isAvgColor = getAvgColor()
    const hOffset = hOffsetInput.value
    const vOffset = vOffsetInput.value

    requestChangeCapt(`left-count=${left}&right-count=${right}&top-count=${top}&bottom-count=${bottom}&avg-color=${isAvgColor}&h-offset=${hOffset}&v-offset=${vOffset}`)
    .then(changes => {
      updateCaptSettings(changes)
    })

  }

  function updateCaptSettings(data) {
    vOffsetInput.value = data['v-offset']
    hOffsetInput.value = data['h-offset']
    leftCount.value = data['left-count']
    rightCount.value = data['right-count']
    topCount.value = data['top-count']
    bottomCount.value = data['bottom-count']
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
})