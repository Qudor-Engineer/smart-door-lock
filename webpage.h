#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

// --- Main Keypad Page ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Smart Room Access</title>
  <style>
    :root {
      --bg: #FFFFFF;
      --surface: #EEFDFF;
      --primary: #004AFF;
      --secondary: #2E85FF;
      --accent: #FFFA32;
      --error: #D32F2F;
      --success: #2E7D32;
    }
    body {
      margin: 0; padding: 20px;
      font-family: 'Segoe UI', Roboto, sans-serif;
      background-color: var(--bg);
      display: flex; flex-direction: column; align-items: center; min-height: 100vh;
    }
    h2 { margin-top: 10px; margin-bottom: 30px; letter-spacing: 2px; color: var(--primary); }
    .container {
      background: var(--surface); padding: 25px; border-radius: 20px;
      box-shadow: 0 10px 30px rgba(0, 74, 255, 0.15); width: 100%; max-width: 320px; text-align: center; box-sizing: border-box;
    }
    .display {
      background: #FFFFFF; border: 2px solid var(--primary); color: var(--primary);
      border-radius: 10px; padding: 15px; font-size: 28px; font-weight: bold;
      letter-spacing: 10px; margin-bottom: 20px; min-height: 70px;
      display: flex; align-items: center; justify-content: center; box-sizing: border-box;
    }
    .keypad { display: grid; grid-template-columns: repeat(3, 1fr); gap: 15px; margin-bottom: 20px; }
    .btn {
      background: #FFFFFF; border: 2px solid var(--primary); color: var(--primary);
      font-size: 24px; font-weight: bold; padding: 20px 0; border-radius: 50%;
      cursor: pointer; transition: all 0.1s; user-select: none;
    }
    .btn:active { background: var(--primary); color: #FFFFFF; }
    .btn-action { font-size: 18px; border-radius: 15px; }
    .btn-unlock { background: var(--primary); color: #FFFFFF; }
    .btn-unlock:active { background: var(--secondary); border-color: var(--secondary); }
    .divider { height: 2px; background: rgba(0, 74, 255, 0.15); margin: 20px 0; }
    
    .btn-light {
      width: 100%; background: var(--accent); border: 2px solid var(--primary); color: var(--primary);
      font-size: 18px; padding: 14px; border-radius: 15px; text-transform: uppercase;
      letter-spacing: 1px; font-weight: bold; transition: all 0.1s; cursor: pointer; box-sizing: border-box;
      margin-bottom: 12px;
    }
    .btn-light:active { transform: scale(0.97); background: #EBE52A; }
    
    .btn-wifi {
      width: 100%; background: #FFFFFF; border: 2px solid var(--primary); color: var(--primary);
      font-size: 15px; padding: 10px; border-radius: 15px; font-weight: 600;
      transition: all 0.1s; cursor: pointer; box-sizing: border-box;
    }
    .btn-wifi:active { background: #E5EFFF; }

    #status { margin-top: 20px; font-size: 16px; font-weight: bold; min-height: 24px; color: var(--primary); }
  </style>
</head>
<body>
  <h2>SMART ROOM</h2>
  <div class="container">
    <div class="display" id="display"></div>
    <div class="keypad">
      <button class="btn" onclick="addNum(1)">1</button>
      <button class="btn" onclick="addNum(2)">2</button>
      <button class="btn" onclick="addNum(3)">3</button>
      <button class="btn" onclick="addNum(4)">4</button>
      <button class="btn" onclick="addNum(5)">5</button>
      <button class="btn" onclick="addNum(6)">6</button>
      <button class="btn" onclick="addNum(7)">7</button>
      <button class="btn" onclick="addNum(8)">8</button>
      <button class="btn" onclick="addNum(9)">9</button>
      <button class="btn btn-action" onclick="clearPin()">CLR</button>
      <button class="btn" onclick="addNum(0)">0</button>
      <button class="btn btn-action btn-unlock" onclick="unlock()">🔑</button>
    </div>
    <div class="divider"></div>
    <button class="btn-light" onclick="toggleLight()">💡 Toggle Light</button>
    <button class="btn-wifi" onclick="window.location.href='http://192.168.4.1/wifi'">⚙️ Network Settings</button>
    <div id="status">System Ready</div>
  </div>
  <script>
    let currentPin = '';
    const display = document.getElementById('display');
    const status = document.getElementById('status');
    function updateDisplay() { display.innerText = '*'.repeat(currentPin.length); }
    function addNum(num) { if (currentPin.length < 8) { currentPin += num; updateDisplay(); } }
    function clearPin() { currentPin = ''; updateDisplay(); status.innerText = 'System Ready'; status.style.color = 'var(--primary)'; }
    function setStatus(msg, color) { status.style.color = color; status.innerText = msg; }
    
    function unlock() {
      if(currentPin.length === 0) return;
      setStatus('Verifying...', 'var(--primary)');
      fetch('/unlock', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'pin=' + currentPin
      })
      .then(response => response.text().then(text => {
        if(response.ok) { setStatus(text, 'var(--success)'); setTimeout(clearPin, 2500); }
        else { setStatus(text, 'var(--error)'); currentPin = ''; updateDisplay(); }
      }))
      .catch(() => setStatus('Connection Error', 'var(--error)'));
    }

    function toggleLight() {
      setStatus('Toggling light...', 'var(--primary)');
      fetch('/light/toggle', { method: 'POST' })
      .then(response => {
        if(response.ok) { response.text().then(text => setStatus(text, 'var(--primary)')); }
        else { setStatus('Server Error', 'var(--error)'); }
      })
      .catch(() => setStatus('Connection Error', 'var(--error)'));
    }
  </script>
</body>
</html>
)rawliteral";


// --- Captive Portal Wi-Fi Configuration Page ---
const char wifi_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Wi-Fi Configuration</title>
  <style>
    :root {
      --bg: #FFFFFF;
      --surface: #EEFDFF;
      --primary: #004AFF;
    }
    body {
      margin: 0; padding: 20px;
      font-family: 'Segoe UI', Roboto, sans-serif;
      background-color: var(--bg);
      display: flex; flex-direction: column; align-items: center; min-height: 100vh;
    }
    h2 { margin-top: 10px; margin-bottom: 10px; color: var(--primary); letter-spacing: 1px; }
    p { color: #555; font-size: 14px; margin-bottom: 25px; text-align: center; max-width: 280px; line-height: 1.4; }
    .container {
      background: var(--surface); padding: 30px 25px; border-radius: 20px;
      box-shadow: 0 10px 30px rgba(0, 74, 255, 0.15); width: 100%; max-width: 320px; box-sizing: border-box;
    }
    .form-group { margin-bottom: 20px; text-align: left; }
    label { display: block; font-weight: bold; margin-bottom: 8px; color: var(--primary); font-size: 14px; }
    select, input[type="text"], input[type="password"] {
      width: 100%; padding: 12px; border: 2px solid var(--primary); background: #FFFFFF;
      color: #1A1A1A; border-radius: 10px; font-size: 15px; font-weight: 500; box-sizing: border-box; outline: none;
    }
    select { appearance: none; background-image: url("data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10' viewBox='0 0 10 10'><path fill='%23004AFF' d='M0 3l5 5 5-5z'/></svg>"); background-repeat: no-repeat; background-position: right 12px center; padding-right: 30px; }
    input:focus, select:focus { box-shadow: 0 0 8px rgba(0, 74, 255, 0.3); }
    .btn-save {
      width: 100%; background: var(--primary); border: none; color: #FFFFFF;
      font-size: 18px; padding: 14px; border-radius: 10px; font-weight: bold;
      cursor: pointer; transition: all 0.1s; text-transform: uppercase; letter-spacing: 1px; margin-top: 10px;
    }
    .btn-save:active { transform: scale(0.98); background: #0036B3; }
    #msg { margin-top: 15px; font-weight: bold; text-align: center; color: #2E7D32; font-size: 14px; }
    .divider { height: 1px; background: rgba(0, 74, 255, 0.15); margin: 20px 0; }
  </style>
</head>
<body>
  <h2>Network Setup</h2>
  <p>Select a nearby network or type it manually to connect the door lock.</p>
  <div class="container">
    
    <div class="form-group">
      <label for="network-list">Detected Networks</label>
      <select id="network-list" onchange="updateSSIDField(this.value)">
        <option value="">Scanning for networks...</option>
      </select>
    </div>

    <div class="divider"></div>

    <form id="wifiForm" onsubmit="saveConfig(event)">
      <div class="form-group">
        <label for="ssid">Wi-Fi Name (SSID)</label>
        <input type="text" id="ssid" required placeholder="e.g. Home_Network">
      </div>
      <div class="form-group">
        <label for="pass">Wi-Fi Password</label>
        <input type="password" id="pass" required placeholder="••••••••">
      </div>
      <button type="submit" class="btn-save">Save Settings</button>
    </form>
    <div id="msg"></div>
  </div>
  <script>
    window.onload = function() {
      fetch('/scan')
        .then(res => res.json())
        .then(networks => {
          const dropdown = document.getElementById('network-list');
          dropdown.innerHTML = '<option value="">-- Choose a Network --</option>';
          if(networks.length === 0) {
            dropdown.innerHTML = '<option value="">No networks detected</option>';
            return;
          }
          networks.forEach(ssid => {
            const opt = document.createElement('option');
            opt.value = ssid;
            opt.innerText = ssid;
            dropdown.appendChild(opt);
          });
        })
        .catch(() => {
          document.getElementById('network-list').innerHTML = '<option value="">Failed to load networks</option>';
        });
    };

    function updateSSIDField(val) {
      if(val) {
        document.getElementById('ssid').value = val;
      }
    }

    function saveConfig(e) {
      e.preventDefault();
      const ssid = document.getElementById('ssid').value;
      const pass = document.getElementById('pass').value;
      const msgDiv = document.getElementById('msg');
      msgDiv.innerText = "Saving configuration...";
      
      fetch('/save-wifi', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass)
      })
      .then(response => response.text().then(text => {
        msgDiv.innerText = text;
        if(response.ok) {
          document.getElementById('wifiForm').reset();
          setTimeout(() => { msgDiv.innerText = "System rebooting... reconnecting to your network."; }, 2000);
        }
      }))
      .catch(() => { msgDiv.innerText = "Error sending configurations."; msgDiv.style.color = "#D32F2F"; });
    }
  </script>
</body>
</html>
)rawliteral";

#endif