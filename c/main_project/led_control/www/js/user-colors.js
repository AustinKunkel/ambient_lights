document.addEventListener("DOMContentLoaded", () => {
});

let isRemovingColor = false;

let userColors = [];

function handleColorButtonClick(color) {
  if (isRemovingColor) {
    removeColor(color);
  } else {
    setColor(color);
  }
}

function updateUserColors(data) {
  const colorContainer = document.getElementById("user-colors");
  colors = document.querySelectorAll('.color-circle');

  colors.forEach((element) => {
    element.remove();
  })

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
        openAddColorMenu();
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
      } else {
        button.classList.remove("shake");
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