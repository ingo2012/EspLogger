const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
</head>
<style>
      html {font-family: Arial; display: inline-block; text-align: center;}
      p { font-size: 1.2rem;}
      body {  margin: 0;}
      .topnav { overflow: hidden; background-color: #50B8B4; color: white; font-size: 1rem; }
      .content { padding: 20px; }
      .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
      .cards { max-width: 800px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); }
      .reading { font-size: 1.4rem; }
</style>
<body>
<div class="topnav">
        <h1>DataLogger-D1</h1>
      </div>
      <div class="content">
      <div class="cards">
        <div class="card">
          <p><i class="fas fa-thermometer-half" style="color:#059e8a;"></i> Vorlauf</p><p><span class="reading"><span id="Vorlauf">0</span> &deg;C</span></p>
        </div>
        <div class="card">
          <p><i class="fas fa-tint" style="color:#00add6;"></i> Ruecklauf</p><p><span class="reading"><span id="Rlauf">0</span> &deg;C</span></p>
        </div>
        <div class="card">
          <p><i class="fas fa-angle-double-down" style="color:#e1e437;"></i> Uhrzeit</p><p><span class="reading"><span id="Uhrzeit">0</span></p>
        </div>
        <div class="card">
          <p><i style="color:#e1e437;"></i> Browserzeit</p><p><span class="reading"><span id="Browser">0</span></p>
        </div>
      </div>
</div>
<script>
    
    function addZero(zahl) {
      zahl = (zahl < 10 ? '0' : '' )+ zahl;  
      return zahl;
    }
    function myDate() {
    var jetzt = new Date(),
      h = jetzt.getHours(),
      m = jetzt.getMinutes(),
      s = jetzt.getSeconds();
      m = addZero(m);
      s = addZero(s);
      // return(h+":"+m+":"+s);
      document.getElementById('Browser').innerHTML = h + ':' + m + ':' + s;
      setTimeout(myDate, 500);
    }
    myDate();
    requestData(); // get intial data straight away 
  
    // request data updates every 5000 milliseconds
    setInterval(requestData, 5000);

    function requestData() {
      var xhr = new XMLHttpRequest();
      xhr.open('GET', '/readADC');

      xhr.onload = function() {
        if (xhr.status === 200) {

          if (xhr.responseText) { // if the returned data is not null, update the values

            var data = JSON.parse(xhr.responseText);

            document.getElementById("Vorlauf").innerText = data.vorlauf;
            document.getElementById("Rlauf").innerText = data.rlauf;
            document.getElementById("Uhrzeit").innerText = data.uhrzeit;
            //document.getElementById("Browser").innerText = time;
          } else { // a problem occurred

            document.getElementById("Vorlauf").innerText = "?";
            document.getElementById("Rlauf").innerText = "?";
            document.getElementById("Uhrzeit").innerText = "?";
          }
        } else {
          console.log('Request failed.  Returned status of ' + xhr.status);

          document.getElementById("Vorlauf").innerText = "?";
          document.getElementById("Rlauf").innerText = "?";
          document.getElementById("Uhrzeit").innerText = "?";

        }
      };
      
      xhr.send();
    }
    
  </script>

</body>
</html>
)=====";
