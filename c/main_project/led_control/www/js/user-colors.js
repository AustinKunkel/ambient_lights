document.addEventListener("DOMContentLoaded", () => {
});

let isRemovingColor = false;

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
removeColor()

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