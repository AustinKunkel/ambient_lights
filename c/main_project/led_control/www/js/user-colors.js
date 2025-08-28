
let isRemovingColor = false;

let userColors = [];

function handleColorButtonClick(color) {
  if (isRemovingColor) {
    userColors = userColors.filter(c => c !== color);
    setServerUserColors(userColors)
  } else {
    updateColorPickerFromInput(color);
    changeColor(color);
  }
}

function invertColor(hexValue) {
  // Remove the # symbol if it exists
  let hex = hexValue.startsWith('#') ? hexValue.slice(1) : hexValue;

  // Handle 3-digit hex codes by expanding them
  if (hex.length === 3) {
    hex = hex.split('').map(char => char + char).join('');
  }

  // Validate the hex code length
  if (hex.length !== 6) {
    throw new Error(`Invalid HEX color: ${hexValue}`);
  }

  // Convert to integer, invert, and convert back to hex
  const inverted = (0xFFFFFF ^ parseInt(hex, 16)).toString(16);

  // Pad the result with leading zeros if necessary
  return `#${inverted.padStart(6, '0')}`;
};


function updateUserColors(data) {
  console.log("Updating user colors: ", data);
  const colorContainer = document.getElementById("user-colors");
  colors = document.querySelectorAll('.color-circle');

  colors.forEach((element) => {
    element.remove();
  })

  userColors = [...data];

  if(data) {
    removeButton = document.createElement('button');
    removeButton.id = "remove-user-color";
    removeButton.innerHTML= "<i class='fa-solid fa-minus'></i>";
    removeButton.classList.add("color-circle");
    removeButton.addEventListener('click', (event) => {
        toggleRemoveColor();
    })
    colorContainer.appendChild(removeButton);

    let index = 1;
    data.forEach((color) => {
      const colorButton = document.createElement('button');
      colorButton.classList.add("color-circle");
      colorButton.style.backgroundColor = color;
      colorButton.style.color = invertColor(color);
      colorButton.addEventListener('click', (event) => {
          handleColorButtonClick(color);
      })
      colorContainer.appendChild(colorButton);
      index += 1;
    });

    addButton = document.createElement('button');
    addButton.id = "add-user-color";
    addButton.innerHTML = "<i class='fa-solid fa-plus'></i>";
    addButton.classList.add("color-circle");
    addButton.addEventListener('click', (event) => {
        showAddColorOverlay();
    });
    colorContainer.appendChild(addButton);
  }
}


function toggleRemoveColor() {
  isRemovingColor = !isRemovingColor;
  const removeButton = document.getElementById("remove-user-color");
  const addButton = document.getElementById("add-user-color");
  const colorButtons = document.querySelectorAll('.color-circle');

  colorButtons.forEach((button) => {
    if (button !== removeButton && button !== addButton) {
      if (isRemovingColor) {
        button.classList.add("shake");
        button.innerHTML = "<i class='fa-solid fa-trash'></i>";
      } else {
        button.classList.remove("shake");
        button.innerHTML = "";
      }
    }
  });

  if (isRemovingColor) {
    removeButton.style.backgroundColor = "var(--error)";
    removeButton.style.color = "var(--bg-primary)";
  } else {
    removeButton.style.backgroundColor = "transparent";
    removeButton.style.color = "var(--text-primary)";
  }
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

function addCustomColor() {
  const colorInput = document.getElementById("add-user-color-input");

  userColors.push(colorInput.value)

  updateUserColors();
  setServerUserColors();
  hideAddColorOverlay();
}