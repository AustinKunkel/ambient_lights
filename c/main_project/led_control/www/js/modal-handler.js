  const TYPE = {
    OK: 'ok',
    ERROR: 'error',
    WARNING: 'warning'
  }

  const ICONS = {
    [TYPE.OK]: "fa-solid fa-circle-check",
    [TYPE.ERROR]: "fa-solid fa-circle-exclamation",
    [TYPE.WARNING]: "fa-solid fa-triangle-exclamation fa-lg"
  }

  function message_pop_up(type,message) {
    if (!Object.values(TYPE).includes(type))  {
      console.error('Invalid type provided');
      return;
    }

    const modal = document.getElementById('message-container');
    const modal_icon = document.getElementById('message-icon');
    const modal_message = document.getElementById('modal-message');

    const iconClass = ICONS[type];

    modal_icon.className = iconClass;

    modal_message.innerHTML = message;

    modal.style = "display: flex";

    setTimeout(() => {
      modal.style.opacity = '0'; // Trigger fade-out
      // Hide the modal completely after transition ends
      setTimeout(() => {
        modal.style.display = "none";
      }, 1000); // Match the duration of the opacity transition
    }, 4000); // Delay before starting fade-out
  }

  function openAddColorMenu() {
    const modal = document.getElementById('add-color-menu');
    modal.style = "display: flex";

    color_input = document.getElementById("add-user-color-input");

    color_input.addEventListener("keydown", function(event) {
      if (event.key === "Enter") {
          addColor(color_input.value);
      }
  });
  }

  window.closeAddColorMenu = function() {
    const modal = document.getElementById('add-color-menu');

    modal.style = "display: none";
  }
