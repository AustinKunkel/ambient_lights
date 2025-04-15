function showMessage() {
  alert("Hello from MicroHTTPD!");
}

function sendGet() {
  fetch("/api", {
    method: "GET",
    headers: { "Content-Type": "application/json" }
  })
  .then(response => response.text())
  .then(data => console.log(data));
}

function sendPost() {
  fetch("/api", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ message: "Hello, server!" })
  })
  .then(response => response.text())
  .then(data => console.log(data));
}


function sendDelete() {
  fetch("/api", { method: "DELETE" })
  .then(response => response.text())
  .then(data => console.log(data));
}

document.addEventListener('DOMContentLoaded', () => {
  const brightnessSlider = document.getElementById('brightness-slider');
  const brightnessValue = document.getElementById('brightness-value');

  // Update the displayed value as the slider is adjusted
  brightnessSlider.addEventListener('input', () => {
      brightnessValue.textContent = brightnessSlider.value;
  });

  // Send the brightness value as JSON when the slider is released
  brightnessSlider.addEventListener('change', () => {
      const brightness = brightnessSlider.value;

      // Prepare the data to be sent in JSON format
      const data = {
          'brightness': brightness,
          'color': '#FFFFFF',
          'capture_screen': 0,
          'sound_react': 0,
          'fx_num': 0,
          'count': 206,
          'id': 2
      };

      console.log(JSON.stringify(data));

      // Send the data using the Fetch API
      fetch('/led-settings', {
          method: 'POST',
          headers: {
              'Content-Type': 'application/json',
          },
          body: JSON.stringify(data),  // Send the data as a JSON string
      })
      .then(response => response.json())  // Assuming your server returns a JSON response
      .then(data => {
          console.log('Success:', data);
      })
      .catch(error => {
          console.error('Error:', error);
      });
  });
});
