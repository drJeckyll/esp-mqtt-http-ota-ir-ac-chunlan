# THIS CODE IS OBSOLETE AND WILL NOT BE UPDATED ANYMORE
Please use new repo here: https://git.jeckyll.net/published/personal/esp8266/esp-mqtt-http-ota-ir-ac-chunlan


# esp-mqtt-http-ota-ir-ac-chunlan

This is same as https://github.com/drJeckyll/esp-mqtt-http-ir-ac-chunlan, but was ported to use rBoot and have OTA updates.

There also two new topics /office/service/ac/restart and /office/service/ac/update. If you send something to 'restart' esp8266 will restart. If you send something to 'update' OTA update will be performed (see bellow for more information).

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

Schematics is something like: http://alexba.in/blog/2013/06/08/open-source-universal-remote-parts-and-pictures/

To reboot ESP8266 just send something to MQTT_PREFIX"restart".

To perform OTA update, first compile rom0.bin and rom1.bin. Put them on web server which can be accessed by http://OTA_HOST:OTA_PORT/OTA_PATH. For example:
```
OTA_HOST="192.168.1.1"
OTA_PORT=80
OTA_PATH="/firmware/"
```
For web server root use "/". Always put trailing slash!

Then just send someting to MQTT_PREFIX"update". After 10-15 seconds update will be done. 

There is no version control of bin files. Update is performed every time no matter if it is old ot new bin file.

If you use BOOT_CONFIG_CHKSUM and BOOT_IROM_CHKSUM (and you should - see warning bellow) and update failed device will return with old bin. You can check which bin is loaded by check settings and see rom:0 (for example). After update succes it will be rom:1. Else it will be rom:0 again, so you must perform update again.

You need:
* rboot boot loader: https://github.com/raburton/rboot
* esptool2: https://github.com/raburton/esptool2

**WARNING:** rboot must be compiled with BOOT_CONFIG_CHKSUM and BOOT_IROM_CHKSUM in rboot.h or it will not boot.

You can remove -iromchksum from FW_USER_ARGS in Makefile and use default settings but OTA update will be unrealible - corrupt roms & etc.

If BOOT_CONFIG_CHKSUM and BOOT_IROM_CHKSUM are enabled then rBoot wi-iromchksumll recover from last booted rom and OTA update is much more stable.

This code was tested with esp-open-sdk (SDK 1.4.0). Flash size 1MB (8Mbit) or more.
