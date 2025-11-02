#include <Wire.h>
#include <Adafruit_AMG88xx.h>  // Cảm biến nhiệt AMG8833
#include <Adafruit_SSD1306.h>  // OLED
#include <Adafruit_GFX.h>
#include <IRremote.h>           // Module IR KY-005
#include <Adafruit_BME280.h>    // Cảm biến môi trường BME280

// ====== Cấu hình OLED ======
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ====== Chân kết nối ======
#define IR_SEND_PIN 3           // KY-005 nối D3
// I2C: SDA -> A4, SCL -> A5 (AMG8833 và BME280 cùng bus I2C)

// ====== Khai báo cảm biến ======
Adafruit_AMG88xx amg;
Adafruit_BME280 bme;
IRsend irsend;

// ====== Ngưỡng ======
float tempBodyThresholds[] = {35.0, 36.0, 37.0, 37.5, 38.0};
int acTemps[] = {28, 27, 26, 25, 24}; // nhiệt độ điều hòa
float envHotThreshold = 4.0; // chênh lệch nhiệt > 4°C → người hơi nóng

void setup() 
{
  Serial.begin(9600);
  Wire.begin();

  // Khởi tạo AMG8833
  if(!amg.begin()){
    Serial.println("Loi ket noi AMG8833!");
    while(1);
  }

  // Khởi tạo BME280
  if(!bme.begin(0x76)){
    Serial.println("Loi ket noi BME280!");
    while(1);
  }

  // Khởi tạo OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("Loi khoi tao OLED!");
    while(1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Khoi dong he thong...");
  display.display();
  delay(1000);

  // Khởi tạo KY-005
  irsend.begin(3);
  Serial.println("IR KY-005 san sang!");
}

void loop() {
  float pixels[64];
  float sum = 0;

  // Đọc AMG8833
  amg.readPixels(pixels);
  for(int i=0;i<64;i++) sum += pixels[i];
  float bodyTemp = sum/64.0;

  // Đọc BME280
  float envTemp = bme.readTemperature();

  // Xác định trạng thái người
  String personState;
  int acTemp;

  if(bodyTemp >= 38.0){
    personState = "Nguoi sot!";
    acTemp = -1; // tắt điều hòa
  } else {
    float diff = bodyTemp - envTemp;
    if(diff > envHotThreshold){
      personState = "Nguoi hoi nong!";
      acTemp = determineACTemp(bodyTemp); // giảm điều hòa
    } else {
      personState = "Nguoi on dinh";
      acTemp = determineACTemp(bodyTemp); // nhiệt độ bình thường
    }
  }

  // Hiển thị OLED
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Nhiet do co the: "); display.println(bodyTemp,1);
  display.print("Nhiet do moi truong: "); display.println(envTemp,1);
  display.print("Trang thai: "); display.println(personState);
  if(acTemp==-1) display.println("Dieu hoa: Tat");
  else {
    display.print("Dieu hoa: "); display.print(acTemp); display.println("C");
  }
  display.display();

  // Serial Monitor
  Serial.print("Nhiet do co the: "); Serial.println(bodyTemp,1);
  Serial.print("Nhiet do moi truong: "); Serial.println(envTemp,1);
  Serial.print("Trang thai: "); Serial.println(personState);
  if(acTemp==-1) Serial.println("Dieu hoa: Tat");
  else Serial.print("Dieu hoa: "); Serial.println(acTemp);

  // Gửi lệnh IR
  sendACCommand(acTemp);

  delay(3000);
}

// Hàm xác định nhiệt độ điều hòa theo nhiệt độ cơ thể
int determineACTemp(float bodyTemp){
  if(bodyTemp < tempBodyThresholds[0]) return acTemps[0];
  else if(bodyTemp < tempBodyThresholds[1]) return acTemps[1];
  else if(bodyTemp < tempBodyThresholds[2]) return acTemps[2];
  else if(bodyTemp < tempBodyThresholds[3]) return acTemps[3];
  else return acTemps[4];
}

// Gửi lệnh IR cho KY-005
void sendACCommand(int temp){
  if(temp==-1){
    Serial.println("Nguoi sot, tat dieu hoa!");
    return;
  }

  unsigned long code;
  switch(temp){
    case 28: code=0x20DF10EF; break;
    case 27: code=0x20DF906F; break;
    case 26: code=0x20DF40BF; break;
    case 25: code=0x20DFC03F; break;
    case 24: code=0x20DF807F; break;
    default: code=0x20DF10EF; break;
  }

  irsend.sendNEC(code,32);
  Serial.print("Gui lenh IR: "); Serial.println(temp);
}
