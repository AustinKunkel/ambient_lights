function toggleAvgColorMode() {
  const isChecked = document.getElementById("avg-scrn-color-check").checked;
  capt_settings.avg_color = isChecked ? 1 : 0;
  saveCaptSettingsButtonContainer.classList.remove("hidden-container");
}

function toggleBlendMode() {
  const isChecked = document.getElementById("blend-toggle").checked;
  capt_settings.blend_mode = isChecked ? 1 : 0;
  if(!isChecked) {
    document.getElementById('blend-options').classList.add('grayed-out');
  } else {
    document.getElementById('blend-options').classList.remove('grayed-out');
  }

  saveCaptSettingsButtonContainer.classList.remove("hidden-container");
}

function toggleAutoOffsetMode() {
  const isChecked = document.getElementById("auto-offset-toggle").checked;
  capt_settings.auto_offset = isChecked ? 1 : 0;
  if(isChecked) {
    document.getElementById('offsets').classList.add('grayed-out');
  } else {
    document.getElementById('offsets').classList.remove('grayed-out');
  }

  saveCaptSettingsButtonContainer.classList.remove("hidden-container");
}


function showReconnectOverlay(reconnectTime=-1, failedToConnect=false) {
  document.getElementById("fullscreen-overlay").classList.remove("hidden-container");
  document.body.classList.add('no-scroll');

  const reconnectTimer = document.getElementById("reconnect-timer");

  if(reconnectTime >= 0) {
    reconnectTimer.closest('p').style.display = "block";
    reconnectTimer.innerHTML = reconnectTime;
  } else if(failedToConnect){
    document.getElementById('failed-to-connect-dialog').style.display = "block";
    reconnectTimer.innerHTML = '';
    reconnectTimer.closest('p').style.display = "none";
  }
}

function hideReconnectOverlay() {
  document.getElementById("fullscreen-overlay").classList.add("hidden-container");
  document.body.classList.remove('no-scroll');

  const reconnectTimer = document.getElementById("reconnect-timer");
  reconnectTimer.innerHTML = '';
  reconnectTimer.closest('p').style.display = "none";
  document.getElementById('failed-to-connect-dialog').style.display = "none";
}

function showAddColorOverlay() {
  document.getElementById("add-color-overlay").classList.remove("hidden-container");
  document.body.classList.add('no-scroll');
}

function hideAddColorOverlay() {
  document.getElementById("add-color-overlay").classList.add("hidden-container");
  document.body.classList.remove('no-scroll');
}

document.addEventListener("DOMContentLoaded", () => {

  const colorTransitionInput = document.getElementById("color-transition-input");
  const colorTransitionValue = document.getElementById("color-transition-value");
  colorTransitionInput.addEventListener("input", () => {
    // set capture settings.transition to the value
    capt_settings.transition_rate = parseFloat(colorTransitionInput.value);
    saveCaptSettingsButtonContainer.classList.remove("hidden-container");
    colorTransitionValue.textContent = colorTransitionInput.value;
  })

  // Get all toggle icons (info icons and up-arrow icons)
  const toggleIcons = document.querySelectorAll(".setting-icon, .description-up-icon");

  toggleIcons.forEach(icon => {
    icon.addEventListener("click", () => {
      // Find the nearest .setting-title-and-description or .blend-settings container
      const parentContainer = icon.closest(".setting-title-and-description, .blend-settings");
      if (!parentContainer) return;

      // Find the description inside that container
      const description = parentContainer.querySelector(".setting-description");
      if (description) {
        description.classList.toggle("expanded");
      }
    });
  });

  const sidebarArrowIcons = document.querySelectorAll(".fa-angle-left");
  sidebarArrowIcons.forEach(icon => {
    icon.addEventListener("click", () => {
      const parentContainer = icon.closest(".sidebar-group");
      if(!parentContainer) return;

      icon.classList.toggle("rotated");

      const subListContainer = parentContainer.querySelector(".sidebar-subgroup");
      if (subListContainer) {
        subListContainer.classList.toggle("expanded");
      }
    })
  })
  
});
