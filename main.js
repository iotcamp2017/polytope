var noble = require('noble');


var ipAddress = '192.168.42.1';

var coordServiceUuid = '13333333333333333333333333333331';
var coordCharUuid = '00000000000000000000000072646372';
var a;
var timeElapsed = new Date();
var time = [0, 0, 0, 0, 0, 0, 0]

var face = 0;
var connectionStatus = 0;


noble.on('stateChange', function(state) {
    if (state === 'poweredOn') {
        console.log("powered on");
        setInterval(function() {
            console.log("scanning");
            noble.startScanning([coordServiceUuid], true);
        }, 2000);

    } else {
        noble.stopScanning();
    }
})


var coordService = null;
var coordCharacteristic = null;

noble.on('discover', function(peripheral) {
    console.log('found peripheral', peripheral.advertisement);

    peripheral.connect(function(err) {
        peripheral.discoverServices([coordServiceUuid], function(err, services) {
            services.forEach(function(service) {
                console.log('found service:', service.uuid);
                connectionStatus = 1;
                service.discoverCharacteristics([], function(err, characteristics) {
                    characteristics.forEach(function(characteristic) {
                        console.log('found characteristic:', characteristic.uuid);
                        if (coordCharUuid == characteristic.uuid) {
                            coordCharacteristic = characteristic;
                        }
                    })
                    if (coordCharacteristic) {
                        doCoords();
                    } else {
                        console.log("Missing characteristics");
                    }
                })
            })
        })
    })
})


function doCoords() {
    coordCharacteristic.on('read', function(data, isNotification) {
        console.log(data.readUInt8(1));
        time[face] += (new Date() - timeElapsed)
        timeElapsed = new Date();
        face = data.readUInt8(1);
    });
    coordCharacteristic.notify(true, function(error) {
        console.log('notification on');
        connectionStatus = 2;
    });
}



var fs = require('fs');
var lightSensorPage = fs.readFileSync('/node_app_slot/lightsensor.html');
// Insert the ip address in the code in the page
lightSensorPage = String(lightSensorPage).replace(/<<ipAddress>>/, ipAddress);
var http = require('http');
http.createServer(function(req, res) {
    var value;
    // This is a very quick and dirty way of detecting a request for the page
    // versus a request for light values
    if (req.url.indexOf('lightsensor') != -1) {
        res.writeHead(200, {
            'Content-Type': 'text/html'
        });
        res.end(lightSensorPage);
    } else {
        value = face;
        res.writeHead(200, {
            'Content-Type': 'text/json'
        });
        var timePassed = (new Date() - timeElapsed);
        var timeCopy = time.slice();
        timeCopy[value] += timePassed;
        res.end(JSON.stringify({
            lightLevel: value,
            connectionStatus: connectionStatus,
            a1time: timeCopy[1],
            a2time: timeCopy[2],
            a3time: timeCopy[3],
            a4time: timeCopy[4],
            a5time: timeCopy[5],
            a6time: timeCopy[6],
            rawValue: value
        }));
    }
}).listen(1337, ipAddress);