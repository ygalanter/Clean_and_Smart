var Clay = require('@rebble/clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

var current_settings;

var DEFAULT_SETTINGS = {
  temperatureFormat:     0,
  hoursMinutesSeparator: 0,
  dateFormat:            0,
  topRow:                0,
  bottomRow:             1,
  liveSteps:             0,
  bluetoothAlert:        0,
  language:              255,
  textColor:             16777215,
  bgColor:               0
};

function mergeSettings(stored) {
  var merged = {};
  var key;
  for (key in DEFAULT_SETTINGS) {
    if (Object.prototype.hasOwnProperty.call(DEFAULT_SETTINGS, key)) {
      merged[key] = stored && stored[key] !== undefined && stored[key] !== null
        ? stored[key]
        : DEFAULT_SETTINGS[key];
    }
  }
  return merged;
}

function clayVal(clayData, key, defaultVal) {
  if (defaultVal === undefined) defaultVal = 0;
  var v = clayData[key];
  if (v === undefined || v === null) return defaultVal;

  var raw = (v && typeof v === 'object' && 'value' in v) ? v.value : v;
  if (typeof raw === 'boolean') return raw ? 1 : 0;
  if (typeof raw === 'number' && !isNaN(raw)) return raw | 0;

  var n = parseInt(raw, 10);
  if (!isNaN(n)) return n;

  if (typeof raw === 'string' && /^[0-9a-fA-F]+$/.test(raw)) {
    n = parseInt(raw, 16);
    if (!isNaN(n)) return n;
  }

  return defaultVal;
}

/*  ****************************************** Weather Section **************************************************** */

// converts open-meteo weather icon code to Yahoo weather icon code (to reuse current bitmap with icon set)
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

function getWeather(coords) {
  var url = 'https://api.open-meteo.com/v1/forecast?latitude=' + coords.latitude +
            '&longitude=' + coords.longitude +
            '&current=temperature_2m,weather_code,is_day&temperature_unit=' +
            (current_settings.temperatureFormat === 0 ? 'fahrenheit' : 'celsius');

  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    var json = JSON.parse(this.responseText);
    var temperature = json.current.temperature_2m;
    var code = json.current.weather_code;
    var is_day = json.current.is_day;

    Pebble.sendAppMessage({
      'KEY_WEATHER_CODE': OpenMetroCodeToYahooIcon(code, is_day),
      'KEY_WEATHER_TEMP': temperature
    }, function () {}, function () {});
  };
  xhr.onerror = function () {};
  xhr.open('GET', url);
  xhr.send();
}

function locationSuccess(pos) {
  getWeather(pos.coords);
}

function locationError() {}

function getLocation() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    { timeout: 15000, maximumAge: 60000 }
  );
}

/*  ****************************************** Ready / AppMessage **************************************************** */

Pebble.addEventListener('ready', function () {
  try {
    current_settings = JSON.parse(localStorage.getItem('current_settings'));
  } catch (ex) {
    current_settings = null;
  }

  if (current_settings === null) {
    current_settings = DEFAULT_SETTINGS;
  } else {
    current_settings = mergeSettings(current_settings);
  }

  Pebble.sendAppMessage({ 'KEY_JSREADY': 1 }, function () {}, function () {});
});

Pebble.addEventListener('appmessage', function () {
  getLocation();
});

/*  ****************************************** Config Section **************************************************** */

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) return;

  var clayData = clay.getSettings(e.response, false);

  var msg = {};
  msg.KEY_HOURS_MINUTES_SEPARATOR = clayVal(clayData, 'KEY_HOURS_MINUTES_SEPARATOR');
  msg.KEY_DATE_FORMAT             = clayVal(clayData, 'KEY_DATE_FORMAT');
  msg.KEY_BLUETOOTH_ALERT         = clayVal(clayData, 'KEY_BLUETOOTH_ALERT');
  msg.KEY_LANGUAGE                = clayVal(clayData, 'KEY_LANGUAGE');
  msg.KEY_TEXT_COLOR              = clayVal(clayData, 'KEY_TEXT_COLOR', 16777215);
  msg.KEY_BG_COLOR                = clayVal(clayData, 'KEY_BG_COLOR', 0);
  msg.KEY_TOP_ROW                 = clayVal(clayData, 'KEY_TOP_ROW');
  msg.KEY_BOTTOM_ROW              = clayVal(clayData, 'KEY_BOTTOM_ROW', 1);
  msg.KEY_LIVE_STEPS              = clayVal(clayData, 'KEY_LIVE_STEPS', 0);

  var newTempFormat = clayVal(clayData, 'KEY_TEMPERATURE_FORMAT');
  if (!current_settings || current_settings.temperatureFormat !== newTempFormat) {
    msg.KEY_TEMPERATURE_FORMAT = newTempFormat;
  }

  current_settings = {
    temperatureFormat:     newTempFormat,
    hoursMinutesSeparator: clayVal(clayData, 'KEY_HOURS_MINUTES_SEPARATOR'),
    dateFormat:            clayVal(clayData, 'KEY_DATE_FORMAT'),
    topRow:                clayVal(clayData, 'KEY_TOP_ROW'),
    bottomRow:             clayVal(clayData, 'KEY_BOTTOM_ROW', 1),
    liveSteps:             clayVal(clayData, 'KEY_LIVE_STEPS', 0),
    bluetoothAlert:        clayVal(clayData, 'KEY_BLUETOOTH_ALERT'),
    language:              clayVal(clayData, 'KEY_LANGUAGE', 255),
    textColor:             clayVal(clayData, 'KEY_TEXT_COLOR', 16777215),
    bgColor:               clayVal(clayData, 'KEY_BG_COLOR', 0)
  };
  localStorage.setItem('current_settings', JSON.stringify(current_settings));

  Pebble.sendAppMessage(msg, function () {}, function () {});
});
