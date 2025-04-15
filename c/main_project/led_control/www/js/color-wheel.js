document.addEventListener("DOMContentLoaded", async function() {
    const colorCodeInput = document.getElementById("color-code-input");

    isUpdatingFromInput = false;

    const colorPicker = new iro.ColorPicker('.color-picker', {
        width: 600,
        color: "#ffffff", // Default color
        layoutDirection: "vertical",
        sliderSize: 80,
        handleRadius: 25
    });

    const color_error_label = document.getElementById('color-input-error-label');

    let userColors = [];

    let activeColorButton = null;

    let currentColor = null;

    let isRemovingColor = false;

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
        changeLedColor(hexColor).then((data) => {
            led_settings.capture_screen = data['capt'] >= 1 ? 1 : 0;

            window.updateCaptureButton(led_settings.capture_screen == 1);
        })

        if(activeColorButton != null) {
            activeColorButton.classList.remove("selected");
        }
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
    
        if (!colorButton) {
            console.error(`Button with ID ${buttonId} not found.`);
            return;
        }
    
        const color = colorButton.style.backgroundColor;
    
        if(isRemovingColor) {
            requestRemoveUserColor(rgbToHex(color))
            .then(data => {
                const colorContainer = document.getElementById("color-easy-container");
                const buttonToRemove = document.getElementById(buttonId); // Find the button by ID
                isRemovingColor = false;
                userColors = data.colors || [];
                updateUserColors();
            });
        } else {
            colorButton.classList.add("selected");
            activeColorButton = colorButton;
            updateColorPickerFromInput(color);
        }
    }
    


    // Function to create and append a button with the given color and ID
    function createColorButton(id, color, className = null) {
        const button = document.createElement('button');
        button.id = id;  // Set ID based on the provided value
        button.style.backgroundColor = color;  // Set the background color
        if(className) {
            button.className = className;
            if(className == 'color-easy color')
            button.addEventListener('click', (event) => {
                handleColorButtonSelect(id);
            })
        }

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

            removeButton = createColorButton("remove-user-color", "#cccccc");
            removeButton.innerHTML= "<i class='fa-solid fa-minus'></i>";
            removeButton.classList.add("color");
            removeButton.addEventListener('click', (event) => {
                toggleRemoveColor();
            })
            colorContainer.appendChild(removeButton)

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
        addButton.addEventListener('click', (event) => {
            openAddColorMenu();
        });
        colorContainer.appendChild(addButton);
    }

    window.addUserColor = function(color = currentColor, event = null) {

        if(event) {
            event.preventDefault();
        }
        if(!color) {
            message_pop_up(TYPE.ERROR, "no color selected!");
            return 
        }
        requestAddUserColor(color).then(data => {
            userColors = data.colors || [];
            updateUserColors();
        });

        updateColorPickerFromInput(color, TYPE.ERROR,"Error updating color");
    }

    function toggleRemoveColor() {
        isRemovingColor = !isRemovingColor;

        colors = document.querySelectorAll('.color-easy');

        if(isRemovingColor) {
            colors.forEach((element) => {
                element.innerHTML = "<i class='fa-solid fa-x'></i>"
                element.classList.add("space-10")
            })
        } else {
            colors.forEach((element) => {
                element.innerHTML = ""
                element.classList.remove("space-10")
            })
        } 
    }

    function rgbToHex(rgb) {
        const rgbMatch = rgb.match(/^rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)$/i);

        const [_, r, g, b] = rgbMatch.map(Number);

        const toHex = (num) => num.toString(16).padStart(2, '0').toUpperCase();

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

        led_settings.capture_screen = led_settings.capture_screen == 1 ? 0 : 1; // flip

        sendLedSettingsPost(led_settings).then(() => {
            sendLedSettingsGet().then((data) => {
                led_settings = { ...data };
            })
            window.updateCaptureButton(led_settings.capture_screen > 0);
        })
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
