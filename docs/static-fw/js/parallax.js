var onReady = function() {
  var speed = 0.5;
  var parallaxElements = document.getElementsByClassName("parallax");
  window.addEventListener("scroll", function() {
    parallax(parallaxElements);
  });
  parallax(parallaxElements);

  function parallax(elements) {
    Array.prototype.forEach.call(elements, function(element) {
      var delta = window.scrollY; // window.innerHeight - element.getBoundingClientRect().top;
      var limit = element.offsetTop + element.offsetHeight;
      element.style.backgroundPositionY = (50 * delta / limit) + "%";
    });
  }
};

if (document.readState === "complete" ||
    document.readyState !== "loading" &&
   !document.documentElement.doScroll)
  onReady();
else
  document.addEventListener("DOMContentLoaded", onReady);

