var version = '2.24';
var current_settings;

/*  ****************************************** Weather Section **************************************************** */


// converts forecast.io weather icon code to Yahoo weather icon code (to reuse current bitmap with icon set)
  var ForecastIoIconToYahooIcon = function(forecsat_io_icon) {
    var yahoo_icon = 3200; //initialy not defined
    
    switch (forecsat_io_icon){
      case "clear-day":
        yahoo_icon = 32; // sunny
        break;
      case "clear-night":
        yahoo_icon = 31; // clear night
        break;
      case "rain":
        yahoo_icon = 11; // showers
        break;
      case "snow":
        yahoo_icon = 16; // snow
        break;
      case "sleet": 
        yahoo_icon = 18; // sleet
        break;
      case "wind": 
        yahoo_icon = 24; // windy
        break;
      case "fog": 
        yahoo_icon = 20; // foggy
        break;
      case "cloudy":
        yahoo_icon = 26; // cloudy
        break;
      case "partly-cloudy-day":
        yahoo_icon = 30; // partly cloudy day
        break;
      case "partly-cloudy-night":
        yahoo_icon = 29; // partly cloudy night
        break;
    }
    
    return yahoo_icon;
    
  };


//2016-03-25: Updated for Forecast.io
function getWeather(coords /*woeid*/ ) {  
  
      if (current_settings.forecastIoApiKey === '') {
        //console.log ("\n++++ I am inside of 'getWeather()' API KEY NOT DEFINED");
        return;
      }
  
  var temperature;
  var icon;
  
  
  //* var query = 'select item.condition from weather.forecast where woeid =  ' + woeid + ' and u="' + (current_settings.temperatureFormat === 0? 'f' : 'c') + '"';
  //console.log ("++++ I am inside of 'getWeather()' preparing query:" + query);
  
  //* var url = 'https://query.yahooapis.com/v1/public/yql?q=' + encodeURIComponent(query) + '&format=json&env=store://datatables.org/alltableswithkeys';
  var url = 'https://api.forecast.io/forecast/' + current_settings.forecastIoApiKey + '/' + coords + '?exclude=minutely,hourly,daily,alerts,flags&units=' + (current_settings.temperatureFormat === 0? 'us' : 'si');
  //console.log ("++++ I am inside of 'getWeather()' preparing url:" + url);
  
  // ** Send request to Yahoo
  //Send request to Forecast.io
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    
    //console.log  ("++++ I am inside of 'getWeather()' callback. responseText is " + this.responseText);
    
    var json = JSON.parse(this.responseText);
    
    
    temperature = json.currently.temperature;
    console.log  ("++++ I am inside of 'getWeather()' callback. Temperature is " + temperature);
    
    icon = json.currently.icon;
    console.log  ("++++ I am inside of 'getWeather()' callback. Icon code: " + icon);
   
    
    var dictionary = {
      'KEY_WEATHER_CODE': ForecastIoIconToYahooIcon(icon),
      'KEY_WEATHER_TEMP': temperature
    };
    
    // Send to Pebble
    //console.log  ("++++ I am inside of 'getWeather()' callback. About to send message to Pebble");
    Pebble.sendAppMessage(dictionary,
    function(e) {
      //console.log ("++++ I am inside of 'Pebble.sendAppMessage()' callback. Weather info sent to Pebble successfully!");
    },
    function(e) {
      //console.log ("++++ I am inside of 'Pebble.sendAppMessage()' callback. Error sending weather info to Pebble!");
    }
    );
  };
  
  xhr.onerror = function(e) {
    //console.log("I am inside of 'getWeather()' ERROR: " + e.error);
  };
  
  xhr.open('GET', url);
  xhr.send();
}



// on location success querying woeid and getting weather
function locationSuccess(pos) {
//         // We neeed to get the Yahoo woeid first
//         var woeid;
      
//       /* YG 2016-01-25  !!! This query no longer works due to Yahoo bug. Using the one below it !!!  */  
//       // var query = 'select * from geo.placefinder where text="' +
//       //     pos.coords.latitude + ',' + pos.coords.longitude + '" and gflags="R"';
//          var query = 'select locality1 from geo.places where text="(' + 
//              pos.coords.latitude + ',' + pos.coords.longitude + ')" limit 1';
        
//         //console.log ("++++ I am inside of 'locationSuccess()' preparing query:" + query);
//         var url = 'https://query.yahooapis.com/v1/public/yql?q=' + encodeURIComponent(query) + '&format=json';
//         //console.log ("++++ I am inside of 'locationSuccess()' preparing URL: " + url);
//         // Send request to Yahoo
//         var xhr = new XMLHttpRequest();
//         xhr.onload = function () {
//           var json = JSON.parse(this.responseText);
          
//           /* YG 2016-01-25  !!! This result no longer works due to Yahoo bug. Using the one below it !!!  */  
//           // woeid = json.query.results.Result.woeid;
//           woeid = json.query.results.place.locality1.woeid;
          
//           //console.log ("++++ I am inside of 'locationSuccess()', woeid received:" + woeid);
//           getWeather(woeid);
//         };
//         xhr.open('GET', url);
//         xhr.send();
  
   // requestion weather from forecast.io
   getWeather(pos.coords.latitude + ',' + pos.coords.longitude);

}




function locationError(err) {
  //console.log ("++++ I am inside of 'locationError: Error requesting location!");
}


// Get Location lat+lon
function getLocation() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}


// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
      
    //reading current stored settings
    try {
       current_settings = JSON.parse(localStorage.getItem('current_settings'));
    } catch(ex) {
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
             woeid: version >= '2.22'? '' : 0,
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
      function(e) {
        //console.log ("++++ I am inside of 'Pebble.sendAppMessage() callback: Ready notice sent to phone successfully!");
      },
      function(e) {
        //console.log ("++++ I am inside of 'Pebble.sendAppMessage() callback: Error ready notice to Pebble!");
      }
    ); 
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log ("++++ I am inside of 'Pebble.addEventListener('appmessage'): AppMessage received");
    
    if (current_settings.locationService == 1) { // for manual location - request weather right away
     //***** console.log ("\n++++ I am inside of 'Pebble.addEventListener('appmessage'): Requesting weather by WOEID");
     // console.log ("\n++++ I am inside of 'Pebble.addEventListener('appmessage'): Requesting weather by coords:" + current_settings.woeid);
      getWeather(current_settings.woeid);
    } else {
       console.log ("++++ I am inside of 'Pebble.addEventListener('appmessage'): Requesting automatic location");
       getLocation();  // for automatic location - get location
    }
    
  }                     
);

/*    ******************************************************************** Config Section ****************************************** */ 

Pebble.addEventListener("showConfiguration",
  function(e) {
   
    //Load the remote config page
   
    //Pebble.openURL("http://codecorner.galanter.net/pebble/clean_smart_config.htm?version=" + version);
    Pebble.openURL("http://ygalanter.github.io/configs/clean_smart/clean_smart_config.htm?version=" + version); //YG 2016-03-15; moved config to github hosting
    
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    
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
        function(e) {
          //console.log ("++++ I am inside of 'Pebble.addEventListener(webviewclosed) callback' Data sent to phone successfully!");
        },
        function(e) {
          //console.log ("++++ I am inside of 'Pebble.addEventListener(webviewclosed) callback' Data sent to phone failed!");
        }
      );
    }
  }
);