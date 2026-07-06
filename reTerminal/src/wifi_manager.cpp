#include "wifi_manager.h"
#include "config.h"

// ----- Webserver -----

static const char indexHtml[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8">
<title>reTerminal - Setup</title>
<style>
body{font-family:sans-serif;max-width:80%;margin:1em auto;padding:0 1em;font-family:Tahoma,Verdana,Arial,sans-serif;background:#444;color:#ddd;}
h1{font-size:1.4em;border-bottom:2px solid #444;padding-bottom:.2em}
fieldset{margin:1em 0;padding:1em;border:1px solid #ccc;border-radius:6px}
legend{font-weight:bold;padding:0 .4em}
input[type=text],input[type=password],input[type=number]{width:100%;padding:.5em;font-size:1em;box-sizing:border-box}
label{display:block;margin:.5em 0 .2em}
button{padding:.6em 1.2em;font-size:1em;margin-top:.6em;cursor:pointer}
.ok{color:#080}.err{color:#a00}
</style>
<script src="https://d3js.org/d3.v4.js"></script>
</head><body>

<h1>reTerminal E1001 — Dashboard</h1>

<h2>Temperature in °C</h2>
<div id="temperature"></div>

<script>

// parse the date / time
var parseTime = d3.timeParse("%d-%m-%Y_%H:%M:%S");

// set the dimensions and margins of the graph
var margin = {top: 10, right: 30, bottom: 30, left: 60},
    width1 = 800 - margin.left - margin.right,
    height1 = 300 - margin.top - margin.bottom;

// set the ranges
var x1 = d3.scaleTime().range([0, width1]);
var y1 = d3.scaleLinear().range([height1, 0]);

// define the 1st line
var valueline1 = d3.line()
    .x(function(d) { return x1(d.date); })
    .y(function(d) { return y1(d.temperature); });

// append the svg obgect to the the page
var svg1 = d3.select("#temperature")
  .append("svg")
    .attr("width", width1 + margin.left + margin.right)
    .attr("height", height1 + margin.top + margin.bottom)
  .append("g")
    .attr("transform",
          "translate(" + margin.left + "," + margin.top + ")");

//##### Get the data #####

d3.csv("weather/temperature.csv", function(error, data) {
  if (error) throw error;

  // format the data
  data.forEach(function(d) {
      d.date = parseTime(d.date);
      d.temperature = +d.temperature;
  });

//##### plot#####

  // Scale the range of the data
  x1.domain(d3.extent(data, function(d) { return d.date; }));
  y1.domain([d3.min(data, function(d) {
    return Math.min(d.temperature); })
    ,d3.max(data, function(d) {
    return Math.max(d.temperature); })]);

  // Add the valueline path.
  svg1.append("path")
      .data([data])
      .attr("fill", "none")
      .attr("class", "line")
      .style("stroke", "red")
      .attr("d", valueline1);

  // Add the X Axis
  svg1.append("g")
      .attr("transform", "translate(0," + height1 + ")")
      .call(d3.axisBottom(x1));

  // Add the Y Axis
  svg1.append("g")
      .call(d3.axisLeft(y1));

});

</script>

<h2>Humidity in %</h2>
<div id="humidity"></div>

<script>

// parse the date / time
var parseTime = d3.timeParse("%d-%m-%Y_%H:%M:%S");

// set the dimensions and margins of the graph
var margin = {top: 10, right: 30, bottom: 30, left: 60},
    width1 = 800 - margin.left - margin.right,
    height1 = 300 - margin.top - margin.bottom;

// set the ranges
var x2 = d3.scaleTime().range([0, width1]);
var y2 = d3.scaleLinear().range([height1, 0]);

// define the 1st line
var valueline2 = d3.line()
    .x(function(d) { return x2(d.date); })
    .y(function(d) { return y2(d.humidity); });

// append the svg obgect to the the page
var svg2 = d3.select("#humidity")
  .append("svg")
    .attr("width", width1 + margin.left + margin.right)
    .attr("height", height1 + margin.top + margin.bottom)
  .append("g")
    .attr("transform",
          "translate(" + margin.left + "," + margin.top + ")");

//##### Get the data #####

d3.csv("weather/humidity.csv", function(error, data) {
  if (error) throw error;

  // format the data
  data.forEach(function(d) {
      d.date = parseTime(d.date);
      d.humidity = +d.humidity;
  });

//##### plot#####

  // Scale the range of the data
  x2.domain(d3.extent(data, function(d) { return d.date; }));
  y2.domain([0,100]);

  // Add the valueline path.
  svg2.append("path")
      .data([data])
      .attr("fill", "none")
      .attr("class", "line")
      .style("stroke", "blue")
      .attr("d", valueline2);

  // Add the X Axis
  svg2.append("g")
      .attr("transform", "translate(0," + height1 + ")")
      .call(d3.axisBottom(x2));

  // Add the Y Axis
  svg2.append("g")
      .call(d3.axisLeft(y2));

});

</script>

<h2>Pressure in mBar</h2>
<div id="pressure"></div>

<script>

<p><a href="/setup">go to setup</a></p>
</body></html>
)HTML";

static const char configHtml[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8">
<title>reTerminal - Setup</title>
<style>
body{font-family:sans-serif;max-width:80%;margin:1em auto;padding:0 1em;font-family:Tahoma,Verdana,Arial,sans-serif;background:#444;color:#ddd;}
h1{font-size:1.4em;border-bottom:2px solid #444;padding-bottom:.2em}
fieldset{margin:1em 0;padding:1em;border:1px solid #ccc;border-radius:6px}
legend{font-weight:bold;padding:0 .4em}
input[type=text],input[type=password],input[type=number]{width:100%;padding:.5em;font-size:1em;box-sizing:border-box}
label{display:block;margin:.5em 0 .2em}
button{padding:.6em 1.2em;font-size:1em;margin-top:.6em;cursor:pointer}
.ok{color:#080}.err{color:#a00}
</style>
<script src="https://d3js.org/d3.v4.js"></script>
</head><body>

<h1>reTerminal E1001 — Setup</h1>

<fieldset>
<legend>Update Wi-Fi credentials</legend>
<form method="POST" action="/config">
<label>SSID</label>       <input type="text"     name="ssid" required>
<label>Password</label>   <input type="password" name="pass">
<label>Latitude</label>   <input type="number"   name="lat" step="0.001" value="+std::to_string(nvsForecastLatitude)+"> TODO:fix including coord
<label>Longitude</label>  <input type="number"   name="lon" step="0.001" value="+std::to_string(nvsForecastLongitude)+"> TODO:fix including coord
<label>Master Key</label> <input type="password" name="masterKey" required>
<button type="submit">save</button>
</form>
</fieldset>

<p><a href="/setup">go to setup</a></p>
</body></html>
)HTML";

// ---- handlers ----

void WifiManager::handleIndex()
{
    if (_server) {
        _server->send(200, "text/html", indexHtml);
    } else {
        LOG.println("[WiFi] ERROR: AP server not running");
        requestOff();
    }
}

void WifiManager::handleSetup()
{
    if (_server) {
        _server->send(200, "text/html", configHtml);
    } else {
        LOG.println("[WiFi] ERROR: AP server not running");
        requestOff();
    }
}

void WifiManager::handleIngest()
{
    if (_server) {
        float temp = _server->hasArg("temp")  ? _server->arg("temp").toFloat()  : NAN;
        float hum  = _server->hasArg("hum")   ? _server->arg("hum").toFloat()   : NAN;
        float pres = _server->hasArg("press") ? _server->arg("press").toFloat() : NAN;
        float lux  = _server->hasArg("lux")   ? _server->arg("lux").toFloat()   : NAN;
        float volt = _server->hasArg("volt")  ? _server->arg("volt").toFloat()  : NAN;

        String resp = "{\"ok\":true}";
        _server->send(200, "application/json", resp);

        //TODO: save reading

    } else {
        LOG.println("[WiFi] ERROR: AP server not running");
        requestOff();
    }
}

void WifiManager::handleConfig()
{
    if (_server) {
        if (!_server->hasArg("masterKey") || _server->arg("masterKey") != AP_MASTER_KEY) {
            _server->send(401, "text/plain", "wrong Master Key");
            return;
        }

        const char* ssid = _server->arg("ssid").c_str();
        const char* pass = _server->arg("pass").c_str();
        float lat = std::stof(std::string(_server->arg("lat").c_str()));
        float lon = std::stof(std::string(_server->arg("lon").c_str()));

        saveNvsCredentials(std::string(ssid), std::string(pass), lat, lon);
        
        if (strlen(ssid) != 0 && strlen(pass) != 0) {
            requestOff();
            requestSta(ssid, pass);
        }
    } else {
        LOG.println("[WiFi] ERROR: AP server not running");
        requestOff();
    }
}

void WifiManager::handleNotFound()
{
    if (_server) {
        _server->send(404, "text/plain", "not found");
    } else {
        LOG.println("[WiFi] ERROR: AP server not running");
        requestOff();
    }
}

// ---- WifiManager ----

void WifiManager::begin()
{
    WiFi.mode(WIFI_OFF);
    _initTime = (time_t)(-1);
    _state = WifiState::Off;
    _server = nullptr;
}

void WifiManager::requestOff()
{
    if (_state == WifiState::ApActive) {
        if (_server) {
            _server->stop();
            _server = nullptr;
        }
        WiFi.softAPdisconnect(true);
        LOG.printf("[WiFi] AP disconnected.\n");
    } else if (_state == WifiState::StaActive) {
        WiFi.disconnect();
        LOG.printf("[WiFi] STA disconnected.\n");
    }
    WiFi.mode(WIFI_OFF);
    _initTime = (time_t)(-1);
    _state = WifiState::Off;
    LOG.printf("[WiFi] disabled\n");
}

void WifiManager::requestSta(const char *ssid, const char *pass)
{
    if (state() == WifiState::StaActive) {
        LOG.printf("[WiFi] already in STA mode\n");
        return;
    } else {
        requestOff();
    }
    LOG.printf("[WiFi] requesting STA\n");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    
    wl_status_t wifiStatus;
    while (true) {
        wifiStatus = WiFi.status();
        if (wifiStatus == WL_CONNECTED) {
            _initTime = millis();
            _state = WifiState::StaActive;
            LOG.printf("[WiFi] STA up: ip=%s\n", WiFi.softAPIP().toString().c_str());
            break;
        } else if ((millis() - _initTime) >= STA_SETUP_TIMEOUT_MS ||
                    wifiStatus == WL_CONNECT_FAILED ||
                    wifiStatus == WL_NO_SSID_AVAIL) {
            LOG.println("[WiFi] STA timeout/failed");
            requestOff();
            break;
        }
        delay(100);
        yield();
        LOG.printf(".");
    }
}

void WifiManager::requestAp(const char *ssid, const char *pass)
{
    if (state() == WifiState::ApActive) {
        LOG.printf("[WiFi] already in STA mode\n");
        return;
    } else {
        requestOff();
    }
    LOG.printf("[WiFi] requesting AP\n");
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(ssid, pass, AP_CHANNEL)) {
        log_e("[WiFi] AP creation failed.");
        requestOff();
        return;
    }

    static WebServer server(80);
    _server = &server;
    _server->on("/",        HTTP_GET,  [this]() { handleIndex(); });
    _server->on("/ingest",  HTTP_POST, [this]() { handleIngest(); });
    _server->on("/setup",   HTTP_POST, [this]() { handleSetup(); });
    _server->on("/config",  HTTP_POST, [this]() { handleConfig(); });
    _server->onNotFound([this]() { handleNotFound(); });
    _server->begin();

    _initTime = millis();
    _state = WifiState::ApActive;
    LOG.printf("[WiFi] AP up: %s\n", WiFi.localIP().toString().c_str());
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("[WiFi] AP IP address:");
    Serial.println(myIP);
}

bool WifiManager::checkConnection()
{
    if (WifiState::StaActive == _state) {
        if (WiFi.status() != WL_CONNECTED) {
            LOG.println("[WiFi] STA lost link");
            requestOff();
            return false;
        } else {
            LOG.println("[WiFi] STA still connected");
            return true;
        }
    }
    return false;
}

void WifiManager::servePage() {
    if (_state == WifiState::ApActive) {
        if (_server) {
            _server->handleClient();
        } else {
            LOG.println("[WiFi] ERROR: AP server not running");
            requestOff();
        }
    }
}
