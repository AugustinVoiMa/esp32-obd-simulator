Setup your esp32 as a SSP bluetooth device that is set available for an inbound connection.

Once connected you can open a serial terminal or develop your app.
Your esp32 will act as an elm327 interface, which makes easier to develop an app from your chair rather than your car seat.


Simply open the .ino file in arduino, compile and upload on your esp32, then it will be available as "ESP-32". You can change the bluetooth device name in the setup()
