function toggleBlendMode() {
  capt_settings.blend_mode = document.getElementById("blend-toggle").checked ? 1 : 0;
  saveCaptSettingsButtonContainer.classList.remove("hidden-container");
}

function toggleAutoOffsetMode() {
  capt_settings.auto_offset = document.getElementById("auto-offset-toggle").checked ? 1 : 0;
  saveCaptSettingsButtonContainer.classList.remove("hidden-container");
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


  
});
