#include <IRremote.h>
#include <EEPROM.h>
int RECV_PIN = 12; // įvesties pinas, kuriuo priimami IR signalai
IRrecv irrecv(RECV_PIN);
decode_results results;

#include <Servo.h> 
 
Servo myservo1;  // create servo object to control a Roboclaw channel
Servo myservo2;  // create servo object to control a Roboclaw channel
 

int centras_pulse = 1520;
int min_pulse = 1200;
int max_pulse = 1900;


const byte program_led = 13;  //RX LED

//////////////////// JSUMO pultelio kodai //////////////////////////
int kanalArray[11][4] = {
    { 2559, 2438, 2439, 2440 }, // Kanalo parinkimas, Ready, Start, Stop kodai
    { 2687, 2566, 2567, 2568 },
    { 2815, 2694, 2695, 2696 },
    { 2943, 2822, 2823, 2824 },
    { 3071, 2950, 2951, 2952 },
    { 3199, 3078, 3079, 3080 },
    { 3327, 3206, 3207, 3208 },
    { 3455, 3334, 3335, 3336 },
    { 3583, 3462, 3463, 3464 },
    { 3711, 3590, 3591, 3592 },
    { 3839, 3718, 3719, 3720 }
};


int pasirinktas_kanalas = 0;
bool paleista = false;
bool sustabdyta = false;

////////////////////////////////////////////////////////////////////

//LINE begin
const byte LINE_LEFT = A3;
const byte LINE_RIGHT = A2;
//LINE end


//FRONT SENSOR begin
const byte FD90 = 2;
const byte FD65 = 3;
const byte FD45 = 4;
const byte CC = 5;
const byte FL45 = 6;
const byte FL65 = 7;
const byte FL90 = 8;
//FRONT SENSOR end

//START MODULE begin
const byte START_MODULE = 11;
//START MODULE end

//SENSORS SUMS
byte line_sum = 0;
byte sharp_sum = 0;

//MOTOR CONTROL VARIABLES
double INTEGRAL = 0;
int V = 0;

//MOTORS begin
  int S1_pin = 9;
  int S2_pin = 10;
//MOTORS end

//VALDYMO KINTAMIEJI begin
const byte SPEED_STOP = 0;
 byte SPEED_NORMAL = 80;//80
 byte Default_speed = SPEED_NORMAL;//80
const byte SPEED_ATTACK2 = 180;//180
const byte SPEED_ATTACK = 255;//255

const byte SPEED_GO_BACK_LINE = 255;//255
const byte SPEED_GO_BACK_LINE_TURN = 230;//200
const byte SPEED_GO_BACK_LINE_TURN2 = 230;//200

/// LAIKAI ///////////////////////
const unsigned short TIME_FOR_GO_BACK = 300; //80
const unsigned short TIME_FOR_TURNING = 800;
const unsigned short TIME_FOR_TURNING_BOTH = 1000;
unsigned short TURNING_TIME = TIME_FOR_TURNING;
unsigned long LINE_TIME = 2;

//STATES
byte RUN_STATE = 0;
byte LINE_AVOID_STATE = 0;
byte LAST_SEEN = 0;
byte LAST_STATE = 0;
byte STRATEGY_STATE = 0;

//STATE TIMERS
unsigned long LAST_SEEN_TIME = 0;
unsigned long STRATEGY_START_TIME = 0;
unsigned long TIME_FOR_STRATEGY = 0;

unsigned long TIME_FOR_ATACK_SET = 0;
unsigned long TIME = 0;

bool abu_mate = false;
int selectedProgram = -1;

unsigned long valdymo[][3] = {  /// pultelio programu kodai hex formatu
  {0x530DB67C, 0x17B4A228, 0xA90},     // Programu pultelio Start komanda
  {0xCD76C8CB, 0x7BA067FF, 0x5D0}      // Programu pultelio Start komanda
};

unsigned long programos_check[3] = {0x54E680D6, 0x903F952A, 0xA70}; 

unsigned long sensoriu_isjungimas[3] = {0xA74647DD, 0xFDAC0152, 0xDD0}; 
unsigned long sensoriu_ijungimas[3] = {0xD41A2FD5, 0x8243CF09, 0xFD0}; 

byte naudoti_pulta = 0;
bool naudoti_sensorius = true;


byte sukimo_state = 0;    // busena kuri nusako kad suktusi bet skaitytu pilnai sensorius(jei ijungti jie)
unsigned long greicio_perjungimo_interval = 5000;
unsigned long bejudesio = 0;

uint8_t suma_sensoriu = 0; // laikinas kintamasis kol naudojam ne visus 7 priekinius sensorius.

void setup() { 
  
  myservo1.attach(S1_pin);  // attaches the RC signal on pin 5 to the servo object 
  myservo2.attach(S2_pin);  // attaches the RC signal on pin 6 to the servo object 
  delay(1000);
  Stop();
  delay(200);

  Serial.begin(9600);
  pinMode(LINE_LEFT, INPUT);
  pinMode(LINE_RIGHT, INPUT);

  pinMode(FD90, INPUT);
  pinMode(FD65, INPUT);
  pinMode(FD45, INPUT);
  pinMode(CC, INPUT);
  pinMode(FL45, INPUT);
  pinMode(FL65, INPUT);
  pinMode(FL90, INPUT);


  pinMode(START_MODULE, INPUT);
  pinMode(program_led,OUTPUT);

  irrecv.enableIRIn(); // įjungti priėmimą
  pasirinktas_kanalas = EEPROM.read(0);


    while(true){
     jsumo_pultelis();
     pultelio_programos();
     if(digitalRead(START_MODULE)){ naudoti_pulta = 0; break; }
     
     if (irrecv.decode(&results)) {
        unsigned long kodas = results.value;
        int jsumo_kodas = IrReceiver.decodedIRData.decodedRawData;
        
        Serial.println(kodas, HEX);
        if(kodas == valdymo[0][0] || kodas == valdymo[0][1] || kodas == valdymo[0][2] || jsumo_kodas == kanalArray[pasirinktas_kanalas][2]){
          naudoti_pulta = 1;
          break;
        }
        
        //delay(1);
      }
      
    }
  //STRATEGY_STATE = 0;
  STRATEGY_START_TIME = millis();
  
} 
 
void loop(){ 
  if (millis() - LINE_TIME > TURNING_TIME) LINE_AVOID_STATE = 0;
  if (millis() - STRATEGY_START_TIME > TIME_FOR_STRATEGY) { STRATEGY_STATE = 0; sukimo_state = 0;}
  

   readSensors();
   switch (naudoti_pulta){
     case 1:
     stop_skaitymas();
     break;
   }

   switch (RUN_STATE) {
    case 0: switch(LINE_AVOID_STATE) {
              case 0: switch(STRATEGY_STATE) {
                        case 0: foward(); break;
                        case 1: TIME_FOR_STRATEGY = 300;
                                left();
                                break;
                                
                        case 2: Mega_sumo_taktika(); break;
                        
                        case 3: TIME_FOR_STRATEGY = 300;
                                right();
                                break;
                      }
                      break;
              case 1: right();  break;
              case 2: left();   break; 
            }
            
            break;
                
    case 1: back();
            delay(TIME_FOR_GO_BACK);
            LINE_AVOID_STATE = 1;
            TIME_FOR_STRATEGY = 0;
            TURNING_TIME = TIME_FOR_TURNING;
            LINE_TIME = millis();
            STRATEGY_STATE = 0;
            break;
            
    case 2: back();
            delay(TIME_FOR_GO_BACK);
            LINE_AVOID_STATE = 2;
            TIME_FOR_STRATEGY = 0;
            TURNING_TIME = TIME_FOR_TURNING;
            LINE_TIME = millis();
            STRATEGY_STATE = 0;
            break;
            
    case 3: back();
            delay(TIME_FOR_GO_BACK);
            LINE_AVOID_STATE = 2;
            TURNING_TIME = TIME_FOR_TURNING_BOTH;
            TIME_FOR_STRATEGY = 0;
            LINE_TIME = millis();
            STRATEGY_STATE = 0;
            break; 

   case 4: right();
           delay(200);
           TIME_FOR_STRATEGY = 0;
           STRATEGY_START_TIME = millis();
           break;
           
   case 5: foward();
           break;

  case 6:  left();
           delay(200);
           TIME_FOR_STRATEGY = 0;
           STRATEGY_START_TIME = millis();
           break;
  }
 
} 

void readSensors() {
  line_sum  = PINC & 0b11;
 // sharp_sum = PIND & 0b01110000;
 //Serial.println(sharp_sum, BIN);

  uint8_t desine = digitalRead(FD45);
  uint8_t centras = digitalRead(CC);
  uint8_t kaire = digitalRead(FL45);
  
  suma_sensoriu  = (kaire << 2) | (centras << 1) | desine;

  switch(suma_sensoriu){
   case 0b001: RUN_STATE = 4; break;
   case 0b010: RUN_STATE = 5; break;
   case 0b100: RUN_STATE = 6; break;
   case 0b110: RUN_STATE = 4; break;
   case 0b011: RUN_STATE = 6; break;
   case 0b111: RUN_STATE = 0; break;
   case 0b000:
         switch(line_sum){
               case 0b10: RUN_STATE = 1; break;
               case 0b01: RUN_STATE = 2; break;
               case 0b11: RUN_STATE = 3; break;
               case 0b00: RUN_STATE = 0; break;
              }
        break;
   }
 delay(1);

}

void foward(){
  myservo1.writeMicroseconds(max_pulse);  //full forward
  myservo2.writeMicroseconds(max_pulse);  //full reverse
}

void back(){
  myservo1.writeMicroseconds(min_pulse);  //full forward
  myservo2.writeMicroseconds(min_pulse);  //full reverse
}

void right(){
  myservo1.writeMicroseconds(min_pulse);  //full forward
  myservo2.writeMicroseconds(max_pulse);  //full reverse
}

void left(){
  myservo1.writeMicroseconds(max_pulse);  //full forward
  myservo2.writeMicroseconds(min_pulse);  //full reverse
}

void Stop(){
  myservo1.writeMicroseconds(centras_pulse);  //full forward
  myservo2.writeMicroseconds(centras_pulse);  //full reverse
}

