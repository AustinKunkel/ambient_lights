let led_settings = {
  'brightness' : 100,
  'color' : "#FFFFFF",
  'capture_screen' : 0,
  'sound_react' : 0,
  'fx_num' : 0,
  'count' : 206,
  'id' : 2
}

let capt_settings = {
  'v_offset' : 0,
  'h_offset' : 0,
  'avg_color' : 0,
  'left_count' : 36,
  'right_count' : 37,
  'top_count' : 66,
  'bottom_count' : 67,
  'res_x' : 640,
  'res_y' : 480,
  'blend_depth' : 5,
  'blend_mode' : 1,
  'auto_offset' : 1
}

let socket;
function startWebSocket() {
  socket = new WebSocket('ws://' + window.location.hostname + ':8080', 'websocket');

  socket.onopen = function(event) {
    console.log('WebSocket connection opened.');
  };

  socket.onmessage = function(event) {
    //console.log('Message from server:', event.data);
    const message = JSON.parse(event.data);
    //console.log('Parsed Data:', message);
    const { action, data } = message;

    switch (action) {
      case "get_led_settings":
        console.log('Message from server:', event.data);
        led_settings = {...data};
        updateLedSettings();
      case "get_capt_settings":
        console.log('Message from server:', event.data);
        break;
      case "led_pixel_data":
        updateEdgePixels(data);
        break;
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
  //const soundReact = document.getElementById("snd-rct-check")
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

    if(scrollY >= 120) {
      header.style.backgroundColor = "#121212";
      headerTitle.style.display = "block";
    } else {  
      header.style.backgroundColor = "transparent";
      headerTitle.style.display = "none";
    }
  });

  window.toggleSidebar = () => {
    sidebarOpen = !sidebarOpen;
    sidebar.style.left = sidebarOpen ? 0 : '-100%';
  }

  window.updateLedSettings = () => {
    brightnessValue.textContent = led_settings.brightness;
    brightnessInput.value = led_settings.brightness;
    countInput.value = Number(led_settings.count);
    //soundReact.checked = led_settings.sound_react >= 1;
    showScrnAndSndReactOptions();
    updateCaptureButton(led_settings.capture_screen > 0);

    const colorPickerContainer = document.getElementById('color-picker-container');
    const pixelFrameContainer = document.getElementById('pixel-grid');
    if(led_settings.capture_screen > 0) {
      colorPickerContainer.style.opacity = 0;
      colorPickerContainer.style.pointerEvents = 'none';
      pixelFrameContainer.style.opacity = 1;
    } else {
      colorPickerContainer.style.opacity = 1;
      colorPickerContainer.style.pointerEvents = 'auto';
      pixelFrameContainer.style.opacity =  0;
    }
    
    pixelFrameContainer.style.opacity = led_settings.capture_screen > 0 ? 1 : 0;
    initializeColorPicker();
    updateEntirePixelFrame();
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
    const isSoundWithScreen = false//soundReact.checked ? 1 : 0;
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
        //soundReact.checked = changes['srea'] >= 1;
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
    //capt_settings = { ...data };

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

  const spacing = 1; // number of pixels to be averaged

  function getEdgeIndices() {
    const indices = [];
  
    // These control the size of each pixel
    const length = '15px';  // longer side
    const width = '15px';    // shorter side
  
    const containerPadding = 0; // percentage padding for better alignment

    // Right column
    let i_roof = Math.floor((capt_settings.right_count - 2)/ spacing);
    for (let i = 0; i < i_roof; i++) {
      indices.push({
        row: i_roof - i,
        col: 'right',
        width: length,
        height: width
      });
    }
  
    // Top row
    i_roof = Math.floor((capt_settings.top_count - 1) / spacing);
    for (let i = 0; i <= i_roof; i++) {
      indices.push({
        row: 0,
        col: i_roof - i,
        width: width,
        height: length,
        topOffset: `${containerPadding}%`
      });
    }

    // Left column
    i_roof = Math.floor((capt_settings.left_count - 2) / spacing);
    for (let i = i_roof; i > 0; i--) {
      indices.push({
        row: i,
        col: 0,
        width: length,
        height: width
      });
    }
  
    // Bottom row
    i_roof = Math.floor((capt_settings.bottom_count - 2) / spacing);
    for (let i = i_roof; i > 0; i--) {
      indices.push({
        row: 'bottom',
        col: i_roof - i,
        width: width,
        height: length,
        topOffset: `calc(100% - ${containerPadding}%)`
      });
    }
    return indices;
  }

  const edgePixels = [];

  updateEntirePixelFrame();

  function updateEntirePixelFrame() {
    const edgeCoords = getEdgeIndices();
    console.log("EdgeCoords:", edgeCoords);
    const container = document.getElementById('pixel-grid');
    container.innerHTML = ''; // Clear the container
    edgePixels.length = 0; // Clear the edgePixels array
  
    const horizontalMargin = 5; // Horizontal space between sides (left, right)
    const verticalMargin = 5; // Vertical space between sides (top, bottom)
  
    // Calculate the available space after subtracting margins
    const availableWidth = container.offsetWidth - (2 * horizontalMargin);
    const availableHeight = container.offsetHeight - (2 * verticalMargin);
  
    // Calculate spacing for each side based on available space
    const horizontalSpacing = availableWidth / Math.floor(capt_settings.top_count / spacing);
    const verticalSpacing = availableHeight / Math.floor(capt_settings.left_count / spacing);
  
    // Ensure pixels take up the available space without overshooting
    const pixelWidth = horizontalSpacing; // Each pixel will take up the full width space
    const pixelHeight = verticalSpacing; // Each pixel will take up the full height space
  
    edgeCoords.forEach(coord => {
      const pixel = document.createElement('div');
      pixel.className = 'pixel';
      pixel.style.position = 'absolute';
      pixel.style.width = `${pixelWidth}px`;
      pixel.style.height = `${pixelHeight}px`;
      pixel.style.backgroundColor = 'red';
    
      if (coord.col === 'right') {
        // Right edge: position exactly within the container
        const x = container.offsetWidth - horizontalMargin - pixelWidth;
        const y = verticalMargin + (coord.row * verticalSpacing);
        pixel.style.left = `${x}px`;
        pixel.style.top = `${y}px`;
      } else if (coord.row === 'bottom') {
        // Bottom edge: position exactly within the container
        const x = horizontalMargin + (coord.col * horizontalSpacing);
        const y = container.offsetHeight - verticalMargin - pixelHeight;
        pixel.style.left = `${x}px`;
        pixel.style.top = `${y}px`;
      } else if (coord.row === 0) {
        // Top edge: already correct
        const x = horizontalMargin + (coord.col * horizontalSpacing);
        pixel.style.left = `${x}px`;
        pixel.style.top = `${verticalMargin}px`;
      } else {
        // Left edge: already correct
        const x = horizontalMargin;
        const y = verticalMargin + (coord.row * verticalSpacing);
        pixel.style.left = `${x}px`;
        pixel.style.top = `${y}px`;
      }
    
      container.appendChild(pixel);
      edgePixels.push(pixel);
    });
  }

  window.updateEdgePixels = (colorArray) => {
    let groupedColors = [];
    let i = 0;
  
    // Helper: Convert hex to RGB
    const hexToRgb = (hex) => {
      const r = parseInt(hex.slice(1, 3), 16);
      const g = parseInt(hex.slice(3, 5), 16);
      const b = parseInt(hex.slice(5, 7), 16);
      return { r, g, b };
    };
  
    // Helper: Convert RGB to hex
    const rgbToHex = ({ r, g, b }) => {
      const toHex = (val) => val.toString(16).padStart(2, '0');
      return `#${toHex(r)}${toHex(g)}${toHex(b)}`;
    };
  
    while (i < colorArray.length) {
      const chunkSize = Math.min(spacing, colorArray.length - i);
      let r = 0, g = 0, b = 0;
  
      for (let j = 0; j < chunkSize; j++) {
        const { r: rr, g: gg, b: bb } = hexToRgb(colorArray[i + j].color);
        r += rr;
        g += gg;
        b += bb;
      }
  
      r = Math.round(r / chunkSize);
      g = Math.round(g / chunkSize);
      b = Math.round(b / chunkSize);
  
      groupedColors.push(rgbToHex({ r, g, b }));
      i += chunkSize;
    }
  
    // Apply to edgePixels
    for (let j = 0; j < edgePixels.length; j++) {
      if (groupedColors[j]) {
        edgePixels[j].style.backgroundColor = groupedColors[j];
      }
    }
  };
})