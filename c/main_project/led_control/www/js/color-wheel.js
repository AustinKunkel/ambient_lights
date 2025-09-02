document.addEventListener("DOMContentLoaded", async function() {
    const colorCodeInput = document.getElementById("color-code-input");

    isUpdatingFromInput = false;

    let color_picker_element = null;

    const colorPicker = new iro.ColorPicker('.color-picker', {
        layout: [
            {
              component: iro.ui.Wheel
            }
        ],
        width: 700,
        color: "#ffffff", // Default color
        layoutDirection: "vertical",
        handleRadius: 35,
        wheelLightness: false
    });

    let activeColorButton = null;

    let currentColor = null;

    let isRemovingColor = false;

    const set_color_button = document.getElementById("set-color-button");


    // Function to set the color picker and input field to the current color
    window.initializeColorPicker = () => {
        const color = led_settings.color;
        if (color) {
            colorPicker.color.set(color);
            colorCodeInput.value = color;
            set_color_button.disabled = false;
            set_color_button.classList.remove('disabled-button');
            set_color_button.style.borderColor = color;
            if(color_picker_element) {
                color_picker_element.style.boxShadow = `0 0 50px ${color}`;
            }
        }
    }
    // Call the function to initialize color picker on page load
    initializeColorPicker();


    colorPicker.on('input:end', function(color) {
        const hexColor = color.hexString;
        changeColor(hexColor);
    });

    colorPicker.on('color:change', function(color) {
        let hexColor = color.hexString;
        colorCodeInput.value = hexColor;

        if(activeColorButton != null) {
            activeColorButton.classList.remove("selected");
        }
        set_color_button.disabled = false;
        set_color_button.classList.remove('disabled-button');
        set_color_button.style.borderColor = hexColor;
        set_color_button.style.boxShadow = `0 0 10px ${hexColor}`;
        color_picker_element.style.boxShadow = `0 0 50px ${hexColor}`;
        currentColor = hexColor
    });

    colorPicker.on('mount', function() {
        color_picker_element = document.querySelector('.IroWheelBorder');
    });

    window.verifyColorInput = function() {
        const color = colorCodeInput.value;
        updateColorPickerFromInput(color, TYPE.WARNING, "Not a valid hex!");
    }

    window.changeColor = function(color = -1) {
        if(color == -1) {
            color = colorCodeInput.value;
        }
        led_settings.color = color;
        led_settings.capture_screen = 0;
        led_settings.sound_react = 0;
        led_settings.fx_num = 0;
        setServerLEDSettings();
        getLEDSettings();
        // sendLedSettingsPost(led_settings).then(
        //     sendLedSettingsGet().then((data) => {
        //         led_settings = { ...data };
        //         updateLedSettings();
        //     })
        // );
    }
    

    window.updateColorPickerFromInput = (color, errorType = TYPE.ERROR, message = "Error updating color") => {
        try {
            isUpdatingFromInput = true;
            colorPicker.color.set(color);
            isUpdatingFromInput = false;

            if(isRGB(color)) {
                color = rgbToHex(color);
            }
            colorCodeInput.value = color;
            set_color_button.disabled = false;
            set_color_button.classList.remove('disabled-button');
            set_color_button.style.borderColor = color;
            set_color_button.style.boxShadow = `0 0 10px ${color}`;
            color_picker_element.style.boxShadow = `0 0 50px ${color}`;
        } catch (error) {
            isUpdatingFromInput = false;
            console.log(error);
            message_pop_up(errorType, message);
            set_color_button.disabled = true;
            set_color_button.classList.add('disabled-button');
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

        setServerLEDSettings();
    }

    /**
     * Runs oninput of adding a new color to the users "easy colors" section
     */
    window.checkUserColor = function() {
        const userColorInput = document.getElementById("add-user-color-input");
        const saveButton = document.getElementById("color-add-button");
        const warningIcon = document.getElementById("add-color-input-warning");

        if(userColorInput.value.length <= 0 || !isHex(userColorInput.value)) {
            saveButton.classList.add("grayed-out");
            saveButton.disabled = true;
            warningIcon.style.color = COLORS[TYPE.WARNING]
            return
        }


        saveButton.classList.remove("grayed-out");
        saveButton.disabled = false;
        warningIcon.style.color = "transparent"

    }

    function isHex(color) {
        // Regular expression for 3 or 6 digit hex color codes
        const hexRegex = /^#([0-9A-Fa-f]{3}){1,2}$/;
        return hexRegex.test(color);
    }

    window.addCurrentColor = () => {
        if(currentColor) {
                userColors.push(currentColor);
                updateUserColors(userColors);
                setServerUserColors(userColors);
                hideAddColorOverlay();
        }
    }

});
