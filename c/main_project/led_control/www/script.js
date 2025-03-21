function showMessage() {
  alert("Hello from MicroHTTPD!");
}

fetch("/data.json", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify({ message: "Hello, server!" })
})
.then(response => response.text())
.then(data => console.log(data));

fetch("/data.json", { method: "DELETE" })
.then(response => response.text())
.then(data => console.log(data));
