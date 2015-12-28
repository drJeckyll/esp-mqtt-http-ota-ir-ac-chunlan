# esp-mqtt-http-ota-ir-ac-chunlan

This is same as https://github.com/drJeckyll/esp-mqtt-http-ir-ac-chunlan, but was ported to use rBoot and have OTA updates.

There also two new topics /office/service/ac/restart and /office/service/ac/update. If you send something to 'restart' esp8266 will restart. If you send something to 'update' OTA update will be performed.

Settings for WIFI, MQTT and OTA are in Makefile, or you can set them via env:

```DEVICE=test WIFI_SSID=Test make```

You must set DEVICE since it is used both OTA and MQTT.

If you want to use this project unmodified you must have at least 16Mbit flash (2MB).


You can read more for this project here (Bulgarian): http://f-e-n.net/ur

In my office I have some cheap AC named Chunlan which I want to operate remotly. So I make this IR remote control with ESP8266. It works via MQTT and also have WEB interafce.

Settings for MQTT and WIFI are in include/user_config.h.

IR protocol is like this:
```
 0 \ 11 Heat | 00 Cool
 1 / 10 Dry
 2
 3 - 1 On | 0 Off
 4
 5
 6
 7

 8 | Temp 9
 9 |
10 |
11 |
12 |
13 |
14
15

16 \ 00 Fan auto | 01 Fan 2
17 / 11 Fan 1    | 10 Fan 3
18
19 - Vane
20
21
22
23
```

Temp = Temp + 9

I try to use PWM, but I failed, so I make it with delays. You can see code in user/ac.c - ir_send().

You can use the following template and just set bits which you want:
000000000000000000000000001001001110000000000000000000000000000000110000

You can change settings via JSON and MQTT on topic /office/service/ac. And on /office/service/ac/settings you will receive new settings:
{"power":"on","mode":"heat","temp":"18","fan":"3","swing":"off","dht_temp":"2000","dht_humid":"3900"}

You can get settings via JSON:
curl -u user:pass http://ip.address/load.tpl
{"power":"on","mode":"heat","temp":"18","fan":"3","swing":"off","dht_temp":"2000","dht_humid":"3900"}

You can set settings:
curl -u user:pass http://ip.address/save.cgi?power=on&mode=heat
OK

Valid settings are:
power: on/off
mode: heat/cool
temp: 15-30
fan: 1/2/3/auto
swing: on/off

This code was tested with ESP SDK 0.9.3.

Schematics is something like: http://alexba.in/blog/2013/06/08/open-source-universal-remote-parts-and-pictures/
