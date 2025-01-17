
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

const char CAN_PROTOCOL = '6';
const char DEFAULT_PROTOCOL = CAN_PROTOCOL;


byte buffer[1024];
int msg_len = 0;

bool E = true;
bool L = true;
bool S = true;
bool H = false;
String DEVICE_DESCRIPTION = "OBDII to RS232 Interpreter";
String DEVICE_IDENTIFIER = "cccccccccccc";
String DEVICE_VERSION = "ELM327 v2.3";
char active_protocol = '0';

void reset(){
  E = true;
  L = true;
  S = true;
  H = false;
}

void setup() {
  Serial.begin(9600);
  SerialBT.begin("ESP32BT");
  Serial.println("OK");
  SerialBT.println("Connected");
  SerialBT.print(">");
  randomSeed(analogRead(0));
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
  Serial.printf("tx: %s\r\n", str);
  SerialBT.write('\r');
}

void empty_response(){
  SerialBT.write('\r');
}

String randomValue(int numBytes, float canErrorProba){
  if (random(0,100) < (canErrorProba*100))
    return "CAN ERROR";
  int value = random(0, (int) pow(256,numBytes));
  String ret = String(value, HEX);
  ret.toUpperCase();
  while(ret.length() < (numBytes*2))
    ret = "0"+ret;
  return ret;
  
}
void malformed_command(){
  SerialBT.write('?'); 
  Serial.println("malformed\r\ntx: ?");
}


void at_command(String str){
  if (str.length() == 3 and str[2] == 'Z'){
    Serial.println("Requested interface reset.");
    send_response("ELM327 v2.3");
  }
  else if (str.length() == 4 and str.substring(2,4) == "@1"){
    send_response(DEVICE_DESCRIPTION);
  }
  else if (str.length() == 4 and str.substring(2,4) == "@2"){
    send_response(DEVICE_IDENTIFIER);
  }
  else if (str.length() == 16 and str.substring(2,4) == "@3"){
    DEVICE_IDENTIFIER=str.substring(4,16);
    send_response("OK");
  }
  else if (str.length() == 5 and str.substring(2,4) == "AT"){
    Serial.printf("Requested Adaptative Timing: %s\r\n", str);
    empty_response();
  }
  else if (str.length() == 4 and str[2] == 'E'){
    Serial.printf("Requested Echo: %s\r\n", str);
    E = str[3] == '1';
    send_response("OK");
  }
  else if (str.length() == 3 and str[2] == 'I'){
    send_response(DEVICE_VERSION);
  }
  else if (str.length() == 4 and str[2] == 'M'){
    Serial.println("Not Implemented: non-volatime memory mode");
    send_response("OK");
  }
  else if (str.length() == 4 and str[2] == 'H'){
    Serial.printf("Requested Additional Header bytes: %s\r\n", str);
    H = (str[3] == '1');
    if (H){
      Serial.println("Unsupported AT H1.");
    }
    send_response("OK");
  }
  else if (str.length() == 4 and str[2] == 'L'){
    Serial.printf("Requested Line Feed: %s\r\n", str);
    L = (str[3] == '1');
    send_response("OK");
  }
  else if (str.length() == 4 and str[2] == 'S'){
    Serial.printf("Requested Spaces: %s\r\n", str);
    S = str[3] == '1';
    send_response("OK");
  }
  else if (str.length() == 5 and str.substring(0,4) == "ATSP"){
    Serial.printf("Change protocol: %s\r\n", str);
    active_protocol = str[4];
    send_response("OK");
  }
  else if (str.length() == 4 and str.substring(0,4) == "ATPC"){
    Serial.printf("Stop protocol\r\n", str);
    send_response("OK");
  }
  else if (str.length() == 4 and str.substring(0,4) == "ATRV"){
    Serial.printf("Read Voltage\r\n", str);
    send_response("12.6V");
  }
  else if (str.length() == 4 and str.substring(0,4) == "ATAL"){
    Serial.printf("Allow long message\r\n", str);
    send_response("OK");
  }
  else if (str.length() == 6 and str.substring(0,4) == "ATST"){
    Serial.printf("Set CAN Timeout\r\n", str);
    send_response("OK");
  }
  else if (str.length() == 5 and str.substring(0,4) == "ATKW"){
    Serial.printf("Set CAN Keyword\r\n", str);
    send_response("OK");
  }
  else if (str.length() == 6 and str.substring(0,4) == "ATSW"){
    Serial.printf("Set CAN alive beacon timer\r\n", str);
    send_response("OK");
  }
  else if (str.length() == 5 and str.substring(0,5) == "ATDPN"){
    Serial.printf("Requested current protocol: %s\r\n", str);
    char current_protocol_arr[] = {active_protocol};
    send_response(String(current_protocol_arr));
  }
  else if (str.length() == 6 and str.substring(0,5) == "ATSPA"){
    Serial.printf("Change protocol auto: %s\r\n", str);
    active_protocol = str[5];
    send_response("OK");
  }
  else{
    malformed_command();
  }
}

void auto_protocol(){
  if (active_protocol == '0'){
    send_response("SEARCHING...");
    delay(5);
    active_protocol = DEFAULT_PROTOCOL;
  }
}

bool check_protocol(){
  if (active_protocol != CAN_PROTOCOL){
    // different error message depending on the protocol
    if (active_protocol == '1' 
    || active_protocol == '2')
    {
     send_response("NO DATA");
     return false; 
    }
    else if (active_protocol == '3' 
    || active_protocol == '4' 
    || active_protocol == '5')
    {
      send_response("BUS INIT: ...ERROR");
      return false;
    }
    else if (active_protocol == '6' 
    || active_protocol == '7' 
    || active_protocol == '8' 
    || active_protocol == '9' 
    || active_protocol == 'A' 
    || active_protocol == 'B' 
    || active_protocol == 'C')
    {
      send_response("CAN ERROR"); 
      return false;
    }
  }
  return true;
}

void mode01_command(String str){
  if (str == "0100"){
    Serial.println("Requested available pids for mode 1 (1-20)");
    String res = format("41 00 BE 3F A8 11");
    send_response(res);
  } else if (str == "0120"){
    Serial.println("Requested available pids for mode 1 (21-40)");
    String res = format("41 20 A0 07 B0 11");
    send_response(res); 
  } else if (str == "0140"){
    Serial.println("Requested available pids for mode 1 (41-60)");
    String res = format("41 40 FE D3 00 40");
    send_response(res); 
  }
  else if (str == "0103"){
    Serial.println("Requested fuel status");
    String res = format("41 03 02");
    send_response(res);
  } 
  else if (str == "0104"){ // Load
    Serial.println("Requested Load");
    String res = randomValue(2, 0.05);
    if (res != "CAN ERROR")
      res = format("41 04 "+res);
    send_response(res);
  }
  else if (str == "0107"){ // LTFT
    Serial.println("Requested LTFT");
    String res = randomValue(1, 0.05);
    if (res != "CAN ERROR")
      res = format("41 07 "+res);
    send_response(res);
  }
  else if (str == "0106"){ // STFT
    Serial.println("Requested STFT");
    String res = randomValue(1, 0.05);
    if (res != "CAN ERROR")
      res = format("41 06 "+res);
    send_response(res);
  }
  else if (str == "010C"){ // Rpm
    Serial.println("Requested RPM");
    String res = randomValue(2, 0.05);
    if (res != "CAN ERROR")
      res = format("41 0C "+res);              
    send_response(res);
  }
  else if (str == "010D"){ // Speed
    Serial.println("Requested Speed");
    
    String res = format("41 0D 05");
    send_response(res);
  }
  else if (str == "0110"){ // Mass Air flow g/s
    Serial.println("Requested Mass air flow");
    String res = format("41 10 00 F4");
    send_response(res);
  }
  else if (str == "0111"){ // throttle rate
    Serial.println("Requested Throttle");
    String res = randomValue(1, 0.05);
    if (res != "CAN ERROR")
      res = format("41 11 "+res);
    send_response(res);
  }

  else if (str == "011C"){ // OBD Standard
    Serial.println("Requested OBD Standard");
    String res = format("41 1C 06");
    Serial.println(res);
    send_response(res);
  }
  else if (str == "015E"){ // fuel rate
    Serial.println("Requested Fuel rate");
    String res = randomValue(1, 0.05);
    if (res != "CAN ERROR")
      res = format("41 5E "+res);
    send_response(res);
  }
  else {
    send_response("NO DATA");
  }
}

void mode09_command(String str){
  if(str == "0902"){
    Serial.println("Requested VIN, serving a const reply");
    String rows[] = {
    "014",
    "0: 49 02 01 57 46 30",
    "1: 44 41 42 43 44 45 46",
    "2: 47 48 30 31 32 33 34"      
    };
    for(int i=0; i < 4; i++){
      String row = format(rows[i]);
      send_response(row, i==3);
    }
  }
  else {
    send_response("NO DATA");
  }
}


void loop() {
  if (SerialBT.available()) {
    msg_len = 0;
    char car;
    do {
      car = (char) SerialBT.read();
      if (car != ' ' and car != '\r')
        buffer[msg_len++] = car;
    } while (car != '\r');
    if (msg_len > 0){
      String str = String((char*)buffer).substring(0, msg_len);
      str.toUpperCase();
      Serial.printf("rx: %s\r\n", str);
      
      if (E){
        send_response(format(str));
      }
      
      if (str.substring(0,2) == "AT"){
        at_command(str);
      }
      else{
        auto_protocol();

        bool protocol_ok = check_protocol();
        
        if (protocol_ok){
          if (str.substring(0,2) == "01"){
            mode01_command(str);
          }
          else if (str.substring(0,2) == "09"){
            mode09_command(str);
          }
          else
            send_response("NO DATA");
        }
      }
      if (SerialBT.available()>0){
        send_response("STOPPED");
      }

      SerialBT.write('\r');
      if (L){
        SerialBT.write('>');
        Serial.println("tx: >");
      }
    }
  }

  if (Serial.available()) {
    msg_len = 0;
    while (Serial.available()) {
      buffer[msg_len++] = Serial.read();
    }
    Serial.printf("Send new message of len %d :\r\n", msg_len);
    SerialBT.write(buffer, msg_len);
  }
  delay(2);
}
