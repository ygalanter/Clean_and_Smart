var version = '2.30.0';
var current_settings;

/*  ****************************************** Weather Section **************************************************** */


// converts open-metri weather icon code to Yahoo weather icon code (to reuse current bitmap with icon set)
var OpenMetroCodeToYahooIcon = function (weather_code, is_day) {
  var yahoo_icon = 3200; //initially not defined

  if (weather_code === 0) {
    yahoo_icon = is_day === 1 ? 32 : 31; // sunny or clear night
  } else if ([51, 53, 55, 61, 63, 65, 80, 81, 82, 95, 96, 99].includes(weather_code)) {
    yahoo_icon = 11; //showers
  } else if ([71, 73, 75, 77, 85, 86].includes(weather_code)) {
    yahoo_icon = 16; //snow
  } else if ([56, 57, 66, 67].includes(weather_code)) {
    yahoo_icon = 18; //sleet
  } else if ([45, 48].includes(weather_code)) {
    yahoo_icon = 20; //foggy
  } else if (weather_code === 3) {
    yahoo_icon = 26; //cloudy
  } else if ([1, 2].includes(weather_code)) {
    yahoo_icon = is_day === 1 ? 30 : 29; //partly cloudy day or night
  }

  return yahoo_icon;

};


//2016-03-25: Updated for Forecast.io
function getWeather(coords) {

  var temperature;
  var code;
  var is_day;


  var url = `https://api.open-meteo.com/v1/forecast?latitude=${coords.latitude}&longitude=${coords.longitude}&current=temperature_2m,weather_code,is_day&temperature_unit=${current_settings.temperatureFormat === 0 ? 'fahrenheit' : 'celsius'}`;
  //console.log ("++++ I am inside of 'getWeather()' preparing url:" + url);

  // ** Send request to Yahoo
  //Send request to Forecast.io
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {

    //console.log  ("++++ I am inside of 'getWeather()' callback. responseText is " + this.responseText);

    var json = JSON.parse(this.responseText);

    temperature = json.current.temperature_2m;
    console.log("++++ I am inside of 'getWeather()' callback. Temperature is " + temperature);

    code = json.current.weather_code;
    console.log("++++ I am inside of 'getWeather()' callback. Icon code: " + code);

    is_day = json.current.is_day;
    console.log("++++ I am inside of 'getWeather()' callback. Is day: " + is_day);

    var dictionary = {
      'KEY_WEATHER_CODE': OpenMetroCodeToYahooIcon(code, is_day),
      'KEY_WEATHER_TEMP': temperature
    };

    // Send to Pebble
    //console.log  ("++++ I am inside of 'getWeather()' callback. About to send message to Pebble");
    Pebble.sendAppMessage(dictionary,
      function (e) {
        //console.log ("++++ I am inside of 'Pebble.sendAppMessage()' callback. Weather info sent to Pebble successfully!");
      },
      function (e) {
        //console.log ("++++ I am inside of 'Pebble.sendAppMessage()' callback. Error sending weather info to Pebble!");
      }
    );
  };

  xhr.onerror = function (e) {
    //console.log("I am inside of 'getWeather()' ERROR: " + e.error);
  };

  xhr.open('GET', url);
  xhr.send();
}



// on location success querying woeid and getting weather
function locationSuccess(pos) {

  getWeather(pos.coords);

}




function locationError(err) {
  //console.log ("++++ I am inside of 'locationError: Error requesting location!");
}


// Get Location lat+lon
function getLocation() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    { timeout: 15000, maximumAge: 60000 }
  );
}


// Listen for when the watchface is opened
Pebble.addEventListener('ready',
  function (e) {

    //reading current stored settings
    try {
      current_settings = JSON.parse(localStorage.getItem('current_settings'));
    } catch (ex) {
      current_settings = null;
    }

    if (current_settings === null) {
      current_settings = {
        temperatureFormat: 0,
        hoursMinutesSeparator: 0,
        dateFormat: 0,
        invertColors: 0,
        bluetoothAlert: 0, // new 2.18
        locationService: 0,
        woeid: version >= '2.22' ? '' : 0,
        language: 255,
        forecastIoApiKey: ''
      };
    }

    //console.log ("++++ I am inside of 'Pebble.addEventListener('ready'): PebbleKit JS ready!");
    var dictionary = {
      "KEY_JSREADY": 1
    };

    // Send to Pebble, so we can load units variable and send it back
    //console.log ("++++ I am inside of 'Pebble.addEventListener('ready') about to send Ready message to phone");
    Pebble.sendAppMessage(dictionary,
      function (e) {
        //console.log ("++++ I am inside of 'Pebble.sendAppMessage() callback: Ready notice sent to phone successfully!");
      },
      function (e) {
        //console.log ("++++ I am inside of 'Pebble.sendAppMessage() callback: Error ready notice to Pebble!");
      }
    );
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function (e) {
    console.log("++++ I am inside of 'Pebble.addEventListener('appmessage'): AppMessage received");

    console.log("++++ I am inside of 'Pebble.addEventListener('appmessage'): Requesting automatic location");
    getLocation();  // for automatic location - get location

  }
);

/*    ******************************************************************** Config Section ****************************************** */

Pebble.addEventListener("showConfiguration",
  function (e) {

    //Load the remote config page

    //Pebble.openURL("http://codecorner.galanter.net/pebble/clean_smart_config.htm?version=" + version);
    Pebble.openURL("http://ygalanter.github.io/configs/clean_smart/clean_smart_config.htm?version=" + version); //YG 2016-03-15; moved config to github hosting

  }
);

Pebble.addEventListener("webviewclosed",
  function (e) {

    if (e.response !== '') {

      //console.log ("++++ I am inside of 'Pebble.addEventListener(webviewclosed). Resonse from WebView: " + decodeURIComponent(e.response));

      //Get JSON dictionary
      var settings = JSON.parse(decodeURIComponent(e.response));

      var app_message_json = {};

      // preparing app message
      app_message_json.KEY_HOURS_MINUTES_SEPARATOR = settings.hoursMinutesSeparator;
      app_message_json.KEY_DATE_FORMAT = settings.dateFormat;
      app_message_json.KEY_INVERT_COLORS = settings.invertColors;
      app_message_json.KEY_BLUETOOTH_ALERT = settings.bluetoothAlert; // new 2.18
      app_message_json.KEY_LOCATION_SERVICE = settings.locationService;
      app_message_json.KEY_WEATHER_INTERVAL = settings.weatherInterval;
      app_message_json.KEY_LANGUAGE = settings.language;

      // only storing and passing to pebble temperature format if it changed, because it will cause Pebble to reissue weather AJAX
      // (or if forecast.io API Key was set/changed - then we need to update weather as well)
      // (or if coordinates (former woeid) changed - then we need to update weather as well)
      if (current_settings.temperatureFormat != settings.temperatureFormat ||
        current_settings.forecastIoApiKey != settings.forecastIoApiKey ||
        current_settings.woeid != settings.woeid) {
        app_message_json.KEY_TEMPERATURE_FORMAT = settings.temperatureFormat;
      }

      // storing new settings
      localStorage.setItem('current_settings', JSON.stringify(settings));
      current_settings = settings;

      //console.log ("++++ I am inside of 'Pebble.addEventListener(webviewclosed). About to send settings to the phone");
      Pebble.sendAppMessage(app_message_json,
        function (e) {
          //console.log ("++++ I am inside of 'Pebble.addEventListener(webviewclosed) callback' Data sent to phone successfully!");
        },
        function (e) {
          //console.log ("++++ I am inside of 'Pebble.addEventListener(webviewclosed) callback' Data sent to phone failed!");
        }
      );
    }
  }
);