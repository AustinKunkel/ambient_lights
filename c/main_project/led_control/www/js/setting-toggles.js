document.addEventListener("DOMContentLoaded", () => {
  const offsetToggleIcon = document.getElementById("offset-icon");
  const offsetDescription = document.getElementById("offset-description");
  const offsetUpIcon = document.getElementById("offset-up-icon");

  offsetToggleIcon.addEventListener("click", () => {
    offsetDescription.classList.toggle("expanded");
  });

  offsetUpIcon.addEventListener("click", () => {
    offsetDescription.classList.toggle("expanded");
  })
});
