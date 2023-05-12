/*******************************************/
/*!
    @file
      companionplus_device.ino
    @libraries
      PN532 library (.h, .cpp) by Adafruit
    @author
      Nest R&D, Lis Miller
    @publishing date
      31st March, 2023
    @special thanks
      Arduino Discord Community
      Staffordshire University, UK
    @license 
      BSD (see license.txt)

      COMPANION PLUS Device script
    This script handles power on/off functions, pedometer
    counter and SPI communication for writing NFC tag
    game data (total steps) to the start of the tag's
    memory.

    The application (Android .apk) manages the game data
    by deducing the first scan's steps (entering walk mode)
    from the second scan's steps (exiting walk mode). This
    allows the device to reset without interrupting game
    progress.

    Always ensure the correct battery is used with the
    device, according to the accompanying README.

    'Melody' sketch (public domain) by Tom Igoe added 4/5/2023
    https://www.arduino.cc/en/Tutorial/BuiltInExamples/toneMelody
    
*/

// Systems
uint8_t devicemode = 0;
uint8_t timeout = 250;
int currentLoop = 0;

// IMU on 33 BLE Sense (REV2)
#include "Arduino_BMI270_BMM150.h"

// Pedometer stats
int flopX = 0;
int flopY = 0;
int flopZ = 0;
uint32_t totalSteps = 0;
float accelerationSense = 25;

// Piezo buzzer tone
#include "pitches.h"

int melody[] = {
  NOTE_A5, NOTE_D6, NOTE_G6, NOTE_A5, NOTE_A5
};

int noteDurations[] = {
  8, 8, 8, 8, 4,
};

// Elechouse PN532 V3 module
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// SPI communication pins
#define PN532_SCK   (13)
#define PN532_MOSI  (11)
#define PN532_SS    (10)
#define PN532_MISO  (12)
#define PN532_IRQ    (8)
#define PN532_RESET  (9)

// Define pins in Adafruit PN532 library nfc function
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Create a URI record to hold a buffer of the current step data
char buffer[12];
char* url = (char*) buffer;
uint8_t ndefprefix = NDEF_URIPREFIX_NONE;

#define NR_SHORTSECTOR          (32)    // Number of short sectors on Mifare 1K/4K
#define NR_LONGSECTOR           (8)     // Number of long sectors on Mifare 4K
#define NR_BLOCK_OF_SHORTSECTOR (4)     // Number of blocks in a short sector
#define NR_BLOCK_OF_LONGSECTOR  (16)    // Number of blocks in a long sector

// Determine the sector trailer block based on sector number
#define BLOCK_NUMBER_OF_SECTOR_TRAILER(sector) (((sector)<NR_SHORTSECTOR)? \
  ((sector)*NR_BLOCK_OF_SHORTSECTOR + NR_BLOCK_OF_SHORTSECTOR-1):\
  (NR_SHORTSECTOR*NR_BLOCK_OF_SHORTSECTOR + (sector-NR_SHORTSECTOR)*NR_BLOCK_OF_LONGSECTOR + NR_BLOCK_OF_LONGSECTOR-1))

// Determine the sector's first block based on the sector number
#define BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector) (((sector)<NR_SHORTSECTOR)? \
  ((sector)*NR_BLOCK_OF_SHORTSECTOR):\
  (NR_SHORTSECTOR*NR_BLOCK_OF_SHORTSECTOR + (sector-NR_SHORTSECTOR)*NR_BLOCK_OF_LONGSECTOR))

// Default Mifare Classic key
static const uint8_t KEY_DEFAULT_KEYAB[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


void setup(void) {
  // Open serial for sending debugging data to serial monitor
  // and delay if board powers on before connecting to IDE
  //Serial.begin(115200);
  //while (!Serial) delay (10);
  pinMode(6, OUTPUT);

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata){
    Serial.print("Didn't find PN532 module!");
    while (1);
  }
  Serial.print("Found PN532 module!");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  PlaySong();
}

void loop(void) {

  if(devicemode == 0){
    UpdateSteps();
    currentLoop++;
    if (currentLoop >= 25){
      devicemode = 1;
    }
  }

  if (devicemode == 1){
    //ReadCard();
    devicemode = 2;
  }
    
  if (devicemode == 2){
    ResetCard();
    devicemode = 3;
  } 

  // Convert totalSteps to char buffer after each card reset
  itoa (totalSteps, buffer, 10);
  
  if (devicemode == 3){
    WriteCard();
    devicemode = 0;
    currentLoop = 0;
  }
}

// Pedometer, runs on every 250m/s
void UpdateSteps (void){
    float x, y, z;

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(x, y, z);
    if(x > accelerationSense || x < -accelerationSense){
      //Serial.println(x);
      //Serial.print('\t');
      flopX = 1;
    }
    if(y > accelerationSense || y < -accelerationSense){
      //Serial.println(y);
      //Serial.print('\t');
      flopY = 1;
    }
    if(z > accelerationSense || z < -accelerationSense){
      //Serial.println(z);
      //Serial.print('\t');
      flopZ = 1;
    }
  if(flopX == 1){
    flopX = 0;
    totalSteps += 1;
  }
  if(flopY == 1){ 
    flopY = 0;
    totalSteps += 1;
  }
  if(flopZ == 1){
    flopZ = 0;
    totalSteps += 1;
  }
  Serial.print("Steps taken: ");
  Serial.println(totalSteps);
  
  }
  delay(250);
}

// Read game card data, add to totalSteps on device (to be completed)
//void ReadCard (void){
//  uint8_t success;                          // Flag to check if there was an error with the PN532
//  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
//  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
//  uint8_t currentblock;                     // Counter to keep track of which block we're on
//  bool authenticated = false;               // Flag to indicate if the sector is authenticated
//  uint8_t data[16];                         // Array to store block data during reads
//  uint32_t szPos;                           // Position of NDEF Message in NFC tag data
//  uint8_t dataPrint;
//
//  uint8_t keyuniversal[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
//
//  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, timeout);
//
//  if (success) {
//    if (uidLength == 4)
//    {
//      for (currentblock = 4; currentblock < 7; currentblock++)
//      {
//        success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);
//        success = nfc.mifareclassic_ReadDataBlock(currentblock, data);
//        if (success){
//          for (szPos = 0; szPos < 16; szPos++) {
//            if (data[szPos] <= 0x2f || data[szPos] >= 0x3a){} 
//            else {
//              // Take 48 (0x30 as a byte or '0' as a character) from each character and convert to unsigned int
//              dataPrint = ((data[szPos]) - 48);
//              Serial.println("totalsteps: " + totalSteps);
//              Serial.println("dataprint: " + dataPrint);
//              Serial.println((uint8_t)(data[szPos]) - 48);
//              
//              totalSteps = totalSteps + dataPrint;
//            }
//          }
//        }
//      }
//    }
//  }
//  //Serial.flush();
//}

// Format card correctly in preparation for NDEF record
void ResetCard (void){
  uint8_t success;                          // Flag to check if there was an error with the PN532
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t blockBuffer[16];                  // Buffer to store block contents
  uint8_t blankAccessBits[3] = { 0xff, 0x07, 0x80 };
  uint8_t idx = 0;
  uint8_t numOfSector = 16;                 // Assume Mifare Classic 1K for now (16 4-block sectors)

  Serial.println("Place your NDEF formatted Mifare Classic 1K card on the reader");

  // Wait for an ISO14443A type card (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, timeout);

  if (success)
  {
    // Display some basic information about the found tag
    Serial.println("Found an ISO14443A card/tag");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    // Make sure this is a Mifare Classic card
    if (uidLength != 4)
    {
      Serial.println("Ooops ... this doesn't seem to be a Mifare Classic card!");
      return;
    }

    Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
    Serial.println("");
    Serial.println("Reformatting card for Mifare Classic (please don't touch it!) ... ");

    // Now run through the card sector by sector
    for (idx = 0; idx < numOfSector; idx++)
    {
      // Step 1: Authenticate the current sector using key B 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, BLOCK_NUMBER_OF_SECTOR_TRAILER(idx), 1, (uint8_t *)KEY_DEFAULT_KEYAB);
      if (!success)
      {
        Serial.print("Authentication failed for sector "); Serial.println(numOfSector);
        return;
      }

      // Step 2: Write to the other blocks
      if (idx == 16)
      {
        memset(blockBuffer, 0, sizeof(blockBuffer));
        if (!(nfc.mifareclassic_WriteDataBlock((BLOCK_NUMBER_OF_SECTOR_TRAILER(idx)) - 3, blockBuffer)))
        {
          Serial.print("Unable to write to sector "); Serial.println(numOfSector);
          return;
        }
      }
      if ((idx == 0) || (idx == 16))
      {
        memset(blockBuffer, 0, sizeof(blockBuffer));
        if (!(nfc.mifareclassic_WriteDataBlock((BLOCK_NUMBER_OF_SECTOR_TRAILER(idx)) - 2, blockBuffer)))
        {
          Serial.print("Unable to write to sector "); Serial.println(numOfSector);
          return;
        }
      }
      else
      {
        memset(blockBuffer, 0, sizeof(blockBuffer));
        if (!(nfc.mifareclassic_WriteDataBlock((BLOCK_NUMBER_OF_SECTOR_TRAILER(idx)) - 3, blockBuffer)))
        {
          Serial.print("Unable to write to sector "); Serial.println(numOfSector);
          return;
        }
        if (!(nfc.mifareclassic_WriteDataBlock((BLOCK_NUMBER_OF_SECTOR_TRAILER(idx)) - 2, blockBuffer)))
        {
          Serial.print("Unable to write to sector "); Serial.println(numOfSector);
          return;
        }
      }
      memset(blockBuffer, 0, sizeof(blockBuffer));
      if (!(nfc.mifareclassic_WriteDataBlock((BLOCK_NUMBER_OF_SECTOR_TRAILER(idx)) - 1, blockBuffer)))
      {
        Serial.print("Unable to write to sector "); Serial.println(numOfSector);
        return;
      }

      // Step 3: Reset both keys to 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      memcpy(blockBuffer, KEY_DEFAULT_KEYAB, sizeof(KEY_DEFAULT_KEYAB));
      memcpy(blockBuffer + 6, blankAccessBits, sizeof(blankAccessBits));
      blockBuffer[9] = 0x69;
      memcpy(blockBuffer + 10, KEY_DEFAULT_KEYAB, sizeof(KEY_DEFAULT_KEYAB));

      // Step 4: Write the trailer block
      if (!(nfc.mifareclassic_WriteDataBlock((BLOCK_NUMBER_OF_SECTOR_TRAILER(idx)), blockBuffer)))
      {
        Serial.print("Unable to write trailer block of sector "); Serial.println(numOfSector);
        return;
      }
    }
  }

  // Wait before trying again
  Serial.flush();
  //while(Serial.available()) 
  Serial.read();
}

// Write new totalSteps to game card, for scanning via phone app
void WriteCard (void){
  uint8_t success;                          // Flag to check if there was an error with the PN532
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, timeout);

  if (success)
  {
    nfc.PrintHex(uid, uidLength);

    if (uidLength != 4)
    {
      return;
    }

    Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

    // Try to format the card for NDEF data
    success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, 0, 0, keya);
    if (!success)
    {
      Serial.println("Unable to authenticate block 0 to enable card formatting!");
      return;
    }
    success = nfc.mifareclassic_FormatNDEF();
    if (!success)
    {
      Serial.println("Unable to format the card for NDEF");
      return;
    }

    Serial.println("Card has been formatted for NDEF data using MAD1");

    // Try to authenticate block 4 (first block of sector 1) using our key
    success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, 4, 0, keya);

    // Make sure the authentification process didn't fail
    if (!success)
    {
      Serial.println("Authentication failed.");
      return;
    }

    // Try to write a URL
    Serial.println("Writing URI to sector 1 as an NDEF Message");

    // Authenticated seems to have worked
    // Try to write an NDEF record to sector 1
    // Use 0x01 for the URI Identifier Code to prepend "http://www."
    // to the url (and save some space).  For information on URI ID Codes
    // see http://www.ladyada.net/wiki/private/articlestaging/nfc/ndef
    if (strlen(url) > 38)
    {
      // The length is also checked in the WriteNDEFURI function, but lets
      // warn users here just in case they change the value and it's bigger
      // than it should be
      Serial.println("URI is too long ... must be less than 38 characters long");
      return;
    }

    // URI is within size limits ... write it to the card and report success/failure
    success = nfc.mifareclassic_WriteNDEFURI(1, ndefprefix, url);
    if (success)
    {
      Serial.println("NDEF URI Record written to sector 1");
      PlaySong();
    }
    else
    {
      Serial.println("NDEF Record creation failed! :(");
    }
  }
  Serial.flush();
  //while(Serial.available()) 
  Serial.read();

}

void PlaySong (void){
    digitalWrite(6, HIGH); // Turn on buzzer
    for (int thisNote = 0; thisNote < 5; thisNote++) {

    int noteDuration = 1000 / noteDurations[thisNote];
    tone(6, melody[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(6);
  }
    digitalWrite(6, LOW); // Turn off buzzer

}
