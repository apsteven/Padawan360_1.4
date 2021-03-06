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


