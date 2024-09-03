document.addEventListener("DOMContentLoaded", async function() {
    const colorCodeInput = document.getElementById("color-code-input");

    isUpdatingFromInput = false;

    const colorPicker = new iro.ColorPicker('.color-picker', {
        width: 200,
        color: "#ffffff", // Default color
        layoutDirection: "vertical"
    });

    const color_error_label = document.getElementById('color-input-error-label');

    let userColors = [];

    activeColorButton = null;

    let currentColor = null;

    getUserColors().then(data => {
        userColors = data.colors || [];
        updateUserColors();
    });


    // Function to set the color picker and input field to the current color
    async function initializeColorPicker() {
        const color = await getCurrColor();
        if (color) {
            colorPicker.color.set(color);
            colorCodeInput.value = color;
        }
    }
    // Call the function to initialize color picker on page load
    initializeColorPicker();


    colorPicker.on('color:change', function(color) {
        let hexColor = color.hexString;
        if(!isUpdatingFromInput) {
            colorCodeInput.value = hexColor;
        }
        color_error_label.style="display: none";
        currentColor = hexColor;
        change_led_color(hexColor);
    });

    window.changeColor = function() {
        const color = colorCodeInput.value;
        updateColorPickerFromInput(color, TYPE.WARNING, "Not a valid hex!");
    }

    
    function updateColorPickerFromInput(color, errorType = TYPE.ERROR, message = "Error updating color") {
        try {
            isUpdatingFromInput = true;
            colorPicker.color.set(color);
            isUpdatingFromInput = false;

            if(isRGB(color)) {
                color = rgbToHex(color);
            }
            colorCodeInput.value = color;
        } catch (error) {
            isUpdatingFromInput = false;
            message_pop_up(errorType, message);
        }
    }


    function handleColorButtonSelect(buttonId) {
        // updates the button selected
        if(activeColorButton) {
            activeColorButton.classList.remove("selected");
        }

        const colorButton = document.getElementById(buttonId);
        colorButton.classList.add("selected");

        const color = colorButton.style.backgroundColor;

        activeColorButton = colorButton;

        updateColorPickerFromInput(color);
    }

// Function to create and append a button with the given color and ID
function createColorButton(id, color, className = null) {
    const button = document.createElement('button');
    if(className) {
        button.className = className;
        button.addEventListener('click', (event) => {
            handleColorButtonSelect(event.target.id);
        })
    }
    button.id = id;  // Set ID based on the provided value
    button.style.backgroundColor = color;  // Set the background color

    return button;
}

function updateUserColors() {
    const colorContainer = document.getElementById("color-easy-container");

     colors = document.querySelectorAll('.color');

    colors.forEach((element) => {
        element.remove();
    })

    if (userColors) {
        // Add multiple color buttons
        let index = 1;
        userColors.forEach((color) => {
            const buttonId = `color${index++}`; // Use key or unique value for ID
            const button = createColorButton(buttonId, color, 'color-easy color');
            colorContainer.appendChild(button);
        });
    }
    

    addButton = createColorButton("add-user-color", "#cccccc");
    addButton.innerHTML = "<i class='fa-solid fa-plus'></i>";
    addButton.classList.add("color");
    colorContainer.appendChild(addButton);
    addButton.addEventListener('click', (event) => {
        openAddColorMenu();
    });
}

window.addUserColor = function(color = currentColor) {
    if(!color) {
        message_pop_up(TYPE.ERROR, "no color selected!");
        return 
    }

    console.log("adding color:", color);
    requestAddUserColor(color).then(data => {
        console.log(data);
        userColors = data.colors || [];
        console.log(userColors.length);
        updateUserColors();
    });

    updateColorPickerFromInput(color, TYPE.ERROR,"Error updating color");
}

function rgbToHex(rgb) {
    const rgbMatch = rgb.match(/^rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)$/i);

    const [_, r, g, b] = rgbMatch.map(Number);

    const toHex = (num) => num.toString(16).padStart(2, '0');

    return `#${toHex(r)}${toHex(g)}${toHex(b)}`;
}


function isRGB(color) {
    const rgbRegex = /^rgb\(\s*\d{1,3}\s*,\s*\d{1,3}\s*,\s*\d{1,3}\s*\)$/i;
    const rgbaRegex = /^rgba\(\s*\d{1,3}\s*,\s*\d{1,3}\s*,\s*\d{1,3}\s*,\s*[\d\.]+\s*\)$/i;
    return rgbRegex.test(color) || rgbaRegex.test(color);
}

window.getCapture = function() {
    if(activeColorButton) {
        activeColorButton.classList.remove("selected");
    }
    activeColorButton = null;

    request_capture();
}

/**
 * Runs oninput of adding a new color to the users "easy colors" section
 */
window.checkUserColor = function() {
    const userColorInput = document.getElementById("add-user-color-input");
    const saveButton = document.getElementById("save-user-color");

    if(userColorInput.value.length <= 0 || !isHex(userColorInput.value)) {
        color_error_label.style.display = "flex";
        saveButton.classList.add("grayed-out");
        saveButton.disabled = true;
        return
    }

    color_error_label.style.display = "none";
    saveButton.classList.remove("grayed-out");
    saveButton.disabled = false;

}

function isHex(color) {
    // Regular expression for 3 or 6 digit hex color codes
    const hexRegex = /^#([0-9A-Fa-f]{3}){1,2}$/;
    return hexRegex.test(color);
}

});
