document.addEventListener("DOMContentLoaded", () => {
  const addUserColorButton = document.getElementById("add-user-color");
  addUserColorButton.addEventListener("click", openAddColorMenu);

  loadUserColors();
});

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