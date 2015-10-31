var app = angular.module('app', []);

app.config(function ($httpProvider) {
  $httpProvider.defaults.transformRequest = function (data) {
    if (data === undefined) {
      return data;
    }
    return $.param(data);
  };
  $httpProvider.defaults.headers.post['Content-Type'] = 'application/x-www-form-urlencoded;';
});

app.controller('MainCtrl', function ($scope, $http, $timeout, patternService, variableService) {
  $scope.brightness = "255";
  $scope.busy = false;
  $scope.power = 1;
  $scope.color = "#0000ff"
  $scope.r = 0;
  $scope.g = 0;
  $scope.b = 255;
  $scope.noiseSpeedX = 0;
  $scope.noiseSpeedY = 0;
  $scope.noiseSpeedZ = 0;
  $scope.noiseScale = 0;
  $scope.powerText = "On";
  $scope.status = "Please enter your access token.";
  $scope.disconnected = false;
  $scope.accessToken = "";
  $scope.isDeviceSelected = false;

  $scope.patterns = [];
  $scope.patternIndex = 0;

  $scope.devices = [];

  $scope.onSelectedDeviceChange = function() {
    $scope.isDeviceSelected = $scope.device != null;

    var devicePatterns = localStorage["deviceId" + $scope.device.id + "patterns"];

    if(devicePatterns === undefined)
      $scope.patterns = null;
    else
      $scope.patterns = JSON.parse(devicePatterns);
  }

  $scope.onSelectedPatternChange = function() {
    $scope.setPattern();
  }

  $scope.onSelectedColorChange = function() {
    $scope.setColor();
  }

  $scope.getDevices = function () {
    $scope.status = 'Getting devices...';

    $http.get('https://api.particle.io/v1/devices?access_token=' + $scope.accessToken).
      success(function (data, status, headers, config) {
        if (data && data.length > 0) {
          var deviceId = null;

          if (Modernizr.localstorage) {
            deviceId = localStorage["deviceId"];
          }

          for (var i = 0; i < data.length; i++) {
            var device = data[i];
            device.title = (device.connected == true ? "● " : "◌ ") + device.name;
            device.status = device.connected == true ? "online" : "offline";

            if (data[i].id == deviceId) {
              $scope.device = data[i];
              $scope.onSelectedDeviceChange();
            }
          }

          $scope.devices = data;

            if (!$scope.device)
              $scope.device = data[0];

          $scope.isDeviceSelected = $scope.device != null;
        }
        $scope.status = 'Loaded devices';
      }).
      error(function (data, status, headers, config) {
        $scope.busy = false;
        $scope.status = data.error_description;
      });
  }

  if (Modernizr.localstorage) {
    $scope.accessToken = localStorage["accessToken"];
    // $('#inputAccessToken').scope().$apply();

    if ($scope.accessToken && $scope.accessToken != "") {
      $scope.status = "";
      $scope.getDevices();
    }
  }

  $scope.save = function () {
    localStorage["accessToken"] = $scope.accessToken;
    $scope.status = "Saved access token.";

    $scope.getDevices();
  }

  $scope.connect = function () {
    // $scope.busy = true;

    localStorage["deviceId"] = $scope.device.id;

    variableService.getVariableValue("power", $scope.device.id, $scope.accessToken)
    .then(function (response) {
      $scope.power = response.data.result;
      $scope.status = 'Loaded power';
    })

    .then(function (data) {
      return variableService.getVariableValue("brightness", $scope.device.id, $scope.accessToken);
    })
    .then(function (response) {
      $scope.brightness = response.data.result;
      $scope.status = 'Loaded brightness';
    })

    .then(function (data) {
      return variableService.getExtendedVariableValue("r", $scope.device.id, $scope.accessToken);
    })
    .then(function (response) {
      $scope.r = response.data.return_value;
      $scope.status = 'Loaded red';
    })

    .then(function (data) {
      return variableService.getExtendedVariableValue("g", $scope.device.id, $scope.accessToken);
    })
    .then(function (response) {
      $scope.g = response.data.return_value;
      $scope.status = 'Loaded green';
    })

    .then(function (data) {
      return variableService.getExtendedVariableValue("b", $scope.device.id, $scope.accessToken);
    })
    .then(function (response) {
      $scope.b = response.data.return_value;
      $scope.status = 'Loaded blue';
    })

    .then(function (data) {
      return variableService.getExtendedVariableValue("nsx", $scope.device.id, $scope.accessToken);
    })
    .then(function (response) {
      $scope.noiseSpeedX = response.data.return_value;
      $scope.status = 'Loaded noise speed x';
    })

    .then(function (data) {
      return variableService.getExtendedVariableValue("nsy", $scope.device.id, $scope.accessToken);
    })
    .then(function (response) {
      $scope.noiseSpeedY = response.data.return_value;
      $scope.status = 'Loaded noise speed y';
    })

    .then(function (data) {
      return variableService.getExtendedVariableValue("nsz", $scope.device.id, $scope.accessToken);
    })
    .then(function (response) {
      $scope.noiseSpeedZ = response.data.return_value;
      $scope.status = 'Loaded noise speed z';
    })

    .then(function (data) {
      return variableService.getExtendedVariableValue("nsc", $scope.device.id, $scope.accessToken);
    })
    .then(function (response) {
      $scope.noiseScale = response.data.return_value;
      $scope.status = 'Loaded noise scale';
    })

    .then(function (data) {
      if($scope.patterns === undefined)
        $scope.getPatterns();
      else
        $scope.getPatternIndex();
    });
  }

  $scope.getPower = function () {
    // $scope.busy = true;
    $http.get('https://api.particle.io/v1/devices/' + $scope.device.id + '/power?access_token=' + $scope.accessToken).
      success(function (data, status, headers, config) {
        $scope.busy = false;
        $scope.power = data.result;
      }).
      error(function (data, status, headers, config) {
        $scope.busy = false;
        $scope.status = data.error_description;
      });
  };

  $scope.powerOn = function () {
    // $scope.busy = true;
    $http({
      method: 'POST',
      url: 'https://api.particle.io/v1/devices/' + $scope.device.id + '/variable',
      data: { access_token: $scope.accessToken, args: "pwr:1" },
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
    }).
    success(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.power = data.return_value;
      $scope.status = $scope.power == 1 ? 'Turned on' : 'Turned off';
    }).
    error(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.status = data.error_description;
    });
  };

  $scope.powerOff = function () {
    // $scope.busy = true;
    $http({
      method: 'POST',
      url: 'https://api.particle.io/v1/devices/' + $scope.device.id + '/variable',
      data: { access_token: $scope.accessToken, args: "pwr:0" },
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
    }).
    success(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.power = data.return_value;
      $scope.status = $scope.power == 1 ? 'Turned on' : 'Turned off';
    }).
    error(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.status = data.error_description;
    });
  };

  $scope.getBrightness = function () {
    // $scope.busy = true;
    $http.get('https://api.particle.io/v1/devices/' + $scope.device.id + '/brightness?access_token=' + $scope.accessToken).
      success(function (data, status, headers, config) {
        $scope.busy = false;
        $scope.brightness = data.result;
      }).
      error(function (data, status, headers, config) {
        $scope.busy = false;
        $scope.status = data.error_description;
      });
  };

  $scope.setBrightness = function ($) {
    // $scope.busy = true;
    $http({
      method: 'POST',
      url: 'https://api.particle.io/v1/devices/' + $scope.device.id + '/variable',
      data: { access_token: $scope.accessToken, args: "brt:" + $scope.brightness },
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
    }).
    success(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.brightness = data.return_value;
      $scope.status = 'Brightness set';
    }).
    error(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.status = data.error_description;
    });
  };

  $scope.setColor = function ($) {
    var rgb = $scope.hexToRgb();

    $scope.r = rgb.r;
    $scope.g = rgb.g;
    $scope.b = rgb.b;

    $scope.setR();
    $scope.setG();
    $scope.setB();
  };

  $scope.hexToRgb = function ($) {
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec($scope.color);
    return result ? {
      r: parseInt(result[1], 16),
      g: parseInt(result[2], 16),
      b: parseInt(result[3], 16)
    } : null;
  }

  $scope.setR = function ($) {
    // $scope.busy = true;
    $http({
      method: 'POST',
      url: 'https://api.particle.io/v1/devices/' + $scope.device.id + '/variable',
      data: { access_token: $scope.accessToken, args: "r:" + $scope.r },
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
    }).
    success(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.r = data.return_value;
      $scope.status = 'Red set';
    }).
    error(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.status = data.error_description;
    });
  };

  $scope.setG = function ($) {
    // $scope.busy = true;
    $http({
      method: 'POST',
      url: 'https://api.particle.io/v1/devices/' + $scope.device.id + '/variable',
      data: { access_token: $scope.accessToken, args: "g:" + $scope.g },
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
    }).
    success(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.g = data.return_value;
      $scope.status = 'Green set';
    }).
    error(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.status = data.error_description;
    });
  };

  $scope.setB = function ($) {
    // $scope.busy = true;
    $http({
      method: 'POST',
      url: 'https://api.particle.io/v1/devices/' + $scope.device.id + '/variable',
      data: { access_token: $scope.accessToken, args: "b:" + $scope.b },
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
    }).
    success(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.b = data.return_value;
      $scope.status = 'Blue set';
    }).
    error(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.status = data.error_description;
    });
  };

  $scope.setNoiseSpeedX = function ($) {
    // $scope.busy = true;
    $http({
      method: 'POST',
      url: 'https://api.particle.io/v1/devices/' + $scope.device.id + '/variable',
      data: { access_token: $scope.accessToken, args: "nsx:" + $scope.noiseSpeedX },
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
    }).
    success(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.noiseSpeedX = data.return_value;
      $scope.status = 'noiseSpeedX set';
    }).
    error(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.status = data.error_description;
    });
  };

  $scope.setNoiseSpeedY = function ($) {
    // $scope.busy = true;
    $http({
      method: 'POST',
      url: 'https://api.particle.io/v1/devices/' + $scope.device.id + '/variable',
      data: { access_token: $scope.accessToken, args: "nsy:" + $scope.noiseSpeedY },
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
    }).
    success(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.noiseSpeedY = data.return_value;
      $scope.status = 'noiseSpeedY set';
    }).
    error(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.status = data.error_description;
    });
  };

  $scope.setNoiseSpeedZ = function ($) {
    // $scope.busy = true;
    $http({
      method: 'POST',
      url: 'https://api.particle.io/v1/devices/' + $scope.device.id + '/variable',
      data: { access_token: $scope.accessToken, args: "nsz:" + $scope.noiseSpeedZ },
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
    }).
    success(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.noiseSpeedZ = data.return_value;
      $scope.status = 'noiseSpeedZ set';
    }).
    error(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.status = data.error_description;
    });
  };

  $scope.setNoiseScale = function ($) {
    // $scope.busy = true;
    $http({
      method: 'POST',
      url: 'https://api.particle.io/v1/devices/' + $scope.device.id + '/variable',
      data: { access_token: $scope.accessToken, args: "nsc:" + $scope.noiseScale },
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
    }).
    success(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.noiseScale = data.return_value;
      $scope.status = 'noiseScale set';
    }).
    error(function (data, status, headers, config) {
      $scope.busy = false;
      $scope.status = data.error_description;
    });
  };

  $scope.getPatternIndex = function () {
    // $scope.busy = true;
    $http.get('https://api.particle.io/v1/devices/' + $scope.device.id + '/patternIndex?access_token=' + $scope.accessToken).
      success(function (data, status, headers, config) {
        $scope.busy = false;
        $scope.patternIndex = data.result;
        $scope.pattern = $scope.patterns[$scope.patternIndex];
        $scope.disconnected = false;
        $scope.status = 'Ready';
      }).
      error(function (data, status, headers, config) {
        $scope.busy = false;
        $scope.status = data.error_description;
      });
  };

  $scope.getPatternNames = function (index) {
    if (index < $scope.patternCount) {
      var promise = patternService.getPatternName(index, $scope.device.id, $scope.accessToken);
      promise.then(
        function (payload) {
          $scope.patterns.push({ index: index, name: payload.data.result });
          $scope.status = 'Loaded pattern name ' + index;
          $scope.getPatternNames(index + 1);
        });
    }
    else {
      $scope.busy = false;
      $scope.getPatternIndex();

      localStorage["deviceId" + $scope.device.id + "patterns"] = JSON.stringify($scope.patterns);
    }
  };

  $scope.getPatterns = function () {
    // $scope.busy = true;

    // get the pattern count
    var promise = $http.get('https://api.particle.io/v1/devices/' + $scope.device.id + '/patternCount?access_token=' + $scope.accessToken);

    // get the name of the first pattern
    // getPatternNames will then recursively call itself until all pattern names are retrieved
    promise.then(
      function (payload) {
        $scope.patternCount = payload.data.result;
        $scope.patterns = [];
        $scope.status = 'Loaded pattern count';

        $scope.getPatternNames(0);
      },
      function (errorPayload) {
        $scope.busy = false;
        $scope.status = data.error_description;
      });
  };

  $scope.setPattern = function () {
    // $scope.busy = true;

    var promise = $http({
      method: 'POST',
      url: 'https://api.particle.io/v1/devices/' + $scope.device.id + '/patternIndex',
      data: { access_token: $scope.accessToken, args: $scope.pattern.index },
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
    })
	  .then(
	    function (payload) {
	      $scope.busy = false;
	      $scope.status = 'Pattern set';
	    },
	    function (errorPayload) {
	      $scope.busy = false;
	    });
  };
});

app.factory('patternService', function ($http) {
  return {
    getPatternName: function (index, deviceId, accessToken) {
      return $http({
        method: 'POST',
        url: 'https://api.particle.io/v1/devices/' + deviceId + '/pNameCursor',
        data: { access_token: accessToken, args: index },
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
      })
      .then(
      function (payload) {
        return $http.get('https://api.particle.io/v1/devices/' + deviceId + '/patternName?access_token=' + accessToken);
      });
    }
  }
});

app.factory('variableService', function ($http) {
  return {
    getVariableValue: function (variableName, deviceId, accessToken) {
      return $http({
        method: 'GET',
        url: 'https://api.particle.io/v1/devices/' + deviceId + '/' + variableName + '?access_token=' + accessToken,
      });
    },

    getExtendedVariableValue: function (variableName, deviceId, accessToken) {
      return $http({
        method: 'POST',
        url: 'https://api.particle.io/v1/devices/' + deviceId + '/varCursor',
        data: { access_token: accessToken, args: variableName },
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
      });
    }
  }
});
