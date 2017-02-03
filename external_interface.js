var API = require('./iot.api.js'); //рекварим нашу api-библиотеку ОБЩЕГО сервера
var INTERFACE = {}; //тут мы будет описывать всё, что на экспорт

var API_UUID = API.registerInfrastructure('Polytope', {}); // регаемся в нашей API-библиотеке и получаем уникальный ID, 
                                                                 // с которым мы потом всё время к ней обращаемся


REMINDER_MULTIPLIER = 5;

checkInterval = 1000;
var startDayTime;

alerted = [false, false, false, false, false, false, false, false, false, false]

var UUID_PREFIX = '0000';
var UUID_SUFFIX = '00001000800000805f9b34fb';
var SERVICE1_UUID = UUID_PREFIX + "0F00" + UUID_SUFFIX;
var SERVICE2_UUID = UUID_PREFIX + "0F10" + UUID_SUFFIX;
var SERVICE3_UUID = UUID_PREFIX + "0F20" + UUID_SUFFIX;
INTERFACE.serviceUUID = ["0F00", SERVICE1_UUID, SERVICE2_UUID, SERVICE3_UUID];
INTERFACE.charUUID = {
    SERVICE1_UUID : [(UUID_PREFIX + "0F01" + UUID_SUFFIX), (UUID_PREFIX + "0F02" + UUID_SUFFIX)],
     SERVICE2_UUID : [(UUID_PREFIX + "0F11" + UUID_SUFFIX)],
      SERVICE3_UUID : [(UUID_PREFIX + "0F21" + UUID_SUFFIX), (UUID_PREFIX + "0F22" + UUID_SUFFIX)]};

diodeCharUUID = (UUID_PREFIX + "0F22" + UUID_SUFFIX);
buzzerCharUUID = (UUID_PREFIX + "0F21" + UUID_SUFFIX);


currSideCB = function(data, isNotification) {
    currSide = data.readUInt8(0);
    return true;
};

isActiveCB = function(data, isNotification) {
    isActive = data.readUInt8(0);
    return true;
};

var CHAR11_UUID = UUID_PREFIX + "0F01" + UUID_SUFFIX;
var CHAR12_UUID = UUID_PREFIX + "0F02" + UUID_SUFFIX;
var readCallbacks = {CHAR11_UUID : currSideCB, CHAR12_UUID : isActiveCB};

var DiodeChar;
var BuzzerChar;

var Tasks = ['Social networks', 'Eating', 'Social interaction', 'Rest',
     'Studying', 'Work', 'Housekeeping*', 'Planning', 'E-mail', 'Phone call', 'Sports', 'Walk', 'Guitar'];
var currentTasks = ['Social networks', 'Eating', 'Rest',
     'Studying', 'Work', 'Housekeeping*', 'Planning', 'E-mail', 'Phone call', 'Sports', 'Walk'];

var currSide = 12;
var isActive = false;





API.registerNobleServiceUUID(API_UUID, INTERFACE.serviceUUID); 


INTERFACE.data = {}; //этот объект будет передаваться API при динамическом обновлении данных

INTERFACE.data.status = 'Default status'; 
var timeWeek = {};
var timeDay = {};
var weekNames = ['Social networks', 'Eating', 'Social interaction', 'Rest',
     'Studying', 'Work', 'Housekeeping*', 'Planning', 'E-mail', 'Phone call', 'Sports', 'Walk', 'Guitar'];
var dayNames = ['Social networks', 'Eating', 'Rest',
     'Studying', 'Work', 'Housekeeping*', 'Planning', 'E-mail', 'Phone call', 'Sports', 'Walk'];

var upperLimit = {'Social networks' : checkInterval * 5};
var lowerLimit = {};

INTERFACE.data.graphDay = {};
INTERFACE.data.graphWeek = {"Guitar" : checkInterval * 15 * 60};

API.registerDrawHTMLCallback(API_UUID, function(){ // функция, отвечающая за отрисовку html-кода в блок на странице

    //тут мы можем отрисовывать ЧТО хотим и КАК хотим. стандартные поля типа INTERFACE.data.status обрабатываются другими методами (API делает это автоматически, их тут трогать не нужно)
    
    var html = '';

    //html += '<b>СТАТУС:</b> ' + INTERFACE.data.status + "<br>\n"; // ЭТО НЕ НУЖНО!
    //html += '<b>Вывод:</b> ' + (INTERFACE.data.someOutput == 0?'<i>пока ещё не было</i>':INTERFACE.someOutput);

    return html; //возвращаем в код!

});

INTERFACE.emitSocket = function(sock, data){return true;}; //эта функция объявляется на Вашем сервере, когда нужно затригать сокет клиенту 

INTERFACE.init = function(noble){ //this function contains all the main functionality

    //initialize
    setInterval(periodCheck, checkInterval);
    for (var i = 0; i < weekNames.length; i++) {
        if (!timeWeek.hasOwnProperty(weekNames[i])) {
            timeWeek[weekNames[i]] = 0;
        }
    }
    for (var i = 0; i < dayNames.length; i++) {
        if (!timeDay.hasOwnProperty(dayNames[i])) {
            timeDay[dayNames[i]] = 0;
        }
    }
    startDayTime = (new Date()).getTime();

    //BLE onDiscover should be described over here
    noble.on('discover', function(peripheral) {
       // noble.stopScanning();
        console.log("peripheral discovered");
        peripheral.connect(function(error) {
            console.log('connected to peripheral: ' + peripheral.uuid);
            peripheral.discoverServices(INTERFACE.serviceUUID, function(error, services) {
                services.forEach(function(service) { //SOME additional check for service to be valid. you may use simply "var service = services[0];"
                    console.log('found service:', service.uuid);

                    service.discoverCharacteristics(INTERFACE.charUUID[service.uuid], function (error, characteristics) {
                        characteristics.forEach(function(characteristic) {
                            if (readCallbacks.hasOwnProperty(characteristic.uuid)) {
                                characteristic.on('read', readCallbacks[characteristic.uuid]);
                                characteristic.notify(true, function (error) {
                                    console.log('[BLE] some notification is on');
                                    
                                    API.onInfrastructureUpdateInfo(API_UUID, INTERFACE.data);
                                    
                                    API.onInfrastructureRedrawPalette(API_UUID); // говорим API, что блок нужно перерисовать у пользователя

                                    //if('emitSocket' in INTERFACE && typeof INTERFACE.emitSocket == 'function')
                                    INTERFACE.emitSocket.call('client update html', INTERFACE.data);
                                });
                            }
                            if (characteristic.uuid == diodeCharUUID) {
                                DiodeChar = characteristic;
                            }
                            if (characteristic.uuid == buzzerCharUUID) {
                                BuzzerChar = characteristic;
                            }
                        });
                        
                    });
                });
            });
        });
    });

    /*init noble*/
    
    
    return true;
    
};

var InterfaceInited = false;

INTERFACE.initOnce = function(noble){
    if(InterfaceInited) return;
    InterfaceInited = true;
    
    var res = this.init(noble);
    
    if(res)
        console.log('INTERFACE inited successfully');
    else
        console.log('INTERFACE have failed during the initialization');
    
    return res;
};



changeSideName = function(side, name) {
    if (side < 0 || side > 10 || currentTasks.indexOf(name) >= 0) {
        //TODO: add some exception
        console.log("Some incorrect values in changeSideName");
    }
    if (!timeDay.hasOwnProperty(name)) {
        timeDay[name] = 0;
    }
    if (!timeWeek.hasOwnProperty(name)) {
        timeWeek[name] = 0;
    }
    if (Tasks.indexOf(name) < 0) {
        Tasks.push(name);
    }
    currentTasks[side] = name;
    alerted[side] = false;
    return true;
};

periodCheck = function() {
    console.log('periodCheck called');
    var time = (new Date()).getTime();
    if (time - startDayTime >= 24 * 3600 * 1000) {
        currentTasks.forEach(function(task) {
            timeDay[task] = 0;
        });
        while (time - startDayTime >= 24 * 3600 * 1000) {
            startDayTime += 24 * 3600 * 1000;
        };
        for (var i = 0; i <= 10; i++) {
           alerted[i] = false;
        }
    }
    if (isActive && currSide <= 10) {
        var name = currentTasks[currSide];
        //TODO: add real time addition
        timeDay[name] += checkInterval;
        timeWeek[name] += checkInterval;
        if (upperLimit.hasOwnProperty(name) && upperLimit[name] >= timeDay[name] && !alerted[currSide]) {
            alerted[currSide] = true;
            sendWarning(currSide);
        } 
        for (var i = 0; i <= 10; i++) {
            if (i != currSide && lowerLimit.hasOwnProperty(i) && !alerted[i] && (24 * 3600 * 1000 - time) <= lowerLimit[i]* REMINDER_MULTIPLIER) {
                alerted[i] = true;
                sendWarning(i);
                break;
            }
        }
    }
    currentTasks.forEach(function(task) {
        INTERFACE.data.graphDay[task] = timeDay[task];
        INTERFACE.data.graphWeek[task] = timeWeek[task];
    });
    for (var task in timeDay) {
        if (timeDay[task] > 0) {
            INTERFACE.data.graphDay[task] = timeDay[task];
        }
    }
    for (var task in timeWeek) {
        if (timeWeek[task] > 0) {
            INTERFACE.data.graphWeek[task] = timeWeek[task];
        }
    }



    API.onInfrastructureUpdateInfo(API_UUID, INTERFACE.data);
    API.onInfrastructureRedrawPalette(API_UUID); // говорим API, что блок нужно перерисовать у пользователя
    INTERFACE.emitSocket.call('client update html', INTERFACE.data);
};

sendWarning = function(side) {
    console.log("sending warning on side", side);
    var diodes = new Buffer(12);
    for (var i = 0; i < 12; i++) {
        if (i == side) {
            diodes.writeUInt8(1, i);
        } else {
            diodes.writeUInt8(0, i);
        }
    }
    DiodeChar.write(diodes);
    var buzzer_buff = new Buffer(1);
    buzzer_buff.writeUInt8(1, 0);
    BuzzerChar.write(buzzer_buff);
};

module.exports = INTERFACE;
