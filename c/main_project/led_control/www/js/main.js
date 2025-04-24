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
  'auto_offset' : 1,
  'transition_rate': .3
}

function getEdgeIndices() {
  const indices = [];

  // These control the size of each pixel
  const length = '15px';  // longer side
  const width = '15px';    // shorter side

  const containerPadding = 0; // percentage padding for better alignment

  // Right column
  let i_roof = Math.floor((capt_settings.right_count - 2));
  for (let i = 0; i < i_roof; i++) {
    indices.push({
      row: i_roof - i,
      col: 'right',
      width: length,
      height: width
    });
  }

  // Top row
  i_roof = Math.floor((capt_settings.top_count - 1));
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
  i_roof = Math.floor((capt_settings.left_count - 2));
  for (let i = i_roof; i > 0; i--) {
    indices.push({
      row: i,
      col: 0,
      width: length,
      height: width
    });
  }

  // Bottom row
  i_roof = Math.floor((capt_settings.bottom_count - 2));
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

function updateEntirePixelFrame () {
  const edgeCoords = getEdgeIndices();
  const container = document.getElementById('pixel-grid');
  container.innerHTML = ''; // Clear the container
  edgePixels.length = 0; // Clear the edgePixels array

  const horizontalMargin = 5; // Horizontal space between sides (left, right)
  const verticalMargin = 5; // Vertical space between sides (top, bottom)

  // Calculate the available space after subtracting margins
  const availableWidth = container.offsetWidth - (2 * horizontalMargin);
  const availableHeight = container.offsetHeight - (2 * verticalMargin);

  // Calculate spacing for each side based on available space
  const horizontalSpacing = availableWidth / Math.floor(capt_settings.top_count);
  const verticalSpacing = availableHeight / Math.floor(capt_settings.left_count);

  // Ensure pixels take up the available space without overshooting
  const pixelWidth = horizontalSpacing; // Each pixel will take up the full width space
  const pixelHeight = verticalSpacing; // Each pixel will take up the full height space

  edgeCoords.forEach(coord => {
    const pixel = document.createElement('div');
    pixel.className = 'pixel';
    pixel.style.position = 'absolute';
    pixel.style.width = `${pixelWidth}px`;
    pixel.style.height = `${pixelHeight}px`;
    pixel.style.backgroundColor = 'black';
  
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

function updateEdgePixels (colorArray) {
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

  function rgbToHsl(r, g, b) {
    r /= 255; g /= 255; b /= 255;
    const max = Math.max(r, g, b), min = Math.min(r, g, b);
    let h, s, l = (max + min) / 2;
  
    if (max === min) {
      h = s = 0;
    } else {
      const d = max - min;
      s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
      switch (max) {
        case r: h = ((g - b) / d + (g < b ? 6 : 0)); break;
        case g: h = ((b - r) / d + 2); break;
        case b: h = ((r - g) / d + 4); break;
      }
      h /= 6;
    }
    return [h, s, l];
  }
  
  function hslToRgb(h, s, l) {
    let r, g, b;
    if (s === 0) {
      r = g = b = l; // achromatic
    } else {
      const hue2rgb = (p, q, t) => {
        if (t < 0) t += 1;
        if (t > 1) t -= 1;
        if (t < 1/6) return p + (q - p) * 6 * t;
        if (t < 1/2) return q;
        if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
        return p;
      };
      const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
      const p = 2 * l - q;
      r = hue2rgb(p, q, h + 1/3);
      g = hue2rgb(p, q, h);
      b = hue2rgb(p, q, h - 1/3);
    }
    return [
      Math.round(r * 255),
      Math.round(g * 255),
      Math.round(b * 255)
    ];
  }

  while (i < colorArray.length) {
    const chunkSize = Math.min(1, colorArray.length - i);
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

    let [h, s, l] = rgbToHsl(r, g, b);
    l = Math.min(1, l + .15); // brighten

    [r, g, b] = hslToRgb(h, s, l);

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

let saveCaptSettingsButtonContainer = null;

let socket;

function startWebSocket() {
  socket = new WebSocket('ws://' + window.location.hostname + ':80', 'websocket');

  socket.onopen = function(event) {
    console.log('WebSocket connection opened.');
    hideReconnectOverlay();
    message_pop_up(TYPE.OK, "Connected.");
  };

  socket.onmessage = function(event) {
    //console.log('Message from server:', event.data);
    const message = JSON.parse(event.data);
    //console.log('Parsed Data:', message);
    const { action, data } = message;

    switch (action) {
      case "get_led_settings":
       //console.log('Message from server:', event.data);
        led_settings = {...data};
        updateLedSettings();
        updateEntirePixelFrame();
      case "get_capt_settings":
       // console.log('Message from server:', event.data);
        capt_settings = { ...data, transition_rate: parseFloat(data.transition_rate.toFixed(2)) };
        updateCaptSettings();
        break;
      case "led_pixel_data":
        updateEdgePixels(data);
        break;
    }
  };

  socket.onclose = function(event) {
    console.log('WebSocket connection closed.');
    showReconnectOverlay();
  };

  socket.onerror = function(error) {
    console.error('WebSocket error:', error);
    message_pop_up(TYPE.ERROR, "Websocket Error");
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

function getCaptSettings() {
  if(socket && socket.readyState == WebSocket.OPEN) {
    socket.send(JSON.stringify({
      action : "get_capt_settings"
    }))
  }
}

function setServerLEDSettings() {
  if(socket && socket.readyState == WebSocket.OPEN) {
    socket.send(JSON.stringify({
      action: "set_led_settings",
      data: led_settings
    }));
    message_pop_up(TYPE.OK, "Saved");
  } else {
    console.log("WebSocket is not open.");
  }
}

function setServerCaptSettings() {
  if(socket && socket.readyState == WebSocket.OPEN) {
    socket.send(JSON.stringify({
      action: "set_capt_settings",
      data : capt_settings
    }));
    message_pop_up(TYPE.OK, "Saved");
  } else {
    console.log("Websocket is not open");
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

function saveLEDCount() {
  setServerLEDSettings();

  const saveCountButton = document.getElementById("count-save-button");
  saveCountButton.disabled = true;
  saveCountButton.classList.add("disabled-button");
}

document.addEventListener("DOMContentLoaded", async function() {

  updateEntirePixelFrame();

  saveCaptSettingsButtonContainer = document.getElementById("save-capt-settings-container");

  // sendLedSettingsGet().then(data => {
  //   led_settings = { ...data }; 
  //   updateLedSettings();
  // })
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

  const countInput = document.getElementById("count-input");
  const saveCountButton = document.getElementById("count-save-button");
  countInput.addEventListener("input", () => {
    
    led_settings.count = Number(countInput.value);

    saveCountButton.disabled = false;
    saveCountButton.classList.remove("disabled-button");

  })
  // countInput.addEventListener("keydown", function(event) {
  //   if (event.key === "Enter") {
  //     led_settings.count = Number(countInput.value);
  //     sendLedSettingsPost(led_settings).then(() => {
  //       // request get settings and update led_settings
  //     });
  //   }
  // })

  const vOffsetInput = document.getElementById("v-offset-input");
  vOffsetInput.addEventListener("input", () => {
    capt_settings.v_offset = Number(vOffsetInput.value);

    saveCaptSettingsButtonContainer.classList.remove("hidden-container");
  })
  const hOffsetInput = document.getElementById("h-offset-input")
  hOffsetInput.addEventListener("input", () => {
    capt_settings.h_offset = Number(hOffsetInput.value);

    saveCaptSettingsButtonContainer.classList.remove("hidden-container");
  })

  

  const avgColorInput = document.getElementById("avg-scrn-color-check")
  //const soundReact = document.getElementById("snd-rct-check")
  const blendModeActive = document.getElementById('blend-mode-check');
  const captureButton = document.getElementById('capture_button');

  const blendDepthInput = document.getElementById('blend-depth-input');
  blendDepthInput.addEventListener("input", () => {
    const value = blendDepthInput.value;
    document.getElementById('blend-depth-value').textContent = value;
    capt_settings.blend_depth = Number(value);  // Ensure this matches your settings object
    saveCaptSettingsButtonContainer.classList.remove("hidden-container");
  });

  const hamburgerIcon = document.getElementById('hamburger-icon');

  const sidebar = document.getElementById('sidebar');
  let sidebarOpen = false;

  window.addEventListener('scroll', () => {
    const header = document.getElementsByTagName('header')[0];
    const headerTitle = document.getElementById('header-title');

    const scrollY = window.scrollY;

    if(scrollY >= 90) {
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
    //showScrnAndSndReactOptions();
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
  }

  const leftCount = document.getElementById("left-led-count")
  leftCount.addEventListener("input", () => {
    saveCaptSettingsButtonContainer.classList.remove("hidden-container");
    capt_settings.left_count = leftCount.value;
    console.log(capt_settings.left_count);
  })

  const rightCount = document.getElementById("right-led-count")
  rightCount.addEventListener("input", () => {
    saveCaptSettingsButtonContainer.classList.remove("hidden-container");
    capt_settings.right_count = rightCount.value;
    console.log(capt_settings.right_count);
  })

  const topCount = document.getElementById("top-led-count")
  topCount.addEventListener("input", () => {
    saveCaptSettingsButtonContainer.classList.remove("hidden-container");
    capt_settings.top_count = topCount.value;
    console.log(capt_settings.top_count);
  })

  const bottomCount = document.getElementById("bottom-led-count")
  bottomCount.addEventListener("input", () => {
    saveCaptSettingsButtonContainer.classList.remove("hidden-container");
    capt_settings.bottom_count = bottomCount.value;
    console.log(capt_settings.bottom_count);
  })

  const countDecrementIcons = document.querySelectorAll(".decrement-icon");
  countDecrementIcons.forEach(icon => {
    icon.addEventListener("click", () => {
      // Find the nearest .setting-title-and-description or .blend-settings container
      const parentContainer = icon.closest(".led-counts-input");
      if (!parentContainer) return;

      // Find the description inside that container
      const countInputField = parentContainer.querySelector('input[type="number"]');
      if (countInputField) {
        countInputField.value--;
        switch (countInputField.id) {
          case "left-led-count" : 
            capt_settings.left_count = Number(countInputField.value);
            saveCaptSettingsButtonContainer.classList.remove("hidden-container");
            break;
          case "right-led-count" : 
            capt_settings.right_count = Number(countInputField.value);
            saveCaptSettingsButtonContainer.classList.remove("hidden-container");
            break;
          case "top-led-count" : 
            capt_settings.top_count = Number(countInputField.value);
            saveCaptSettingsButtonContainer.classList.remove("hidden-container");
            break;
          case "bottom-led-count" : 
            capt_settings.bottom_count = Number(countInputField.value);
            saveCaptSettingsButtonContainer.classList.remove("hidden-container");
            break;
          default: break;
        }
      }
    });
  });

  const countIncrementIcons = document.querySelectorAll(".increment-icon");
  countIncrementIcons.forEach(icon => {
    icon.addEventListener("click", () => {
      // Find the nearest .setting-title-and-description or .blend-settings container
      const parentContainer = icon.closest(".led-counts-input");
      if (!parentContainer) return;

      // Find the description inside that container
      const countInputField = parentContainer.querySelector('input[type="number"]');
      if (countInputField) {
        countInputField.value++;
        switch (countInputField.id) {
          case "left-led-count" : 
            capt_settings.left_count = Number(countInputField.value);
            saveCaptSettingsButtonContainer.classList.remove("hidden-container");
            break;
          case "right-led-count" : 
            capt_settings.right_count = Number(countInputField.value);
            saveCaptSettingsButtonContainer.classList.remove("hidden-container");
            break;
          case "top-led-count" : 
            capt_settings.top_count = Number(countInputField.value);
            saveCaptSettingsButtonContainer.classList.remove("hidden-container");
            break;
          case "bottom-led-count" : 
            capt_settings.bottom_count = Number(countInputField.value);
            saveCaptSettingsButtonContainer.classList.remove("hidden-container");
            break;
          default: break;
        }
      }
    });
  });

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
    setServerCaptSettings();
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

  window.updateCaptSettings = () => { 
    saveCaptSettingsButtonContainer.classList.add("hidden-container");
    const autoOffsetInput = document.getElementById("auto-offset-toggle");

    vOffsetInput.value = capt_settings.v_offset;
    hOffsetInput.value = capt_settings.h_offset;
    leftCount.value = capt_settings.left_count;
    rightCount.value = capt_settings.right_count;
    topCount.value = capt_settings.top_count;
    bottomCount.value = capt_settings.bottom_count;
    blendDepthInput.value = capt_settings.blend_depth;
    document.getElementById('blend-depth-value').textContent = blendDepthInput.value;

    autoOffsetInput.checked = capt_settings.auto_offset > 0;
    if(capt_settings.auto_offset > 0) {
      document.getElementById('offsets').classList.add('grayed-out');
    } else {
      document.getElementById('offsets').classList.remove('grayed-out');
    }

    resX = capt_settings.res_x;
    resY = capt_settings.res_y;
    customResXInput.value = resX;
    customResYInput.value = resY;

    avgColorInput.checked = capt_settings.avg_color > 0;

    document.getElementById("blend-toggle").checked = capt_settings.blend_mode > 0;
    if(capt_settings.blend_mode <= 0) {
      document.getElementById('blend-options').classList.add('grayed-out');
    } else {
      document.getElementById('blend-options').classList.remove('grayed-out');
    }

    const colorTransitionInput = document.getElementById("color-transition-input");
    const colorTransitionValue = document.getElementById("color-transition-value");
    colorTransitionInput.value = capt_settings.transition_rate;
    colorTransitionValue.textContent = colorTransitionInput.value;
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