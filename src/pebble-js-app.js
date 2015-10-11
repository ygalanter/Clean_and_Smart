var version = '1.04';
var current_settings;

/*  ****************************************** Weather Section **************************************************** */

function getWeather(woeid) {  
  
  var temperature;
  var icon;
  
  
  var query = 'select item.condition from weather.forecast where woeid =  ' + woeid + ' and u="' + (current_settings.temperatureFormat === 0? 'f' : 'c') + '"';
  console.log(query);
  var url = 'https://query.yahooapis.com/v1/public/yql?q=' + encodeURIComponent(query) + '&format=json&env=store://datatables.org/alltableswithkeys';
  console.log(url);
  // Send request to Yahoo
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    var json = JSON.parse(this.responseText);
    temperature = parseInt(json.query.results.channel.item.condition.temp);
    console.log ("Temperature is " + temperature);
    
    icon = parseInt(json.query.results.channel.item.condition.code);
    console.log ("Icon: " + icon);
   
    
    var dictionary = {
      'KEY_WEATHER_CODE': icon,
      'KEY_WEATHER_TEMP': temperature
    };
    
    // Send to Pebble
    Pebble.sendAppMessage(dictionary,
    function(e) {
      console.log("Weather info sent to Pebble successfully!");
    },
    function(e) {
      console.log("Error sending weather info to Pebble!");
    }
    );
  };
  xhr.open('GET', url);
  xhr.send();
}



// on location success querying woeid and getting weather
function locationSuccess(pos) {
  // We neeed to get the Yahoo woeid first
  var woeid;
 
  
  var query = 'select * from geo.placefinder where text="' +
    pos.coords.latitude + ',' + pos.coords.longitude + '" and gflags="R"';
  console.log(query);
  var url = 'https://query.yahooapis.com/v1/public/yql?q=' + encodeURIComponent(query) + '&format=json';
  console.log(url);
  // Send request to Yahoo
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    var json = JSON.parse(this.responseText);
    woeid = json.query.results.Result.woeid;
    console.log (woeid);
    getWeather(woeid);
  };
  xhr.open('GET', url);
  xhr.send();

}




function locationError(err) {
  console.log("Error requesting location!");
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
             bluetoothBuzz: 0,
             locationService: 0,
             woeid: 0
         };
     }
    
    console.log("PebbleKit JS ready!");
    var dictionary = {
        "KEY_JSREADY": 1
    };

    // Send to Pebble, so we can load units variable and send it back
    Pebble.sendAppMessage(dictionary,
      function(e) {
        console.log("Ready notice sent to phone!");
      },
      function(e) {
        console.log("Error ready notice to Pebble!");
      }
    ); 
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received");
    
    if (current_settings.locationService == 1) { // for manual location - request weather right away
        getWeather(current_settings.woeid);
    } else {
       getLocation();  // for automatic location - get location
    }
    
  }                     
);

/*    ******************************************************************** Config Section ****************************************** */ 

Pebble.addEventListener("showConfiguration",
  function(e) {
   
    //Load the remote config page
    Pebble.openURL("http://codecorner.galanter.net/pebble/clean_smart_config.htm?version=" + version);
    
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    
    if (e.response !== '') {
      
      console.log('resonse: ' + decodeURIComponent(e.response));
      
      //Get JSON dictionary
      var settings = JSON.parse(decodeURIComponent(e.response));
      
      console.log(settings);
      
      
      //Send to Pebble
      var app_message_json = {};

      // preparing app message
      app_message_json.KEY_HOURS_MINUTES_SEPARATOR = settings.hoursMinutesSeparator;
      app_message_json.KEY_DATE_FORMAT = settings.dateFormat;
      app_message_json.KEY_INVERT_COLORS = settings.invertColors;
      app_message_json.KEY_BLUETOOTH_BUZZ = settings.bluetoothBuzz;
      app_message_json.KEY_LOCATION_SERVICE = settings.locationService;
      app_message_json.KEY_WEATHER_INTERVAL = settings.weatherInterval;
     
      // only storing and passing to pebble temperature format if it changed, because it will cause Pebble to reissue weather AJAX
      if (current_settings.temperatureFormat != settings.temperatureFormat) {
        app_message_json.KEY_TEMPERATURE_FORMAT = settings.temperatureFormat;
      }
      
      // storing new settings
      localStorage.setItem('current_settings', JSON.stringify(settings));
      current_settings = settings;
      
      Pebble.sendAppMessage(app_message_json,
        function(e) {
          console.log("Sending settings data...");
        },
        function(e) {
          console.log("Settings feedback failed!");
        }
      );
    }
  }
);