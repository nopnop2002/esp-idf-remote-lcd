# esp-idf-remote-hd44780
Control the HD44780 using HTTP/MQTT.   
I use [this](https://github.com/UncleRus/esp-idf-lib/tree/master/components/hd44780) component.   
It's great work.   
I made this to know if there are any notify on my HEAD LESS linux server.   
![head-less-linux](https://user-images.githubusercontent.com/6020549/219907409-27180c9f-5102-449e-b228-574bc881bf18.JPG)
![head-less-linux-2](https://user-images.githubusercontent.com/6020549/219907739-55e6ee3a-98c5-45e4-896b-a3ce6ea3f54c.JPG)

![HTTP](https://user-images.githubusercontent.com/6020549/215644607-3bc41fe5-a8d5-4baa-bedf-b8aeda235ee5.JPG)
![MQTT](https://user-images.githubusercontent.com/6020549/215644778-1413971f-bee6-4505-bb0d-4a343f3149a7.JPG)


# Software requirements
ESP-IDF V4.4/V5.0.   

# Hardware requirements
Character display using HD44780.   
Usually called 1602LCD/1604LCD.   

# Installation
```Shell
git clone https://github.com/nopnop2002/esp-idf-remote-hd44780
cd esp-idf-remote-hd44780
idf.py menuconfig
idf.py flash
```

# Configuration

![config-top](https://user-images.githubusercontent.com/6020549/215234404-aaf6ba86-282e-4f1a-aa17-8f53c93194e4.jpg)
![config-app](https://user-images.githubusercontent.com/6020549/215645553-858aaf34-f906-466f-b2bf-ef690b7315fa.jpg)

## WiFi Setting   
![config-wifi-1](https://user-images.githubusercontent.com/6020549/215234488-b980e32a-96d0-49b7-93dc-c1f476780975.jpg)

You can connect using the mDNS hostname instead of the IP address.   
![config-wifi-2](https://user-images.githubusercontent.com/6020549/215234489-ad51183b-1c9a-47ad-a06f-1d7fbd9d520e.jpg)

You can use static IP.   
![config-wifi-3](https://user-images.githubusercontent.com/6020549/215234490-02c33ef4-79f6-4839-91d8-33ba38578964.jpg)

## Network protocol Setting
Using HTTP   
![config-protocol-http](https://user-images.githubusercontent.com/6020549/215645885-9c1c02cf-0ef6-4708-ab56-71708621155c.jpg)

Using MQTT   
![config-protocol-mqtt](https://user-images.githubusercontent.com/6020549/215645755-a72e1837-287d-4172-8850-e2fabe35f8b1.jpg)

## Device Setting   
![config-device](https://user-images.githubusercontent.com/6020549/215236874-f2010efc-c843-4a1f-9420-d485e43e23a7.jpg)   
___The initial value is no backlight control.___   


# Wiring
|LCD||ESP32|ESP32S2/S3|ESP32C2/C3||
|:-:|:-:|:-:|:-:|:-:|:-|
|VSS|--|GND|GND|GND||
|VDD|--|3.3V|3.3V|3.3V||
|VO|--||||Variable resistor output|
|RS|--|GPIO19|GPIO1|GPIO0|*1|
|RW|--|GND|GND|GND||
|E|--|GPIO18|GPIO2|GPIO1|*1|
|D0|--|N/C|N/C|N/C||
|D1|--|N/C|N/C|N/C||
|D2|--|N/C|N/C|N/C||
|D3|--|N/C|N/C|N/C||
|D4|--|GPIO5|GPIO3|GPIO2|*1|
|D5|--|GPIO17|GPIO4|GPIO3|*1|
|D6|--|GPIO16|GPIO5|GPIO4|*1|
|D7|--|GPIO4|GPIO6|GPIO5|*1|
|A|--|5V|5V|5V|*2|
|A|--|3.3V|3.3V|3.3V|*2|
|K|--|GND|GND|GND||

(*1) You can change any GPIO using menuconfig.   

(*2) 5V for 5V modules. 3.3V for 3.3V modules.   


# API for HTTP

- cursor:off blink:off(default)   
```curl -X POST -H "Content-Type: application/json" -d '{"lcd":"on", "cursor":"off", "blink":"off"}' http://esp32-server.local:8080/api/control```

- cursor:on blink:off   
```curl -X POST -H "Content-Type: application/json" -d '{"lcd":"on", "cursor":"on", "blink":"off"}' http://esp32-server.local:8080/api/control```

- cursor:off blink:on    
```curl -X POST -H "Content-Type: application/json" -d '{"lcd":"on", "cursor":"off", "blink":"on"}' http://esp32-server.local:8080/api/control```

- cursor:on blink:on   
```curl -X POST -H "Content-Type: application/json" -d '{"lcd":"on", "cursor":"on", "blink":"on"}' http://esp32-server.local:8080/api/control```

- clear lcd   
```curl -X POST -H "Content-Type: application/json" http://esp32-server.local:8080/api/clear```

- set position   
```curl -X POST -H "Content-Type: application/json" -d '{"col":0, "line":0}' http://esp32-server.local:8080/api/gotoxy```

- put character on lcd   
```curl -X POST -H "Content-Type: application/json" -d '{"char":"s"}' http://esp32-server.local:8080/api/putc```

- put string on lcd   
```curl -X POST -H "Content-Type: application/json" -d '{"str":"abcdefghijklmn"}' http://esp32-server.local:8080/api/puts```

- backlight on   
```curl -X POST -H "Content-Type: application/json" -d '{"backlight":"on"}' http://esp32-server.local:8080/api/backlight```

- backlight off   
```curl -X POST -H "Content-Type: application/json" -d '{"backlight":"off"}' http://esp32-server.local:8080/api/backlight```


# Backlight control   
___A transistor is required for backlight control.___   
```
                  emitter
ESP32 GND -----------------+
                           |
                    base   /
ESP32 BL  -----^^^--------|    SS8050
                           \
               collector   |
                           +------- LCD K
ESP32 3.3V/5V --------------------- LCD A

```


# Demo for HTTP   
```
sudo apt install curl
bash ./demo_http.sh
```


# API for MQTT

- cursor:off   
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/hd44780/cursor" -m "off"```

- cursor:on   
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/hd44780/cursor" -m "on"```

- blink:off    
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/hd44780/blink" -m "off"```

- blink:on   
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/hd44780/blink" -m "on"```

- clear lcd   
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/hd44780/clear" -m ""```

- set column   
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/hd44780/col" -m "0"```

- set line   
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/hd44780/line" -m "0"```

- put character on lcd   
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/hd44780/putc" -m "s"```

- put string on lcd   
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/hd44780/puts" -m "abcdefghijklmn"```

- backlight on   
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/hd44780/backlight" -m "on"```

- backlight off   
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/hd44780/backlight" -m "off"```


# Demo for MQTT   
```
sudo apt install mosquitto-clients
bash ./demo_mqtt.sh
```
