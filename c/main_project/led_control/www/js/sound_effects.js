document.addEventListener("DOMContentLoaded", async function() {
  const effectContainer = document.getElementById('sound-effect-select');
  let selectedEffect = {}

  requestGetSoundEffects().then(data => {
    for(const [name, filename] of Object.entries(data)) {
      const effectElement = document.createElement('option');
      effectElement.textContent = name;
      effectElement.value = filename;
      effectContainer.appendChild(effectElement);
    }
  });

  effectContainer.addEventListener('change', function (event) {
    const selectedEffectName = event.target.options[event.target.selectedIndex].textContent;
    const selectedEffectPath = event.target.value;

    requestSetSoundEffect(selectedEffectName, selectedEffectPath)
    .then(() => {
      message_pop_up(TYPE.OK, `Sound effect selected: ${selectedEffectName}`)
    })
  })

  window.showScrnAndSndReactOptions = function() {
    //const checkbox = document.getElementById("snd-rct-check");
    const options = document.getElementById("scrn-sound-react-options");
    // if(checkbox.checked) {
    //   options.style.display = 'block';
    // } else {
    //   options.style.display = 'none';
    // }
  }
})