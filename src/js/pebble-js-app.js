var initialized = false;

Pebble.addEventListener("ready", function() {
  console.log("ready called!");
  initialized = true;
});

Pebble.addEventListener("showConfiguration", function() {
  console.log("showing configuration");
  Pebble.openURL('https://rjw57.github.io/pebble-clubcard/configuration.html');
});

Pebble.addEventListener("webviewclosed", function(e) {
  console.log("configuration closed");
  console.log('response: ' + e.response);
  // webview closed
  var options = JSON.parse(decodeURIComponent(e.response));
  console.log("Options = " + JSON.stringify(options));
  if(options.accountNumber !== undefined) {
      console.log('sending new account number "' + options.accountNumber + '"');
      Pebble.sendAppMessage({ 'accountNumber': options.accountNumber });
  }
});
