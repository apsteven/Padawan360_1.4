 
// =======================================================================================
// /////////////////////////Padawan360 Body Code - Mega I2C v2.0 ////////////////////////////////////
// =======================================================================================
/*

v2.0 Changes:
- Makes left analog stick default drive control stick. Configurable between left or right stick via isLeftStickDrive 

v1.4
by Steven Sloan 
Code for YX5300 sound card added.Use "#define MP3_YX5300" if YX5300 is to be used (Code for Sparkfun MP3 Trigger retained but not tested).  
YX5300 arduino library at https://github.com/MajicDesigns/MD_YX5300
Serial connection to FlthyHP breakout board added.  Removed I2C comms to FlthyHP.
Added additional key combinations for extra sounds and actions.

v1.3 by Andy Smith (locqust)
- added in code to move automation to a function so can leave him chatting without controller being on
- code kindly supplied by Tony (DRK-N1GT)

v1.2 by Andy Smith (locqust)
-added in my list of expanded sound files and their associated command triggers. Sound files taken from BHD's setup

v1.1
by Robert Corvus
- fixed inverse throttle on right turn
- where right turn goes full throttle on slight right and slow when all the way right
- fixed left-shifted deadzone for turning
- fixed left turn going in opposite direction
- fixed default baud rate for new syren10
- clarified variable names

V1.0
by Dan Kraus
dskraus@gmail.com
Astromech: danomite4047
Project Site: https://github.com/dankraus/padawan360/

Heavily influenced by DanF's Padwan code which was built for Arduino+Wireless PS2
controller leveraging Bill Porter's PS2X Library. I was running into frequent disconnect
issues with 4 different controllers working in various capacities or not at all. I decided
that PS2 Controllers were going to be more difficult to come by every day, so I explored
some existing libraries out there to leverage and came across the USB Host Shield and it's
support for PS3 and Xbox 360 controllers. Bluetooth dongles were inconsistent as well
so I wanted to be able to have something with parts that other builder's could easily track
down and buy parts even at your local big box store.

Hardware:
***Arduino Mega 2560***
USB Host Shield from circuits@home
Microsoft Xbox 360 Controller
Microsoft Xbox 360 Chatpad (Requires https://github.com/willtoth/USB_Host_Shield_2.0/tree/xbox-chatpad version of USB_Host_Shield_2.0 Library)
Xbox 360 USB Wireless Reciver
Sabertooth Motor Controller
Syren Motor Controller
Sparkfun MP3 Trigger

This sketch supports I2C and calls events on many sound effect actions to control lights and sounds.
It is NOT set up for Dan's method of using the serial packet to transfer data up to the dome
to trigger some light effects.If you want that, you'll need to reference DanF's original Padawan code.

It uses Hardware Serial pins on the Mega to control Sabertooth and Syren

Set Sabertooth 2x25/2x12 Dip Switches 1 and 2 Down, All Others Up
For SyRen Simple Serial Set Switches 1 and 2 Down, All Others Up
For SyRen Packetized Serial Set Switchs 1, 2 & 4 Down, All Others Up
Placed a 10K ohm resistor between S1 & GND on the SyRen 10 itself

Pins in use
1 = MP3 Trigger Rx
3 = EXTINGUISHER relay pin
5 = Rx to YX5300 MP3 player
6 = Tx to YX5300 MP3 player

14 = Serial3 (Tx3) = FlthyHP 
15 = Serial3 (Rx3) = FlthyHP
16 = Serial2 (Tx2) = Syren10 S1
18 = Serial1 (Tx1) = Sabertooth 2x25 S1
19 = Serial1 (Rx1) = Sabertooth 2x25 S2

20 = SDA (I2C Comms)
21 = SCL (I2C Comms)

*/

//************************** Set speed and turn speeds here************************************//

// SPEED AND TURN SPEEDS
//set these 3 to whatever speeds work for you. 0-stop, 127-full speed.
const byte DRIVESPEED1 = 50;
// Recommend beginner: 50 to 75, experienced: 100 to 127, I like 100.
// These may vary based on your drive system and power system
const byte DRIVESPEED2 = 100;
//Set to 0 if you only want 2 speeds.
const byte DRIVESPEED3 = 127;

// Default drive speed at startup
byte drivespeed = DRIVESPEED1;

// the higher this number the faster the droid will spin in place, lower - easier to control.
// Recommend beginner: 40 to 50, experienced: 50 $ up, I like 70
// This may vary based on your drive system and power system
const byte TURNSPEED = 40;

// Set isLeftStickDrive to true for driving  with the left stick
// Set isLeftStickDrive to false for driving with the right stick (legacy and original configuration)
boolean isLeftStickDrive = true; 

// If using a speed controller for the dome, sets the top speed. You'll want to vary it potenitally
// depending on your motor. My Pittman is really fast so I dial this down a ways from top speed.
// Use a number up to 127 for serial
const byte DOMESPEED = 127;

// Ramping- the lower this number the longer R2 will take to speedup or slow down,
// change this by incriments of 1
const byte RAMPING = 5;

// Compensation is for deadband/deadzone checking. There's a little play in the neutral zone
// which gets a reading of a value of something other than 0 when you're not moving the stick.
// It may vary a bit across controllers and how broken in they are, sometimex 360 controllers
// develop a little bit of play in the stick at the center position. You can do this with the
// direct method calls against the Syren/Sabertooth library itself but it's not supported in all
// serial modes so just manage and check it in software here
// use the lowest number with no drift
// DOMEDEADZONERANGE for the left stick, DRIVEDEADZONERANGE for the right stick
const byte DOMEDEADZONERANGE = 20;
const byte DRIVEDEADZONERANGE = 20;


// Set the baud rate for the Sabertooth motor controller (feet)
// 9600 is the default baud rate for Sabertooth packet serial.
// for packetized options are: 2400, 9600, 19200 and 38400. I think you need to pick one that works
// and I think it varies across different firmware versions.
const int SABERTOOTHBAUDRATE = 9600;

// Set the baud rate for the Syren motor controller (dome)
// for packetized options are: 2400, 9600, 19200 and 38400. I think you need to pick one that works
// and I think it varies across different firmware versions.
const int DOMEBAUDRATE = 9600;

String hpEvent = "";
char char_array[11];

// I have a pin set to pull a relay high/low to trigger my upside down compressed air like R2's extinguisher
#define EXTINGUISHERPIN 3

#include <Sabertooth.h>
#include <MP3Trigger.h>
#include <Wire.h>
#include <XBOXRECV.h>

#include <Servos.h>
#include <SoftwareSerial.h>
#include <SyRenSimplified.h>
#include <Adafruit_PWMServoDriver.h>

#include <MD_YX5300.h> // https://github.com/MajicDesigns/MD_YX5300

/////////////////////////////////////////////////////////////////
//Serial connection to Flthys HP's
#define FlthyTXPin 14 // OR OTHER FREE DIGITAL PIN
#define FlthyRXPin 15 // OR OTHER FREE DIGITAL PIN  // NOT ACTUALLY USED, BUT NEEDED FOR THE SOFTWARESERIAL FUNCTION
const int FlthyBAUD = 9600; // OR SET YOUR OWN BAUD AS NEEDED
SoftwareSerial FlthySerial(FlthyRXPin, FlthyTXPin); 

/////////////////////////////////////////////////////////////////
Sabertooth Sabertooth2x(128, Serial1);
Sabertooth Syren10(128, Serial2);

// Satisfy IDE, which only needs to see the include statement in the ino.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif

// Set some defaults for start up
// 0 = full volume, 255 off
byte vol = 20;  //Sparkfun MP3 player volume
// false = drive motors off ( drive stick disabled ) at start
boolean isDriveEnabled = false;

// Automated functionality
// Used as a boolean to turn on/off automated functions like periodic random sounds and periodic dome turns
boolean isInAutomationMode = false;
unsigned long automateMillis = 0;
byte automateDelay = random(5, 20); // set this to min and max seconds between sounds
//How much the dome may turn during automation.
int turnDirection = 20;
// Action number used to randomly choose a sound effect or a dome turn
byte automateAction = 0;

char driveThrottle = 0; 
//char rightStickValue = 0; 
int throttleStickValue = 0;
int domeThrottle = 0; //int domeThrottle = 0; //ssloan
char turnThrottle = 0; 

boolean firstLoadOnConnect = false;

AnalogHatEnum throttleAxis;
AnalogHatEnum turnAxis;
AnalogHatEnum domeAxis;
ButtonEnum speedSelectButton;
ButtonEnum hpLightToggleButton;

boolean manuallyDisabledController = false;

// this is legacy right now. The rest of the sketch isn't set to send any of this
// data to another arduino like the original Padawan sketch does
// right now just using it to track whether or not the HP light is on so we can
// fire the correct I2C event to turn on/off the HP light.
//struct SEND_DATA_STRUCTURE{
//  //put your variable definitions here for the data you want to send
//  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
//  int hpl; // hp light
//  int dsp; // 0 = random, 1 = alarm, 5 = leia, 11 = alarm2, 100 = no change
//};
//SEND_DATA_STRUCTURE domeData;//give a name to the group of data

boolean isHPOn = false;

MP3Trigger mp3Trigger;
USB Usb;
XBOXRECV Xbox(&Usb);

/****************** YX5300 Configuration  **********************/
#define MP3_YX5300   //Uncomment if using a YX5300 for sound
// Connections for serial interface to the YX5300 module
#ifdef MP3_YX5300
  const uint8_t YX5300_RX = 5;    // connect to TX of MP3 Player module
  const uint8_t YX5300_TX = 6;    // connect to RX of MP3 Player module
  MD_YX5300 YX5300(YX5300_RX, YX5300_TX); 
  uint16_t volume =12; //YX5300 Start volume
#endif

/***************************************************************/

/************* Xbox360 Chatpad Configuration  ******************/
#define CHATPAD    //Uncomment if using a chatpad 
bool orangeToggle = false;

//=====================================
//   SETUP                           //
//=====================================

void setup() {
//  //Syren10Serial.begin(DOMEBAUDRATE);
//
//  // 9600 is the default baud rate for Sabertooth packet serial.
//  //Sabertooth2xSerial.begin(9600);
//  // Send the autobaud command to the Sabertooth controller(s).
//  //Sabertooth2x.autobaud();
//  /* NOTE: *Not all* Sabertooth controllers need this command.
//  It doesn't hurt anything, but V2 controllers use an
//  EEPROM setting (changeable with the function setBaudRate) to set
//  the baud rate instead of detecting with autobaud.
//  If you have a 2x12, 2x25 V2, 2x60 or SyRen 50, you can remove
//  the autobaud line and save yourself two seconds of startup delay.
//  */
//
//  #if !defined(SYRENSIMPLE)
//    Syren10.setTimeout(950);
//  #endif
//
 
  Serial.begin(9600);
  Serial1.begin(SABERTOOTHBAUDRATE);
  Serial2.begin(DOMEBAUDRATE);

#if defined(SYRENSIMPLE)
  Syren10.motor(0);
#else
  Syren10.autobaud();
#endif

  //Flthy HP
  FlthySerial.begin(FlthyBAUD);
  
  // Send the autobaud command to the Sabertooth controller(s).
  /* NOTE: *Not all* Sabertooth controllers need this command.
  It doesn't hurt anything, but V2 controllers use an
  EEPROM setting (changeable with the function setBaudRate) to set
  the baud rate instead of detecting with autobaud.
  If you have a 2x12, 2x25 V2, 2x60 or SyRen 50, you can remove
  the autobaud line and save yourself two seconds of startup delay.
  */
  Sabertooth2x.autobaud();
  // The Sabertooth won't act on mixed mode packet serial commands until
  // it has received power levels for BOTH throttle and turning, since it
  // mixes the two together to get diff-drive power levels for both motors.
  Sabertooth2x.drive(0);
  Sabertooth2x.turn(0);


  Sabertooth2x.setTimeout(950);
  Syren10.setTimeout(950);

  pinMode(EXTINGUISHERPIN, OUTPUT);
  digitalWrite(EXTINGUISHERPIN, HIGH);

  #ifdef MP3_YX5300
    // initialize global libraries
    YX5300.begin();
    YX5300.volume(volume);
  #else
    mp3Trigger.setup();
    mp3Trigger.setVolume(vol);  
  #endif

   if(isLeftStickDrive) {
    throttleAxis = LeftHatY;
    turnAxis = LeftHatX;
    domeAxis = RightHatX;
    speedSelectButton = L3;
    hpLightToggleButton = R3;

  } else {
    throttleAxis = RightHatY;
    turnAxis = RightHatX;
    domeAxis = LeftHatX;
    speedSelectButton = R3;
    hpLightToggleButton = L3;
  }

  // Start I2C Bus. The body is the master.
  Wire.begin();

  Play_Sound(23);

  // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  while (!Serial);
    if (Usb.Init() == -1) {
      //Serial.print(F("\r\nOSC did not start"));
      while (1); //halt
    }
  //Serial.print(F("\r\nXbox Wireless Receiver Library Started"));

}

//============================
//    MAIN LOOP             //
//============================

void loop() {
  Usb.Task();
  // if we're not connected, return so we don't bother doing anything else.
  // set all movement to 0 so if we lose connection we don't have a runaway droid!
  // a restraining bolt and jawa droid caller won't save us here!
  if (!Xbox.XboxReceiverConnected || !Xbox.Xbox360Connected[0]) {
    Sabertooth2x.drive(0);
    Sabertooth2x.turn(0);
    Syren10.motor(1, 0);
    firstLoadOnConnect = false;
    // If controller is disconnected, but was in automation mode, then droid will continue
    // to play random sounds and dome movements
    if(isInAutomationMode){
      triggerAutomation();
    }
    if(!manuallyDisabledController){     
    }
    return;
  }

  // After the controller connects, Blink all the LEDs so we know drives are disengaged at start
  if (!firstLoadOnConnect) {
    firstLoadOnConnect = true;
     Play_Sound(21);
    Xbox.setLedMode(ROTATING, 0);
    manuallyDisabledController=false;
    //triggerI2C(10, 0);
    //domeData.ctl = 1; domeData.dsp = 0; ET.sendData();  
    //Tell the dome that the controller is now connected
  }
  
  if (Xbox.getButtonClick(XBOX, 0)) {
    if(Xbox.getButtonPress(L1, 0) && Xbox.getButtonPress(R1, 0)){ 
      manuallyDisabledController=true;
      Xbox.disconnect(0);
    }
  }

  // enable / disable right stick (droid movement) & play a sound to signal motor state
  if (Xbox.getButtonClick(START, 0)) {
    if (isDriveEnabled) {
      isDriveEnabled = false;
      Xbox.setLedMode(ROTATING, 0);
      Play_Sound(53); // 3 Beeps
    } else {
      isDriveEnabled = true;
      Play_Sound(52); // 2 Beeps
      // //When the drive is enabled, set our LED accordingly to indicate speed
      if (drivespeed == DRIVESPEED1) {
        Xbox.setLedOn(LED1, 0);
      } else if (drivespeed == DRIVESPEED2 && (DRIVESPEED3 != 0)) {
        Xbox.setLedOn(LED2, 0);
      } else {
        Xbox.setLedOn(LED3, 0);
      }
    }
  }

  //Toggle automation mode with the BACK button
  if (Xbox.getButtonClick(BACK, 0)) {
    if (isInAutomationMode) {
      isInAutomationMode = false;
      automateAction = 0;
      Play_Sound(53); // 3 Beeps
      FlthySerial.print("S4\r");//S4 function ssloan
    } else {
      isInAutomationMode = true;
      Play_Sound(52); // 2 Beeps
      FlthySerial.print("S6\r");//S6 function
    }
  }

  // Plays random sounds or dome movements for automations when in automation mode
  if (isInAutomationMode) {
    triggerAutomation();
}

  // Volume Control of MP3 Trigger
  // Hold R1 and Press Up/down on D-pad to increase/decrease volume
  if (Xbox.getButtonClick(DOWN, 0)) {
    // volume down
    if (Xbox.getButtonPress(R1, 0)) {
      
       #ifdef MP3_YX5300
           YX5300.volumeDec();
      #else    
        if (vol > 0) {
          vol--;
          mp3Trigger.setVolume(vol);  
        }
      #endif
      
    }
  }
  if (Xbox.getButtonClick(UP, 0)) {
    //volume up
    if (Xbox.getButtonPress(R1, 0)) {

       #ifdef MP3_YX5300
           YX5300.volumeInc();
      #else      
        if (vol < 255) {
          vol++;
          mp3Trigger.setVolume(vol);  
        }
      #endif
      
    }
  }

  // Logic display brightness.
  // Hold L1 and press up/down on dpad to increase/decrease brightness
  if (Xbox.getButtonClick(UP, 0)) {
    if (Xbox.getButtonPress(L1, 0)) {
      triggerI2C(10, 24);
    }
  }
  if (Xbox.getButtonClick(DOWN, 0)) {
    if (Xbox.getButtonPress(L1, 0)) {
      triggerI2C(10, 25);
    }
  }


  //FIRE EXTINGUISHER
  // When holding L2-UP, extinguisher is spraying. When released, stop spraying

  // TODO: ADD SERVO DOOR OPEN FIRST. ONLY ALLOW EXTINGUISHER ONCE IT'S SET TO 'OPENED'
  // THEN CLOSE THE SERVO DOOR
  if (Xbox.getButtonPress(L1, 0)) {
    if (Xbox.getButtonPress(UP, 0)) {
      digitalWrite(EXTINGUISHERPIN, LOW);
    } else {
      digitalWrite(EXTINGUISHERPIN, HIGH);
    }
  }

#if defined(CHATPAD)
  Check_Chatpad();
#endif

  // Y Button and Y combo buttons
  if (Xbox.getButtonClick(Y, 0)) {
     if (Xbox.getButtonPress(L1, 0) && Xbox.getButtonPress(R1, 0)) {
       //Addams
       Play_Sound(168); // Addams family tune 53s
       //logic lights
       triggerI2C(10, 19);
       //HPEvent Disco for 53s
       FlthySerial.print("A0040|53\r");
       //Magic Panel event - Flash Q
       triggerI2C(20, 28);
     } else if (Xbox.getButtonPress(L1, 0)) {
        //Annoyed
        Play_Sound(8); // Annoyed sounds
        //logic lights, random
        triggerI2C(10, 0);
    } else if (Xbox.getButtonPress(L2, 0)) {
        //Chortle
        Play_Sound(2); // Chortle
        //logic lights, random
        triggerI2C(10, 0);
    } else if (Xbox.getButtonPress(R1, 0)) {
         //Theme
        Play_Sound(9); // Star Wars Theme 5m 29s
        //logic lights, random
        triggerI2C(10, 0);
        //Magic Panel event - Trace up 1
        triggerI2C(20, 8);
    } else if (Xbox.getButtonPress(R2, 0)) {
        //More Alarms
        Play_Sound(random(56, 71)); // More alarms
        //logic lights, random
        triggerI2C(10, 0);
        //Magic Panel event - FlashAll 5s
        triggerI2C(20, 26);
    } else {
        Play_Sound(random(13,17)); // Alarms
        //logic lights, random
        triggerI2C(10, 0);
        //Magic Panel event - FlashAll 5s
        triggerI2C(20, 26);
    }
  }

  // A Button and A combo Buttons
  if (Xbox.getButtonClick(A, 0)) {
   if (Xbox.getButtonPress(L1, 0) && Xbox.getButtonPress(R1, 0)) {
       //Gangnam
       Play_Sound(169); // Gangam Styles 24s
       //logic lights
       triggerI2C(10, 18);
       //HPEvent Disco for 24s
       FlthySerial.print("A0040|24\r");
       //Magic Panel event - Flash Q
       triggerI2C(20, 28);
   } else if (Xbox.getButtonPress(L1, 0)) {
       //shortcircuit
       Play_Sound(6); // Short Circuit
       //logic lights
       triggerI2C(10, 6);
       FlthySerial.print("A0070|5\r");
       //Magic Panel event - Fade Out
       triggerI2C(20, 25);
    } else if (Xbox.getButtonPress(L2, 0)) {
       //scream
       Play_Sound(1); // Scream
       //logic lights, alarm
       triggerI2C(10, 1);
       //HPEvent pulse Red for 4 seconds
       FlthySerial.print("A0031|4\r");
       //Magic Panel event - Alert 4s
       triggerI2C(20, 6);
    } else if (Xbox.getButtonPress(R1, 0)) {
       //Imperial March
       Play_Sound(11); // Imperial March 3m 5s
       //logic lights, alarm2Display
       triggerI2C(10, 11);
       //HPEvent - flash - I2C
       FlthySerial.print("A0030|175\r");
       //magic Panel event - Flash V
       triggerI2C(20, 27);
    } else if (Xbox.getButtonPress(R2, 0)) {
       //More Misc
       Play_Sound(random(72,99));  // Misc Sounds
       //logic lights, random
       triggerI2C(10, 0);
    } else {
       //Misc noises
       Play_Sound(random(17,25)); // More Misc Sounds
       //logic lights, random
       triggerI2C(10, 0);
    }
  }

  // B Button and B combo Buttons
  if (Xbox.getButtonClick(B, 0)) {
     if (Xbox.getButtonPress(L1, 0) && Xbox.getButtonPress(R1, 0)) {
        //Muppets
        Play_Sound(173); // Muppets Tune 30s
        //logic lights
        triggerI2C(10, 17);
        //HPEvent Disco for 30s
        FlthySerial.print("A0040|30\r");
        //Magic Panel event - Trace Up 1
        triggerI2C(20, 8);
     } else if (Xbox.getButtonPress(L1, 0)) {
        //patrol
        Play_Sound(7); // Quiet Beeps
        //logic lights, random
        triggerI2C(10, 0);
    } else if (Xbox.getButtonPress(L2, 0)) {
        //DOODOO
        Play_Sound(3); // DOODOO
       //logic lights, random
        triggerI2C(10, 0);
       //Magic Panel event - One loop sequence
        triggerI2C(20, 30);
    } else if (Xbox.getButtonPress(R1, 0)) {
       //Cantina
       Play_Sound(10); // Cantina 2m 50s
       //logic lights bargraph
       triggerI2C(10, 10);
       // HPEvent 1 - Cantina Music - Disco - I2C
       FlthySerial.print("A0040|165\r");
       //magic Panel event - Trace Down 1
       triggerI2C(20, 10);
    } else if (Xbox.getButtonPress(R2, 0)) {
       //Proc/Razz
       Play_Sound(random(100, 138)); // Proc/Razz
       //logic lights random
       triggerI2C(10, 10);
    } else {
       //Sent/Hum
       Play_Sound(random(32,52)); //Sent/Hum
       //logic lights, random
       triggerI2C(10, 0);
       //Magic Panel event - Expand 2
       triggerI2C(20, 17);
    }
  }

  // X Button and X combo Buttons
  if (Xbox.getButtonClick(X, 0)) {
   if (Xbox.getButtonPress(L1, 0) && Xbox.getButtonPress(R1, 0)) {
       //Leia Short
       Play_Sound(170); // Leia Short 6s
       //logic lights
       triggerI2C(10, 16);
       //HPEvent hologram for 6s
       FlthySerial.print("S1|6\r"); 
       //FlthySerial.print("R1014\r");       
       //FlthySerial.print("A001|6\r"); 
       //magic Panel event - Eye Scan
       triggerI2C(20, 23);
    } else if (Xbox.getButtonPress(L2, 0) && Xbox.getButtonPress(R1, 0)) {
       //Luke message
       Play_Sound(171); // Luke Message 26s
       //logic lights
       triggerI2C(10, 15);
       //HPEvent hologram for 26s
       FlthySerial.print("S1|26\r");
       //FlthySerial.print("F001|26\r");
       //magic Panel event - Cylon Row
       triggerI2C(20, 22);
    } else if (Xbox.getButtonPress(L1, 0)) {
       // leia message L1+X
       Play_Sound(5); // Leia Long 35s
       //logic lights, leia message
       triggerI2C(10, 5);
       // Front HPEvent 1 - HoloMessage leia message 35 seconds
       FlthySerial.print("S1|35\r");
       //FlthySerial.print("F001|35\r");
       //magic Panel event - Cylon Row
       triggerI2C(20, 22);
    } else if (Xbox.getButtonPress(L2, 0)) {
       //WolfWhistle
       Play_Sound(4); // Wolf whistle
       //logic lights
       triggerI2C(10, 4);
       FlthySerial.print("A00312|5\r");
       //magic Panel event - Heart
       triggerI2C(20, 40);
       // CBI panel event - Heart
       triggerI2C(30, 4);    
    } else if (Xbox.getButtonPress(R1, 0)) {
       //Duel of the Fates 
       Play_Sound(12); // Duel of Fates 4m 17s
       //logic lights, random
       triggerI2C(10, 0);
       //magic Panel event - Flash Q
       triggerI2C(20, 28);
    } else if (Xbox.getButtonPress(R2, 0)) {
       //Scream/Whistle 
       Play_Sound(random(139, 168));    // Scream/Whistle
       //logic lights, random
       triggerI2C(10, 0);
       //magic Panel event - Compress 2
       triggerI2C(20, 19);
    } else {
       //ohh (Sad Sound)
       Play_Sound(random(25, 31)); // ohh (Sad Sound)  
       //logic lights, random
       triggerI2C(10, 0);
    }
  }

  // turn hp light on & off with Right Analog Stick Press (R3) for left stick drive mode
  // turn hp light on & off with Left Analog Stick Press (L3) for right stick drive mode
  if (Xbox.getButtonClick(hpLightToggleButton, 0))  {
    // if hp light is on, turn it off
    if (isHPOn) {
      isHPOn = false;
      // turn hp light off
      // Disable LED and HP sequences
      FlthySerial.print("A098\r");
      FlthySerial.print("A198\r");
    } else {
      isHPOn = true;
      // turn hp light on
      // Enable LED and HP sequences
      FlthySerial.print("A099\r");
      FlthySerial.print("A199\r");
    }
  }


  // Change drivespeed if drive is enabled
  // Press Left Analog Stick (L3) for left stick drive mode
  // Press Right Analog Stick (R3) for right stick drive mode
  // Set LEDs for speed - 1 LED, Low. 2 LED - Med. 3 LED High
  if (Xbox.getButtonClick(speedSelectButton, 0) && isDriveEnabled) {
    //if in lowest speed
    if (drivespeed == DRIVESPEED1) {
      //change to medium speed and play sound 3-tone
      drivespeed = DRIVESPEED2;
      Xbox.setLedOn(LED2, 0);
      Play_Sound(53); // 3 Beeps
      triggerI2C(10, 22);
      //magic Panel event - AllOn 5s
      triggerI2C(20, 3);
    } else if (drivespeed == DRIVESPEED2 && (DRIVESPEED3 != 0)) {
      //change to high speed and play sound scream
      drivespeed = DRIVESPEED3;
      Xbox.setLedOn(LED3, 0);
      Play_Sound(1);  // Scream
      triggerI2C(10, 23);
      //magic Panel event - AllOn 10s
      triggerI2C(20, 4);      
    } else {
      //we must be in high speed
      //change to low speed and play sound 2-tone
      drivespeed = DRIVESPEED1;
      Xbox.setLedOn(LED1, 0);
      Play_Sound(52); // 2 Beeps
      triggerI2C(10, 21);
      //magic Panel event - AllOn 2s
      triggerI2C(20, 2);
    }
  }


  // FOOT DRIVES
  // Xbox 360 analog stick values are signed 16 bit integer value
  // Sabertooth runs at 8 bit signed. -127 to 127 for speed (full speed reverse and  full speed forward)
  // Map the 360 stick values to our min/max current drive speed
  throttleStickValue = (map(Xbox.getAnalogHat(throttleAxis, 0), -32768, 32767, -drivespeed, drivespeed));
  if (throttleStickValue > -DRIVEDEADZONERANGE && throttleStickValue < DRIVEDEADZONERANGE) {  
    // stick is in dead zone - don't drive
    driveThrottle = 0;
  } else {
    if (driveThrottle < throttleStickValue) {
      if (throttleStickValue - driveThrottle < (RAMPING + 1) ) {
        driveThrottle += RAMPING;
      } else {
         driveThrottle = throttleStickValue;
      }
    } else if (driveThrottle > throttleStickValue) {
      if (driveThrottle - throttleStickValue < (RAMPING + 1) ) {
        driveThrottle -= RAMPING;
      } else {
        driveThrottle = throttleStickValue;
      }
    }
  }

  turnThrottle = map(Xbox.getAnalogHat(turnAxis, 0), -32768, 32767, -TURNSPEED, TURNSPEED);

  // DRIVE!
  // right stick (drive)
  if (isDriveEnabled) {
    // Only do deadzone check for turning here. Our Drive throttle speed has some math applied
    // for RAMPING and stuff, so just keep it separate here
    if (turnThrottle > -DRIVEDEADZONERANGE && turnThrottle < DRIVEDEADZONERANGE) {
      // stick is in dead zone - don't turn
      turnThrottle = 0;
    }
    Sabertooth2x.turn(-turnThrottle);
    Sabertooth2x.drive(driveThrottle);
  }

  // DOME DRIVE!
          domeThrottle = (map(Xbox.getAnalogHat(domeAxis, 0), -32768, 32767, DOMESPEED, -DOMESPEED));
          
          if (domeThrottle > -DOMEDEADZONERANGE && domeThrottle < DOMEDEADZONERANGE) {
            //stick in dead zone - don't spin dome
            domeThrottle = 0;
          }
        
          Syren10.motor(1, domeThrottle);


} // END loop()

 void triggerI2C(byte deviceID, byte eventID) {
  Wire.beginTransmission(deviceID);
  Wire.write(eventID);
  Wire.endTransmission();
}

void triggerAutomation(){
  // Plays random sounds or dome movements for automations when in automation mode
    unsigned long currentMillis = millis();

    if (currentMillis - automateMillis > (automateDelay * 1000)) {
      automateMillis = millis();
      automateAction = random(1, 5);

      if (automateAction > 1) {
        Play_Sound(random(32, 52)); // Sent/Hum
      }
      if (automateAction < 4) {

      //************* Move the dome for 750 msecs  **************
        
      #if defined(SYRENSIMPLE)
        Syren10.motor(turnDirection);
      #else
        Syren10.motor(1, turnDirection);
      #endif

        delay(750);

        //************* Stop the dome motor **************
      #if defined(SYRENSIMPLE)
        Syren10.motor(0);
      #else
        Syren10.motor(1, 0);
      #endif

        //************* Change direction for next time **************
        if (turnDirection > 0) {
          turnDirection = -45;
        } else {
          turnDirection = 45;
        }
      }

      // sets the mix, max seconds between automation actions - sounds and dome movement
      automateDelay = random(5,15);
    }
}

void Check_Chatpad() {

  // GENERAL SOUND PLAYBACK AND DISPLAY CHANGING
  if (Xbox.getChatpadModifierClick(MODIFER_ORANGEBUTTON)) {
    orangeToggle = !orangeToggle;
    Xbox.setChatpadLed(CHATPADLED_ORANGE, orangeToggle);
  }

  if (!orangeToggle) {

    if (Xbox.getChatpadClick(XBOX_CHATPAD_D1, 0)) {
       //Duel of the Fates 
       Play_Sound(12); // Duel of Fates 4m 17s
       //logic lights, random
       triggerI2C(10, 0);
       //magic Panel event - Flash Q
       triggerI2C(20, 28);
    }   

    if (Xbox.getChatpadClick(XBOX_CHATPAD_D2, 0)) {
         //Theme
        Play_Sound(9); // Star Wars Theme 5m 29s
        //logic lights, random
        triggerI2C(10, 0);
        //Magic Panel event - Trace up 1
        triggerI2C(20, 8);
    }   

    if (Xbox.getChatpadClick(XBOX_CHATPAD_D3, 0)) {
       //Imperial March
       Play_Sound(11); // Imperial March 3m 5s
       //logic lights, alarm2Display
       triggerI2C(10, 11);
       //HPEvent - flash - I2C
       FlthySerial.print("A0030|175\r");
       //magic Panel event - Flash V
       triggerI2C(20, 27);
    }   
    
    if (Xbox.getChatpadClick(XBOX_CHATPAD_D4, 0)) {
       //Cantina
       Play_Sound(10); // Cantina 2m 50s
       //logic lights bargraph
       triggerI2C(10, 10);
       // HPEvent 1 - Cantina Music - Disco - I2C
       FlthySerial.print("A0040|165\r");
       //magic Panel event - Trace Down 1
       triggerI2C(20, 10);
    }   

    if (Xbox.getChatpadClick(XBOX_CHATPAD_D8, 0)) {
       //Muppets
        Play_Sound(173); // Muppets Tune 30s
        //logic lights
        triggerI2C(10, 17);
        //HPEvent Disco for 30s
        FlthySerial.print("A0040|30\r");
        //Magic Panel event - Trace Up 1
        triggerI2C(20, 8);
    }   

    if (Xbox.getChatpadClick(XBOX_CHATPAD_D9, 0)) {
       //Addams
       Play_Sound(168); // Addams family tune 53s
       //logic lights
       triggerI2C(10, 19);
       //HPEvent Disco for 53s
       FlthySerial.print("A0040|53\r");
       //Magic Panel event - Flash Q
       triggerI2C(20, 28);
    }   

    if (Xbox.getChatpadClick(XBOX_CHATPAD_D0, 0)) {
       //Gangnam
       Play_Sound(169); // Gangam Styles 24s
       //logic lights
       triggerI2C(10, 18);
       //HPEvent Disco for 24s
       FlthySerial.print("A0040|24\r");
       //Magic Panel event - Flash Q
       triggerI2C(20, 28);
    }   

    if (Xbox.getChatpadClick(XBOX_CHATPAD_W, 0)) {
       //WolfWhistle
       Play_Sound(4); // Wolf whistle
       //logic lights
       triggerI2C(10, 4);
       FlthySerial.print("A00312|5\r");
       //magic Panel event - Heart
       triggerI2C(20, 40);
       // CBI panel event - Heart
       triggerI2C(30, 4);    
    }

    if (Xbox.getChatpadClick(XBOX_CHATPAD_O, 0)) {

    }
    
    if (Xbox.getChatpadClick(XBOX_CHATPAD_S, 0)) {
       //ohh (Sad Sound)
       Play_Sound(random(25, 31)); // ohh (Sad Sound)  
       //logic lights, random
       triggerI2C(10, 0);
    }

    if (Xbox.getChatpadClick(XBOX_CHATPAD_D, 0)) {
        //DOODOO
        Play_Sound(3); // DOODOO
       //logic lights, random
        triggerI2C(10, 0);
       //Magic Panel event - One loop sequence
        triggerI2C(20, 30);
    }

    if (Xbox.getChatpadClick(XBOX_CHATPAD_L, 0)) {
       Play_Sound(5); // Leia Long 35s
       //logic lights, leia message
       triggerI2C(10, 5);
       // Front HPEvent 1 - HoloMessage leia message 35 seconds
       FlthySerial.print("S1|35\r");
       //FlthySerial.print("F001|35\r");
       //magic Panel event - Cylon Row
       triggerI2C(20, 22);
    }
    
    if (Xbox.getChatpadClick(XBOX_CHATPAD_X, 0)) {
       //shortcircuit
       Play_Sound(6); // Short Circuit
       //logic lights
       triggerI2C(10, 6);
       FlthySerial.print("A0070|5\r");
       //Magic Panel event - Fade Out
       triggerI2C(20, 25);
    }

    if (Xbox.getChatpadClick(XBOX_CHATPAD_C, 0)) {
        //Chortle
        Play_Sound(2); // Chortle
        //logic lights, random
        triggerI2C(10, 0);
    }
        
  }
  else {
    if (Xbox.getChatpadClick(XBOX_CHATPAD_D1, 0)) {
        Play_Sound(3);
        //logic lights, random
        triggerI2C(10, 0);
    }
  
    if (Xbox.getChatpadClick(XBOX_CHATPAD_L, 0)) {
       //Luke message
       Play_Sound(171); // Luke Message 26s
       //logic lights
       triggerI2C(10, 15);
       //HPEvent hologram for 26s
       FlthySerial.print("S1|26\r");
       //FlthySerial.print("F001|26\r");
       //magic Panel event - Cylon Row
       triggerI2C(20, 22);
    }
  }
  
}

void Play_Sound(int Track_Num) {
 
  #ifdef MP3_YX5300
     YX5300.playTrack(Track_Num);
  #else
     mp3Trigger.play(Track_Num);  
  #endif 
 
}
