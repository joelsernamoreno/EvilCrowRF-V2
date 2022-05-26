function AutoRefresh(t) {
  setTimeout("window.location.reload(1);", t);
  }

function AutoRedirect() {
  window.setTimeout(function () {
        window.location.href = "/";
    }, 5000)
  }

function updatemenu() {
  if (document.getElementById('responsive-menu').checked == true) {
    document.getElementById('menu').style.borderBottomRightRadius = '0';
    document.getElementById('menu').style.borderBottomLeftRadius = '0';
  }else{
    document.getElementById('menu').style.borderRadius = '25px';
  }
}
