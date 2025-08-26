document.addEventListener("DOMContentLoaded", () => {

  const
});

let isRemovingColor = false;

function handleColorButtonClick(color) {
  if (isRemovingColor) {
    removeColor(color);
  } else {
    setColor(color);
  }
}

// Function to create and append a button with the given color
function createColorButton(className=null, color) {
    const button = document.createElement('button');
    button.style.backgroundColor = color;  // Set the background color
    if(className) {
        button.className = className;
        if(className == 'color-easy color')
        button.addEventListener('click', (event) => {
            handleColorButtonSelect(color);
        })
    }

    return button;
}

function updateUserColors(data) {
  const colorContainer = document.getElementById("user-colors");
  colors = document.querySelectorAll('.color-circle');

  colors.forEach((element) => {
    element.remove();
  })

  if(data) {
    removeButton = createColorButton("remove-user-color", "#cccccc");
    removeButton.innerHTML= "<i class='fa-solid fa-minus'></i>";
    removeButton.classList.add("color");
    removeButton.addEventListener('click', (event) => {
        toggleRemoveColor();
    })
    colorContainer.appendChild(removeButton);

    let index = 1;
    data.forEach((color) => {
      const colorButton = createColorButton(`user-color-${index}`, color);
      colorButton.classList.add("color-circle", "color");
      colorButton.title = color;
      colorButton.addEventListener('click', (event) => {
          handleColorButtonClick(color);
      })
      colorContainer.appendChild(colorButton);
      index += 1;
    });

    addButton = createColorButton("add-user-color", "#cccccc");
    addButton.innerHTML = "<i class='fa-solid fa-plus'></i>";
    addButton.classList.add("color");
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