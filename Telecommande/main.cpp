/*
Test Lora - Recepteur Télécommande ARC
06/10/2020
Heart beat: 39 mA
Sleep: 22,4 mA
Send Lora: 50 mA
Capacité pilr R6 2500 mA
Ecran OLED, serait en moyenne a 450 uA en fonctionnement
Antenne Lora en SPI (MISO/MOSI)

Voltage
3,27 bug
3,3 bug3,51 ok
3,55 bug

AFAIRE:
- Gestion de la batterie...
- Lorsque la télécommande se signale à l'écran: attendre un retour
- Est-ce nécessaire de réduire la vitesse du bus SPI ?
- Attendre un retour lors de l'envoie d'un message
- enableCRC
*/

/* PROTOCOLE
ENVOI DU BOUTON:
- Premier octet: numéro de l'émetteur (22 pour la télécommande ou 33 pour le heart beat de la télécommande)
- Deuxième octet: numéro du message cyclique de 0 à 7
- Troisième octet: numéro du bouton
ACK:
- premier octet: numéro de l'émetteur (22 ou 33)
- second octet: numéro du message reçu
HEART BEAT: Envoi périodique d'une pulsation à l'écran. En cas de problème l'écran affiche et la télécommande clignote
- premier octet: numéro de l'émetteur 33 pour heart beat
- second octet: numéro cyclique de 0 à 7

- Clignotement rapide LED power: low batt
*/

#define DEBUG // si défini = on affiche sur Serial, sinon riens

#include <Arduino.h>

#include <SPI.h>
#include <LoRa.h>
#include <avr/sleep.h>
#include <avr/power.h>
//#include <MsTimer2.h>
#include <avr/wdt.h>

#define SLEEP_PIN 3
#define FALSE  0
#define TRUE !FALSE

// Boutons ligne haut MOINS - SELECT - PLUS
// Boutons ligne bas START - STOP - PAUSE
#define PIN_BOUTON_PLUS A7 // Analogique
#define PIN_BOUTON_MOINS  8
#define PIN_BOUTON_SELECT 7

#define PIN_BOUTON_STOP 5
#define PIN_BOUTON_PAUSE  6     // Analogique
#define PIN_BOUTON_START  4


#define LED_LORA  A5 // Analogique
#define LED_POWER A2 // Analogique
#define PIN_VOLTAGE A0

#define VOLTAGE_LIMIT 3.3

// Attention, définition des boutons commun avec les recepteurs !
#define LORA_BOUTON_PLUS    1
#define LORA_BOUTON_MOINS   2
#define LORA_BOUTON_SELECT  3
#define LORA_BOUTON_INIT    4
#define LORA_BOUTON_STOP    5
#define LORA_BOUTON_PAUSE   6
#define LORA_BOUTON_START   7
#define LORA_HEART_BEAT     33  // Dites "33"...

#define ID_SENDER 22  // Pourquoi pas ?
#define ID_HEART_BEAT 33  // Pourquoi pas ?
#define NB_HEART_BEAT  4  // Nombre d'attente de 8 secondes de watchdog avant d'envoyer une pulsation

byte Id_Message = 0; // Sera incrémenté de 0 à 7
byte Id_Heart_Beat = 0;

byte Nb_Heart_Beat = NB_HEART_BEAT; 

byte Ack_Recu = FALSE;
byte Low_Batt = FALSE;

#define ATTENTE_ACK 3000  // Attente max pour un ACK en ms

// LORA pins
const int csPin = 10;          // 10 normalement 7 LoRa radio chip select
const int resetPin = 9;       // 5 d'après mon shéma... 6 LoRa radio reset
const int irqPin = 2;         // 1 DIO0 change for your board; must be a hardware interrupt pin

byte Bouton = 0;
unsigned int Rebond = 0;

void SendLoraMsg( byte bouton );
void InterruptionSendHeartBeat( void );

/********************************
 * watchdog interrupt
 * 
 * Appelé par le watchdog à la fin du timer
 * 
 * ******************************/
ISR (WDT_vect) 
{
   wdt_disable();  // disable watchdog

}  // end of WDT_vect

/********************************
 * SendHeartBeat
 * 
 * Envoi une pulsation à l'écran
 * 
 * ******************************/
void SendHeartBeat( void )
{
  SendLoraMsg( LORA_HEART_BEAT );
#ifdef DEBUG 
  Serial.println( "---- Hear Beat ----");
#endif
  analogWrite( LED_POWER, 255 );
  delay( 100 );
  analogWrite( LED_POWER, 0 );

} /* End of SendHeartBeat */

/******************************
 * WakeUp
 * 
 * Appellee en cas de reveil. Positionne Bouton à la valeur du bouton appuyé
 * 
 * *****************************/
void WakeUp( void )
{
  if( millis() + 100 > Rebond )
  {
    //Serial.print( "Reveil ");
    Bouton = 0;
    if( analogRead(PIN_BOUTON_PLUS) > 800  )
      Bouton = LORA_BOUTON_PLUS;
    else
    {
      if( digitalRead(PIN_BOUTON_MOINS) == 1 )
        Bouton = LORA_BOUTON_MOINS;
      else
      {
        if( digitalRead(PIN_BOUTON_SELECT) == 1 )
          Bouton = LORA_BOUTON_SELECT;
        else
        {
          if( digitalRead(PIN_BOUTON_STOP) == 1 )
            Bouton = LORA_BOUTON_STOP;
          else
          {
            if( digitalRead( PIN_BOUTON_START) == 1 )
              Bouton = LORA_BOUTON_START;
            else
            {
              if( digitalRead( PIN_BOUTON_PAUSE ) == 1 )
                Bouton = LORA_BOUTON_PAUSE;
            } /* End of if */           
          } /* End of if */
        } /* End of if */
      } /* End of if */
    } /* End of if */

#ifdef DEBUG
    if( Bouton != 0 )
    {
      Serial.print( "****** Bouton =" );
      Serial.println( Bouton );
    } /* End of if */
#endif
    Rebond = millis();
  } /* End of if */

} /* End of WakeUp */

/**************************************
 * RecoitAck
 * 
 * Call back en cas de reception de la part de l'écran: ACK ou HEART BEAT
 * 
 * Positionne une variable globale si ACK et/ou si HEART BEAT
 * 
 * ***************************************/
void RecoitAck(int packetSize)
{
  if (packetSize == 2)
  {
    byte id_sender_back;
    byte id_message_back;

    id_sender_back = LoRa.read();
    id_message_back = LoRa.read();

    if( id_sender_back == ID_SENDER && id_message_back == Id_Message )
    {
      Ack_Recu = TRUE;
      Id_Message = (Id_Message +1 ) % 8;
#ifdef DEBUG
      Serial.println( "ACK correct recu ............");
#endif
      analogWrite( LED_LORA, 1024 );
      delay( 50 );
      analogWrite( LED_LORA, 0 );
    }
#ifdef DEBUG
    else
    {
      Serial.println( "ERREUR - ACK correct mais pour un mauvais message ");
    }
#endif  
  }
#ifdef DEBUG
  else
  {
    Serial.println( "ERREUR - ACK de taille incorrecte ");
  }
#endif  
} /* End of RecoitAck */

/****************************
 * 
 * SendLoraMsg
 * 
 * Envoie le message en LORA et attend le ACK
 * 
 * *************************/
void SendLoraMsg( byte bouton )
{
  LoRa.beginPacket();
  LoRa.write( ID_SENDER );
  LoRa.write( Id_Message );
  LoRa.write( bouton );
  LoRa.endPacket();
#ifdef DEBUG
  Serial.print( "LoRa sent: ");
  Serial.println( bouton );
#endif
  analogWrite( LED_LORA, 1024 );
  delay( 50 );
  analogWrite( LED_LORA, 0 );

} /* End of SendLoraMsg */

/************************
 * InitTelecommandeVersEcran
 * 
 * Signale la télécommande à l'écran
 * 
 * **************************/
void InitTelecommandeVersEcran( void )
{
  SendLoraMsg( LORA_BOUTON_INIT );
  analogWrite( LED_POWER, 1024 );
  delay( 100 );
  analogWrite( LED_POWER, 0 );
  delay( 100 );
  analogWrite( LED_POWER, 1024 );
  delay( 100 );
  analogWrite( LED_POWER, 0 );

} /* End of InitTelecommandeVersEcran */

/*******************************
 * StartLoRa
 * 
 * Démarre ou reveille la puce lora pour préparer la transmission
 * 
 * *****************************/
void StartLoRa( void )
{
#ifdef DEBUG
  Serial.println("*********** Debut LORA Begin");
#endif
  while( !LoRa.begin(868E6) )
  {
#ifdef DEBUG
    Serial.println("Starting LoRa...");
#endif
    delay( 500 );
  }
#ifdef DEBUG
  Serial.println(" LORA Begin success");
#endif

} /* End of StartLoRa */

/******************************
 * SetWatchDog
 * 
 * Arme un timer de 8 secondes maximum pour le reveiller
 * 
 * *****************************/
void SetWatchDog( void )
{
  // WatchDog toutes les 8 secondes
    // clear various "reset" flags
  MCUSR = 0;     
  // allow changes, disable reset
  WDTCSR = bit (WDCE) | bit (WDE);
  // set interrupt mode and an interval 
  WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);    // set WDIE, and 8 seconds delay
  wdt_reset();  // pat the dog

} /* End of SetWatchDog */

/***************************
 * setup
 * 
 * *************************/
void setup( void ) {

#ifdef DEBUG
  Serial.begin(115200);
  Serial.println(" Démarrage.");
#endif
  pinMode( PIN_BOUTON_PLUS, INPUT );
  pinMode( PIN_BOUTON_MOINS, INPUT );
  pinMode( PIN_BOUTON_SELECT, INPUT );
  pinMode( PIN_BOUTON_STOP, INPUT );
  pinMode( PIN_BOUTON_START, INPUT );
  pinMode( PIN_BOUTON_PAUSE, INPUT );
  pinMode( PIN_VOLTAGE, INPUT );

  pinMode( LED_LORA, OUTPUT );
  pinMode( LED_POWER, OUTPUT );

  for( byte i = 0 ; i < 5 ; i ++ )
  {
    analogWrite( LED_LORA, 1024 );
    delay( 150 );
    analogWrite( LED_LORA, 0 );
    delay( 150 );
  }

  // Sleep
  // Serial.end();
  pinMode( SLEEP_PIN, INPUT);
  sleep_enable();
//  power_adc_disable();
//  power_spi_disable(); Utile pour LORA
// power_timer0_disable();
// power_timer1_disable();
 power_timer2_disable();
  attachInterrupt(1, WakeUp, HIGH); // Reveil sur la pate 4 (Interuption 1)
  set_sleep_mode (SLEEP_MODE_STANDBY);
//  set_sleep_mode (SLEEP_MODE_PWR_DOWN);

#ifdef DEBUG
  Serial.println("LoRa Sender");
#endif
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin
  LoRa.setSPIFrequency(1E6); // Ajout IFOU

  StartLoRa();

  LoRa.onReceive( RecoitAck );
  LoRa.receive();

  SetWatchDog();

  InitTelecommandeVersEcran();

} /* End of setup */

/**********************
 * BlynkBatt
 * 
 * Fait clignoter la LED power
 * 
 * *********************/
void BlynkBatt( void )
{
  for( byte i = 0 ; i < 5 ; i ++)
  {
    digitalWrite( LED_POWER, HIGH );
    delay(50);
    digitalWrite( LED_POWER, LOW );
    delay(50);
  } /* End of for */

} /* End of BlynkBatt */

/**************************
 * 
 * loop
 * 
 * *************************/
void loop() {

#ifdef DEBUG
  Serial.println( "Sleep activé");
  Serial.flush();
#endif
  LoRa.sleep();
  delay( 100 );
  sleep_cpu ();
#ifdef DEBUG
  Serial.println( "WakeUp");
#endif  
  // Au reveil, on s'annonce toujours vivant
  if( --Nb_Heart_Beat == 0 )
  {
    SendHeartBeat();
    Nb_Heart_Beat = NB_HEART_BEAT;
  } /* End of if */

  if( Bouton != 0)
    SendLoraMsg( Bouton );

#ifdef DEBUG
  Serial.print( "Voltage = ");
  Serial.println( analogRead( PIN_VOLTAGE ));
  float batt_level = analogRead( PIN_VOLTAGE )/1024.0*3.7;
  Serial.println( batt_level );
#endif
  if( batt_level < VOLTAGE_LIMIT )
    Low_Batt = TRUE;

  if( Low_Batt )
    BlynkBatt();

  LoRa.receive();
  //delay(1500);

  // On réarme le watchDog avant de se réendormir
  SetWatchDog();

} /* End of loop */
