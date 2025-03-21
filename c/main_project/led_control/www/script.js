function showMessage() {
  alert("Hello from MicroHTTPD!");
}

function sendGet() {
  fetch("/get", {
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
