let led_settings = {
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

let socket;
function startWebSocket() {
  socket = new WebSocket('ws://' + window.location.hostname + ':8080', 'websocket');

  socket.onopen = function(event) {
    console.log('WebSocket connection opened.');
  };

  socket.onmessage = function(event) {
    console.log('Message from server:', event.data);

    try {
      const message = JSON.parse(event.data);
      console.log('Parsed Data:', message);
      const { action, data } = message;

      switch (action) {
        case "get_led_settings":
          led_settings = {...data};
          updateLedSettings();
        case "get_capt_settings":
          break;
        case "led_pixel_data":
          break;
      }
    } catch (e) {
      console.log("received plain message:", event.data);
    }
  };

  socket.onclose = function(event) {
    console.log('WebSocket connection closed.');
  };

  socket.onerror = function(error) {
    console.error('WebSocket error:', error);
  };
}

function getLEDSettings() {
  if(socket && socket.readyState == WebSocket.OPEN) {
    socket.send(JSON.stringify({
      action : "get_led_settings"
    }));
    console.log("Get led settings sent");
  }
}

function setServerLEDSettings() {
  if(socket && socket.readyState == WebSocket.OPEN) {
    socket.send(JSON.stringify({
      action: "set_led_settings",
      data: led_settings
    }));
  } else {
    console.log("WebSocket is not open.");
  }
}

function sendMessage() {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send('Hello from the browser!');
    console.log('Message sent.');
  } else {
    console.log('WebSocket is not open.');
  }
}

function closeWebSocket() {
  if (socket) {
    socket.close();
  }
}

window.onload = function() {
  startWebSocket();
};

document.addEventListener("DOMContentLoaded", async function() {

  // sendLedSettingsGet().then(data => {
  //   led_settings = { ...data }; 
  //   updateLedSettings();
  // })

  requestGetCaptSettings().then (data => {
    updateCaptSettings(data)
  })
  const brightnessValue = document.getElementById("brightness-value");
  const brightnessInput = document.getElementById("brightness-input");
  brightnessInput.addEventListener("input", () => {
      led_settings.brightness = Number(brightnessInput.value)
      brightnessValue.textContent = brightnessInput.value;

      setServerLEDSettings();
      // sendLedSettingsPost(led_settings).then(()=> {
      //   // request get settings and update led_settings
      // });
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

  const hamburgerIcon = document.getElementById('hamburger-icon');

  const sidebar = document.getElementById('sidebar');
  let sidebarOpen = false;

  window.addEventListener('scroll', () => {
    const header = document.getElementsByTagName('header')[0];
    const headerTitle = document.getElementById('header-title');

    const scrollY = window.scrollY;

    if(scrollY >= 275) {
      header.style.backgroundColor = "#121212";
      headerTitle.style.display = "block";
    } else {  
      header.style.backgroundColor = "transparent";
      headerTitle.style.display = "none";
    }
  });

  window.toggleSidebar = () => {
    sidebarOpen = !sidebarOpen;
    console.log(sidebarOpen);
    sidebar.style.left = sidebarOpen ? 0 : '-100%';
  }

  window.updateLedSettings = () => {
    brightnessValue.textContent = led_settings.brightness;
    brightnessInput.value = led_settings.brightness;
    countInput.value = Number(led_settings.count);
    soundReact.checked = led_settings.sound_react >= 1;
    showScrnAndSndReactOptions();
    console.log(led_settings.capture_screen > 0);
    updateCaptureButton(led_settings.capture_screen > 0);
    initializeColorPicker();
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

  let isReactingToSound = false;
  window.reactToSound = () => {
    isReactingToSound = !isReactingToSound;
    const soundReactButton = document.getElementById('sound-react-button');
    if(isReactingToSound) {
      soundReactButton.innerHTML = 'Stop Reacting to Sound';
      soundReactButton.classList.add('stop-option');
    } else {
      soundReactButton.innerHTML = 'React to Sound';
      soundReactButton.classList.remove('stop-option');
    }
  }

  function updateSoundCaptureButton() {

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


  const width = 640;
  const height = 480;
  const scale = 7;

  const cols = Math.floor(width / scale);
  const rows = Math.floor(height / scale);

  function getEdgeIndices(cols, rows) {
    const indices = [];

    const length = '10px';
    const width = '10px';

    // Top row
    for (let i = 0; i < cols; i++) {
      indices.push({ row: 0, col: i, width: width, height: length });
    }
  
    // Right column
    for (let i = 1; i < rows - 1; i++) {
      indices.push({ row: i, col: cols - 1, width: length, height: width });
    }
  
    // Bottom row
    for (let i = cols - 1; i >= 0; i--) {
      indices.push({ row: rows - 1, col: i, width: width, height: length });
    }
  
    // Left column
    for (let i = rows - 2; i > 0; i--) {
      indices.push({ row: i, col: 0, width: length, height: width });
    }
  
    return indices;
  }

  const edgeCoords = getEdgeIndices(cols, rows);
  const container = document.getElementById('pixel-grid');

  // Optional: style container with fixed size

  const edgePixels = [];

  edgeCoords.forEach(coord => {
    const pixel = document.createElement('div');
    pixel.className = 'pixel';

    // Position using absolute positioning
    pixel.style.position = 'absolute';
    pixel.style.width = coord.width;
    pixel.style.height = coord.height;
    pixel.style.left = `${(coord.col / cols) * 100}%`;
    pixel.style.top = `${(coord.row / rows) * 100}%`;
    pixel.style.backgroundColor = "red";

    container.appendChild(pixel);
    edgePixels.push(pixel);
  });

  function updateEdgePixels(colorArray) {
    for (let i = 0; i < edgePixels.length; i++) {
      const color = colorArray[i];
      edgePixels[i].style.backgroundColor = color;
    }
  }
})