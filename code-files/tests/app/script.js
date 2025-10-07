// script.js

/**
 * Returns a greeting string.
 * @param {string} name
 * @returns {string}
 */
function greet(name) {
    return `Hello, ${name}!`;
  }
  
  document.addEventListener("DOMContentLoaded", () => {
    // Create a <p> element with a greeting
    const p = document.createElement("p");
    p.textContent = greet("World");
    document.body.appendChild(p);
  
    // Log to console as well
    console.log(p.textContent);
  });
  