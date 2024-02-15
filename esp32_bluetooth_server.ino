
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

const char CAN_PROTOCOL = '6';



byte buffer[1024];
int msg_len = 0;

bool E = true;
bool L = true;
bool S = true;
char active_protocol = '0';

void setup() {
  Serial.begin(9600);
  SerialBT.begin("ESP32BT");
  Serial.print("OK");
  SerialBT.println("Connected");
  SerialBT.print(">");
}

String format(String str){
  int res_len = 0;
  int spaces_count = 0;
  char res_str[1024];
  for(int i =0; i < strlen(str.c_str()); i++ ) {
    if (str[i] != ' '){
      if (S and res_len > 0 and ((res_len  - spaces_count) % 2) == 0){
        res_str[res_len++] = ' ';
        spaces_count++;
      }
      
      res_str[res_len++] = str[i];
    }
  }
  return String(res_str).substring(0, res_len);
}

void send_response(String str, bool last_line=true){
  char buf[str.length()];
  memcpy(buf, str.c_str(), str.length());
  SerialBT.write((uint8_t *)buf, str.length());
  SerialBT.write('\r');
}

void loop() {
  if (SerialBT.available()) {
    msg_len = 0;
    while (SerialBT.available()) {
      char car = (char) SerialBT.read();
      if (car != ' ' and car != '\r')
        buffer[msg_len++] = car;
    }
    if (msg_len > 0){
      String str = String((char*)buffer).substring(0, msg_len);
      str.toUpperCase();
      if (E)
        send_response(format(str));

      
      Serial.printf("\nReceived new message: %s of len %d :\n", str, msg_len);
      
      if (str.substring(0,2) == "AT"){
        if(str.length() == 3){
          if (str[2] == 'Z'){
            Serial.println("Requested interface reset, sending the init sequence");
            send_response("ELM327 v2.3");
          }
        } else if (str.length() == 4){
          if (str[2] == 'E'){
            Serial.printf("Requested Echo: %s\n", str);
            E = str[3] == '1';
            send_response("");
          }
          else if (str[2] == 'L'){
            Serial.printf("Requested Line Feed: %s\n", str);
            L = str[3] == '1';
            send_response("");
          }
          else if (str[2] == 'S'){
            Serial.printf("Requested Spaces: %s\n", str);
            S = str[3] == '1';
            send_response("");
          }
        } else if (str.length() == 5){
          if (str.substring(0,4) == "ATSP"){
            Serial.printf("Change protocol: %s\n", str);
            active_protocol = str[4];
            send_response("OK");
          }
        }
      }
      else{
        if (active_protocol == '0'){
          send_response("SEARCHING...");
          delay(5);
          active_protocol = CAN_PROTOCOL;
        }
        
        if (active_protocol != CAN_PROTOCOL){
          // different error message depending on the protocol
          if (active_protocol == '1' || active_protocol == '2')
            send_response("NO DATA");
          else if (active_protocol == '3' || active_protocol == '4' || active_protocol == '5')
            send_response("BUS INIT: ...ERROR");
          else if (active_protocol == '6' || active_protocol == '7' || active_protocol == '8' || active_protocol == '9' || active_protocol == 'A' || active_protocol == 'B' || active_protocol == 'C')
            send_response("CAN ERROR"); 
        }
        else{
          if (str == "0100"){
            Serial.println("Requested available pids for mode 1");
            String res = format("41 00 BE 3F A8 11");
            Serial.println(res);
            send_response(res);
          }
          else if(str == "0902"){
            Serial.println("Requested VIN, serving a const reply");
            String rows[] = {
            "014",
            "0: 49 02 01 57 46 30",
            "1: 44 41 42 43 44 45 46",
            "2: 47 48 30 31 32 33 34"      
            };
            for(int i=0; i < 4; i++){
              String row = format(rows[i]);
              Serial.println(row);
              send_response(row, i==3);
            }
          }
          else
            SerialBT.write('?');
        }
      }

      SerialBT.write('\r');
      if (L)
        SerialBT.write('>');
    }
  }

  if (Serial.available()) {
    msg_len = 0;
    while (Serial.available()) {
      buffer[msg_len++] = Serial.read();
    }
    Serial.printf("Send new message of len %d :\n", msg_len);
    SerialBT.write(buffer, msg_len);
  }
  delay(20);
}
