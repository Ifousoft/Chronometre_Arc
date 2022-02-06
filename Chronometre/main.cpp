// Chrono Arc V4 - LoRa
// 5 septembre Initialisation 2019
// 4 octobre réduction des chiffres pour passer sur 3 écrans avec 240 au max
// 29 novembre:
//    - affichage logo lora (bluetooth) en attente de télécommande
//    - On peut plus faire start qd on est en cours de chrono
//    - affichage de perte et de retour de télécommande
// 5 janvier 2021 - ajout de la possibilité d'utiliser les écrans extérieurs. Ca marche !
// 6 janvier 2021 - Ajout d'un flush après réception pour éviter les parasites
// 7 janvier 2021 - Ajout de la gestion de la luminosité pour écran extérieur (même pour int même si ca fait rien)
// 23 janvier 2021 - Réduction du nombre de raffraichissement pour accelerer affichage
// 6 décembre 2021 - Suppression du bruit parasite généré par l'initialisation de l'écran LED. Pas compris...
// 7 décembre 2021 - Impossible de lancer un Wav, il faudrait que les HP soient branchés sur le DAC pin 25 ou 26...
// 8 décembre 2021 - Affichage logo IfouSoft et Logo companie
// 10 décembre 2021 - Affichage du temps de tir avant la volée au lieu de la durée de pre-tir
//                  - Ajout d'une phase de PRE_VOLEE 

// A FAIRE
// Test de tout les éléments d'affichages au démarrage
// Le HP grésille légèrement (Qualité montage ?)
//    Mettre à bas le signal au démarrage ?
// Virer Blynk et remplacer ses timers
// Changer la couleur du pretir pour identifier la différence TIR/Pretir
//    Jean-Philippe ?
// Afficher logo de l'abbaye au démarrage ?

/*
A Faire:
P2 - Quand la télécommande est alluméé avant l'écran, un appuie sur un bouton devrait la faire reconnaitre par l'écran sans attendre le HeartBeat
- Reglable de la luminosité
P2 - descendre le defaut telecommande d'un cran
- Ajouter affichage indication pb batteries sur écran
- Afficher la durée du tir et non du prétir au démarrage
*/

// Choisir entre compiler pour écran intérieurs ou écran extérieur. Exclusifs
//#define ECRAN_INT   // Pour les écrans intérieurs utilisant la library EPS32 64x32 LED MATRIX HUB75 DMA Display
#define ECRAN_EXT   // Pour les ecran extérieurs ayant besoin de la library SmartMatrix pour remapper les lignes

#define VERSION "V4.0"


#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#ifdef ECRAN_INT
#include <ESP32-RGB64x32MatrixPanel-I2S-DMA.h>
#else
#include <MatrixHardware_ESP32_V0.h>                // This file contains multiple ESP32 hardware configurations, edit the file to define GPIOPINOUT (or add #define GPIOPINOUT with a hardcoded number before this #include)
#include <SmartMatrix.h>
#endif
#include <BlynkSimpleEsp32_BT.h>
#include <Tone32.h>
#include "SPIFFS.h"

//#include "Abbaye.c"
#include "A1.c"
#include "A3.c"
#include "A2.c"
#include "Logo.ifou.c"

#define BUZZER_PIN 4 // 14 initialement mais changer et mettre ?
#define BUZZER_CHANNEL 0


#define DUREE_SECONDE 1000  // 1 seconde = 1000 ms
#define DUREE_AFFICHAGE_LOGO_IFOU 5000
#define DUREE_AFFICHAGE_LOGO_CIE 10000



#define BLYNK_USE_DIRECT_CONNECT

char auth[] = "y6KFaS2UmRR15rvGpoEeqGNoRn9UKU3E"; // Code de l'application Blynk

#ifdef ECRAN_INT
RGB64x32MatrixPanel_I2S_DMA dma_display;

#define COULEUR_ROUGE dma_display.color444(15, 0, 0)
#define COULEUR_VERT  dma_display.color444(0, 15, 0)
#define COULEUR_BLEU  dma_display.color444(0, 0, 15)
#define COULEUR_NOIR  dma_display.color444(0, 0, 0)
#define COULEUR_BLANC dma_display.color444(15, 15, 15)
#define COULEUR_ORANGE  dma_display.color444(15, 9, 0)
#define COULEUR_IFOU  dma_display.color444(2, 2, 7)
#define COULEUR_JAUNE  dma_display.color444(15, 15, 0)
#else
#define COLOR_DEPTH 24                  // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24

const uint8_t kMatrixWidth = 192;        // 32 known working: 16, 32, 48, 64
const uint8_t kMatrixHeight = 32;       // 16 known working: 32, 64, 96, 128
const uint8_t kPanelType = SM_PANELTYPE_HUB75_32ROW_64COL_MOD8SCAN_IFOU;   // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
const uint8_t kRefreshDepth = 36;       // known working: 24, 36, 48
const uint8_t kDmaBufferRows = 4;       // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);      // see http://docs.pixelmatix.com/SmartMatrix for options
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
const uint8_t kIndexedLayerOptions = (SM_INDEXED_OPTIONS_NONE);

const rgb24 COULEUR_NOIR = { 0, 0, 0 };
const rgb24 COULEUR_BLANC = { 255, 255, 255 };
const rgb24 COULEUR_ROUGE = { 255, 0, 0 };
const rgb24 COULEUR_VERT = { 0, 255, 0 };
const rgb24 COULEUR_BLEU = { 0, 0, 255 };
const rgb24 COULEUR_ORANGE = { 255, 165, 0 };
const rgb24 COULEUR_JAUNE = { 255, 255, 0 };
const rgb24 COULEUR_IFOU = { 34, 34, 127 };

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
SMARTMATRIX_ALLOCATE_INDEXED_LAYER(indexedLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kIndexedLayerOptions);
#endif

#define X_CHIFFRE_CENTAINES 0
#define X_CHIFFRE_DIZAINES  64
#define X_CHIFFRE_UNITES    128
#define Y_CHIFFRE           2

#define OFF 0
#define ON  1

#define DUREE_TIR_DEFAULT     120
#define DUREE_PRETIR_DEFAULT  20
#define DUREE_ORANGE          30

byte Duree_Pre_Tir = DUREE_PRETIR_DEFAULT;
byte Duree_Tir = DUREE_TIR_DEFAULT;

// Ecran LCD 64x32
// 3 écrans verticaux
// Largeur: 16 pour Feux, 16 pour centaine du compteur, 32 pour dizaine du compteur, 32 pour unités du compteur
// Hauteur 64
#define FEUX_X        43
#define FEU_ROUGE_Y   2
#define FEU_ORANGE_Y  2
#define FEU_VERT_Y    2
#define FEU_BLEU_Y    2 // Feu de configuration
#define FEUX_HAUTEUR  20
#define FEUX_LARGEUR  28

#define EQUIPE_AB 0
#define EQUIPE_CD !EQUIPE_AB

#define FALSE 0
#define TRUE  !FALSE

// Phases de tir
#define P_STOP    0
#define P_PRE_TIR 1
#define P_TIR     2
#define P_PAUSE_TIR 3
#define P_PAUSE_PRE_TIR 4
#define P_PRE_VOLEE 5

#define INTERVAL_SIGNAL_SONORE 1000 // 1 seconde

SimpleTimer Timer_Chrono;
byte Decompte = 0;
byte Phase = P_STOP; // Différentes phases de tir, prétir, stop, pause...
int Timer_Chrono_No;
byte Feu_Rouge = OFF;
byte Feu_Orange = OFF;
byte Feu_Vert = OFF;
byte Feu_Configuration = OFF;
byte Equipe_en_Cours = EQUIPE_AB;
byte Feux_A_Rafraichir = FALSE;
byte Chrono_A_Rafraichir = FALSE;
byte Son_A_Delivrer = 0;
int Freq_Son = 270;
int Duree_Son = 750;
extern byte Telecommande_Connectee;
byte Luminosite = 0; // de 0 à 3

enum { SELECT_VIDE=0, SELECT_PRETIR, SELECT_TIR, SELECT_EQUIPE, SELECT_LUMINOSITE };
byte ElementSelected = SELECT_VIDE;

#ifdef ECRAN_INT
  void DessineChiffre( byte chiffre, int x, int y, int color );
  void DessineNombre( int nombre, int color );
  void AfficheChrono( int time, int color );
  void DessinSegment( byte segment, int x, int y, int color );
  void Println( int x, int y, int couleur, char *texte );
  void DrawPixel( int x, int y, int couleur );
#else
  void DessineChiffre( byte chiffre, int x, int y, const rgb24 color );
  void DessineNombre( int nombre, const rgb24 color );
  void AfficheChrono( int time, const rgb24 color );
  void DessinSegment( byte segment, int x, int y, const rgb24 color );
  void Println( int x, int y, const rgb24 couleur, char *texte );
  void DrawPixel( int x, int y, const rgb24 couleur );
#endif

void AfficheEquipes( byte equipe );
void AfficheFeux( void );
void InitDecompte( int duree );
void AfficheLogoBlueTooth( byte etat);
void AfficheLuminosite( void );

// Recepteur LoRa
void SetupLoRa( void );
void LoopLoRa( void );
void drawBitmapIfou(int16_t x, int16_t y, const gimp_bitmap_logo_ifou* bitmap);



/*****************************
 * 
 * Println
 * 
 * Affiche un texte selon l'écran
 * 
 * ***************************/
#ifdef ECRAN_INT
void Println( int x, int y, int couleur, char *texte )
#else
void Println( int x, int y, const rgb24 couleur, char *texte )
#endif
{
#ifdef ECRAN_INT
  dma_display.println( texte );
#else
  backgroundLayer.drawString( x, y, couleur, texte );
  backgroundLayer.swapBuffers();
#endif

} /* End of Println */

/*****************************
 * 
 * FillScreen
 * 
 * rempli selon l'écran
 * 
 * ***************************/
#ifdef ECRAN_INT
void FillScreen( int couleur )
#else
void FillScreen( const rgb24 couleur )
#endif
{
#ifdef ECRAN_INT
  dma_display.fillScreen( couleur );
#else
  backgroundLayer.fillScreen( couleur );
  backgroundLayer.swapBuffers();
#endif

} /* End of FillScreen */

/*****************************
 * 
 * DrawPixel
 * 
 * rempli selon l'écran
 * 
 * ***************************/
#ifdef ECRAN_INT
void DrawPixel( int x, int y, int couleur )
#else
void DrawPixel( int x, int y, const rgb24 couleur )
#endif
{
#ifdef ECRAN_INT
  dma_display.drawPixel( x, y, couleur );
#else
  backgroundLayer.drawPixel( x, y, couleur );

#endif

} /* End of DrawPixel */

/*******************************
 * 
 * FillRect
 * 
 * Dessine un rectangle rempli selon le type d'écran int/ext
 * 
 * ****************************/
#ifdef ECRAN_INT
void FillRect( int x0, int y0, int larg, int haut, uint16_t couleur )
#else
void FillRect( int x0, int y0, int larg, int haut, const rgb24 couleur )
#endif
{
#ifdef ECRAN_INT
  dma_display.fillRect(x0, y0, larg, haut, couleur ); // x, y, largeure, hauteur
#else
  backgroundLayer.fillRectangle(x0,y0,x0+larg-1,y0+haut-1,couleur); // x1, y1, x2, y2

#endif

} /* End of FillRect */

/*******************************
 * 
 * DrawLine
 * 
 * Dessine une ligne selon le type d'écran int/ext
 * 
 * ****************************/
#ifdef ECRAN_INT
void DrawLine( int x0, int y0, int x1, int y1, uint16_t couleur )
#else
void DrawLine( int x0, int y0, int x1, int y1, const rgb24 couleur )
#endif
{
#ifdef ECRAN_INT
  dma_display.drawLine(x0, y0, x1, y1, couleur );
#else
  backgroundLayer.drawLine(x0,y0,x1,y1,couleur);

#endif

} /* End of DrawLine */

/***************************
 * 
 * EffacePerteTelecommande
 * 
 * Affiche l'indicateur de perte de télécommande
 * 
 * *****************************/
void EffacePerteTelecommande( void )
{
//  dma_display.fillRect(107, 20, 19, 12, COULEUR_NOIR );
    FillRect( 107, 20, 19, 12, COULEUR_NOIR );
#ifdef ECRAN_EXT
    backgroundLayer.swapBuffers();
#endif

} /* End of EffacePerteTelecommande */

/************************
 * 
 * AffichagePerteTelecommande
 * 
 * Affiche le logo de la télécommande perdue
 * 
 * ******************************/
void AffichagePerteTelecommande( void )
{
  DrawLine( 109, 26, 123, 26, COULEUR_ROUGE);
  DrawLine( 110, 25, 113, 22, COULEUR_ROUGE);
  DrawLine( 114, 23, 120, 29, COULEUR_ROUGE);
  DrawLine( 112, 29, 119, 22, COULEUR_ROUGE);
  DrawLine( 120, 23, 122, 25, COULEUR_ROUGE);

  DrawLine( 107, 31, 107, 20, COULEUR_BLANC);
  DrawLine( 108, 20, 125, 20, COULEUR_BLANC);
  DrawLine( 125, 21, 125, 31, COULEUR_BLANC);
  DrawLine( 124, 31, 108, 31, COULEUR_BLANC);

#ifdef ECRAN_EXT
    backgroundLayer.swapBuffers();
#endif

} /* End of AffichagePerteTelecommande */

/**************************
 * EncadrementChrono
 * 
 * Affiche un encadrement autour du chrono Tir/Pretir
 * 
 * *******************************/
#ifdef ECRAN_INT
void EncadrementChrono( int couleur )
#else
void EncadrementChrono( const rgb24 couleur )
#endif
{
    DrawLine( 0,30, 42, 30, couleur);
    DrawLine( 42,0, 42, 29, couleur);
    DrawLine( 106,0, 106, 31, couleur);
    DrawLine( 170,0, 170, 31, couleur);
    DrawLine( 128,0, 169, 0, couleur);
#ifdef ECRAN_EXT
    backgroundLayer.swapBuffers();
#endif

} /* End of EncadrementChrono */

/**************************
 * EncadrementEquipe
 * 
 * Affiche un encadrement autour de l'équipe
 * 
 * *******************************/
#ifdef ECRAN_INT
void EncadrementEquipe( int couleur )
#else
void EncadrementEquipe( const rgb24 couleur )
#endif
{
    DrawLine( 106, 0, 106, 18, couleur);
    DrawLine( 107, 18, 127, 18, couleur);
    DrawLine( 127, 0, 127, 17, couleur);

    DrawLine( 170, 31, 170, 13, couleur);
    DrawLine( 171, 13, 190, 13, couleur);
    DrawLine( 191, 13, 191, 31, couleur);
#ifdef ECRAN_EXT
    backgroundLayer.swapBuffers();
#endif

} /* End of EncadrementEquipe */

/**************************
 * EncadrementLuminosite
 * 
 * Affiche un encadrement autour de l'équipe
 * 
 * *******************************/
#ifdef ECRAN_INT
void EncadrementLuminosite( int couleur )
#else
void EncadrementLuminosite( const rgb24 couleur )
#endif
{
    DrawLine( 171, 1, 191, 1, couleur);
    DrawLine( 191, 2, 191, 10, couleur);
    DrawLine( 191, 11, 171, 11, couleur);
    DrawLine( 171, 2, 171, 10, couleur);
#ifdef ECRAN_EXT
    backgroundLayer.swapBuffers();
#endif

} /* End of EncadrementLuminosite */

/*****************************
 * 
 * AfficheSelection
 * 
 * Affiche la selection courante
 * 
*********************************/
void AfficheSelection( byte element )
{
  switch( element )
  {
    case SELECT_PRETIR:
      // Afiche le chrono du pré-tir et le selectionne
        AfficheChrono( Duree_Pre_Tir, COULEUR_BLEU );
        EncadrementChrono( COULEUR_JAUNE );
    break;
    case SELECT_TIR:
      // Affiche le chrono du tir et le selectionne
        AfficheChrono( Duree_Tir, COULEUR_BLANC );
        EncadrementChrono( COULEUR_JAUNE );
    break;
    case SELECT_EQUIPE:
      // Selectionne les équipes
      EncadrementEquipe( COULEUR_JAUNE );
    break;
    case SELECT_LUMINOSITE:
      // Selectionne la luminosité de l'écran
      EncadrementLuminosite( COULEUR_JAUNE );
      AfficheLuminosite();
    break;
  } /* End of switch */

} /* End of AfficheSelection */

/***********************************
 * 
 * EffaceLuminosite
 * 
 * Efface la zone de reglage de la luminosite
 * 
 * *****************************/
void EffaceLuminosite( void )
{
  FillRect( 171,1, 21, 11, COULEUR_NOIR );
#ifdef ECRAN_EXT
    backgroundLayer.swapBuffers();
#endif

} /* End of EffaceLuminosite */

/*****************************
 * 
 * EffaceSelection
 * 
 * Efface la selection courante
 * 
*********************************/
void EffaceSelection( byte element )
{
  switch( element )
  {
    case SELECT_PRETIR:
      // deselectionne le pretir et réaffiche le tir
      EncadrementChrono( COULEUR_NOIR );
    break;
    case SELECT_TIR:
      // deselectionne le tir
      EncadrementChrono( COULEUR_NOIR );
    break;
    case SELECT_EQUIPE:
      // deselectionne les équipes
      EncadrementEquipe( COULEUR_NOIR );
    break;
    case SELECT_LUMINOSITE:
      // deselectionne les équipes
      EffaceLuminosite( );
    break;
  
  } /* End of switch */

} /* End of EffaceSelection */

/******************************
 * 
 * ChangeEtAfficheSelection
 * 
 * Change la selection des éléments sur écran.
 * Affiche la selection et efface la précédente
 * Ne s'execute pas si le tir est en cours
 * 
 * ***************************/
void ChangeEtAfficheSelection( void )
{
  if( Phase == P_PRE_VOLEE )
  {
    Feu_Rouge = FALSE;
    Feu_Configuration = TRUE; // Fleu bleu
    AfficheFeux();

    EffaceSelection( ElementSelected );
    ElementSelected = (ElementSelected + 1 ) % (SELECT_LUMINOSITE+1);
    if( ElementSelected != SELECT_VIDE )
      AfficheSelection( ElementSelected );
    else
    {
      Feu_Rouge = TRUE;
      Feu_Configuration = FALSE;
      AfficheFeux();
    } /* End of if */
    
  } /* End of if */

} /* End of ChangeEtAfficheSelection */

/*******************************
 * 
 * BoutonPlusMoins
 * 
 * Ajouter du temps au chrono selectionné ou change d'équipe
 * Ne doit pas être appelée en phase de tir
 * 
 * sens: TRUE = plus, FALSE = moins
 * 
 * *********************************/
void BoutonPlusMoins( byte sens )
{
  switch( ElementSelected )
  {
    case SELECT_PRETIR:
      if( sens )
        Duree_Pre_Tir += 5;
      else
      {
        if( Duree_Pre_Tir >= 5 )
          Duree_Pre_Tir -= 5;
      } /* End of if */
      AfficheChrono( Duree_Pre_Tir, COULEUR_BLEU );
    break;
    case SELECT_TIR:
      if( sens )
        Duree_Tir += 5;
      else
      {
        if( Duree_Tir >= 5 )
          Duree_Tir -= 5;
      } /* End of if */
      AfficheChrono( Duree_Tir, COULEUR_BLEU );
    break;
    case SELECT_EQUIPE:
      // deselectionne les équipes
      Equipe_en_Cours = !Equipe_en_Cours;
      AfficheEquipes( Equipe_en_Cours );
    break;
    case SELECT_LUMINOSITE:
      // Change la luminisité
      if( sens )
        Luminosite++;
      else
        if( Luminosite > 0 ) 
          Luminosite--;
      Luminosite = Luminosite % 4; // 4 niveaux
      AfficheLuminosite( );
    break;
  
  } /* End of switch */

} /* End of BoutonPlusMoins */

/***********************
 * UnSignalSonoreCourt
 * 
 * Emet un signal sonore
 * 
 * ************************/
void UnSignalSonoreCourt( void )
{
  tone(BUZZER_PIN, Freq_Son, Duree_Son, BUZZER_CHANNEL);
  noTone(BUZZER_PIN, BUZZER_CHANNEL);

} /* End of UnSignalSonoreCourt */

/***********************
 * UnSignalSonoreLong
 * 
 * Emet un signal sonore
 * 
 * ************************/
void UnSignalSonoreLong( void )
{
  tone(BUZZER_PIN, Freq_Son, Duree_Son*2, BUZZER_CHANNEL);

} /* End of UnSignalSonoreLong */

/****************************
 * SignauxSonores
 * 
 * Emet un certain nombre de signaux sonores
 * 
 * *********************/
void SignalSonore( byte nombre )
{
//  Timer_Chrono.disable( Timer_Chrono_No );
  for( byte i = 0 ; i < nombre ; i ++)
  {
    // Emet un signal sonore
    UnSignalSonoreCourt();
    Serial.println( "Bip");
    delay( INTERVAL_SIGNAL_SONORE );

  } /* End of for */

  Timer_Chrono.restartTimer(Timer_Chrono_No);

}  /* End of SignalSonore */

/****************************
 * Bouton Pause
 * 
 * Met le chrono en pause
 * 
 * ********************/
void BoutonPause( void )
{
  Serial.print( "Pause");
  
  switch( Phase )
  {
    case P_PRE_TIR:
      Phase = P_PAUSE_PRE_TIR;
      Feu_Rouge = ON;
      Feu_Orange = OFF;
      Feu_Vert = OFF;
      break;
    case P_TIR:
      Phase = P_PAUSE_TIR;
      Feu_Rouge = ON;
      Feu_Orange = OFF;
      Feu_Vert = OFF;
      break;
    case P_PAUSE_PRE_TIR:
      Phase = P_PRE_TIR;
      Feu_Rouge = ON;
      Feu_Orange = OFF;
      Feu_Vert = OFF;
      break;
    case P_PAUSE_TIR:
      Phase = P_TIR;
      Feu_Rouge = OFF;
      if( Decompte <= DUREE_ORANGE )
      {
        Feu_Orange = ON;
        Feu_Vert = OFF;
      }
      else
      {
        Feu_Orange = OFF;
        Feu_Vert = ON;
      } /* End of if */
      break;

  } /* End of Switch */

  AfficheFeux();

} /* End of BoutonPause */

/************************
 * BoutonStart
 * 
 * Met le chrono en route ave la duree de PRETIR
 * 
 * ***********************/
void BoutonStart( void )
{
  // Phase PRE_VOLEE, on lance la volée
  if( Phase == P_PRE_VOLEE )
  {
    Serial.println( "Start");
    // Déselectionne si qq chose était encore en selection
    EffaceSelection( ElementSelected);
    ElementSelected = SELECT_VIDE;
    
    Phase = P_PRE_TIR;
    InitDecompte( Duree_Pre_Tir );
    Feu_Rouge = ON;
    Feu_Orange = OFF;
    Feu_Vert = OFF;
    AfficheFeux();
    AfficheChrono( Duree_Pre_Tir, COULEUR_BLANC );
    AfficheEquipes(Equipe_en_Cours);
    SignalSonore( 2 );
  }
  else
  {
      // P_STOP, on prépare la prochaine voléee
      if( Phase == P_STOP )
    {
      Serial.println( "Start");
      // Déselectionne si qq chose était encore en selection
      EffaceSelection( ElementSelected);
      ElementSelected = SELECT_VIDE;
      
      Phase = P_PRE_VOLEE;
      InitDecompte( Duree_Tir );
      Feu_Rouge = ON;
      Feu_Orange = OFF;
      Feu_Vert = OFF;
      AfficheFeux();
      AfficheChrono( Duree_Tir, COULEUR_BLANC );
      AfficheEquipes(Equipe_en_Cours);
//      SignalSonore( 2 );

    } /* End of if */

  } /* End of if */

} /* End of BoutonStart */

/***********************
 * Bouton stop
 * 
 * Arrête le tir et Feu rouge, chrono 0
 * 
 * **************************/
void BoutonStop( void )
{
  Serial.print( "Stop");
  Phase = P_STOP;
  InitDecompte( 0 );
  Feu_Rouge = ON;
  Feu_Orange = OFF;
  Feu_Vert = OFF;
  AfficheFeux();
  AfficheChrono( 0, COULEUR_ROUGE );
  UnSignalSonoreLong();

} /* End of BoutonStop */

/*********************
 * InterruptionDecompte
 * 
 * décompte le temps de 1s et met à jour la phase
 * 
 * ***********************/
void InterruptionDecompte()
{
  Serial.println( millis() );
  // Si phase de pré-tir ou tir en cours: on décompte
  if( ( Phase == P_PRE_TIR || Phase == P_TIR ) && Decompte > 0 )
  {
    Decompte--;
    Chrono_A_Rafraichir = TRUE;
      // Si phase de tir et reste plus bcp de temps
    if( Phase == P_TIR && Decompte <= DUREE_ORANGE )
    {
      Feu_Rouge = OFF;
      Feu_Orange = ON;
      Feu_Vert = OFF;
      Feux_A_Rafraichir = TRUE;
    } /* End of if */
  }
  else
  {
    // Si fin du pré-tir on passe en phase TIR
    if( Decompte == 0 && Phase == P_PRE_TIR )
    {
      Phase = P_TIR;
      Decompte = Duree_Tir;
      Feu_Rouge = OFF;
      Feu_Orange = OFF;
      Feu_Vert = ON;
      Feux_A_Rafraichir = TRUE;
      Son_A_Delivrer = 1;
    } else {
      // Si la phase de tir est terminée: on arrête tout
      if( Decompte == 0 && Phase == P_TIR )
      {
        Phase = P_STOP;
        Feu_Rouge = ON;
        Feu_Orange = OFF;
        Feu_Vert = OFF;
        Feux_A_Rafraichir = TRUE;
        Son_A_Delivrer = 3;
        Equipe_en_Cours = !Equipe_en_Cours; // Changement d'équipe pour la prochaine volée
      } /* End of if */

    } /* End of if */

  } /* End of if */

} /* End of InterruptionDecompte */

/*********************
 * InitDecompte
 * 
 * Positionne le décompte à la valeur initiale
 * 
 * *******************/
void InitDecompte( int duree )
{
  Decompte = duree;

} /* End of InitDecompte */

/*************************
 * AfficheFeux
 * 
 * Affiche les feux sur l'écran
 * 
 * **********************/
void AfficheFeux( void )
{
  // Efface le feux
  FillRect(FEUX_X, FEU_ROUGE_Y, FEUX_HAUTEUR, FEUX_LARGEUR, COULEUR_NOIR);
  if( Feu_Rouge )
    FillRect(FEUX_X, FEU_ROUGE_Y, FEUX_HAUTEUR, FEUX_LARGEUR, COULEUR_ROUGE);
  else
  {
    if( Feu_Orange)
      FillRect(FEUX_X, FEU_ORANGE_Y, FEUX_HAUTEUR, FEUX_LARGEUR, COULEUR_ORANGE);
    else
      if( Feu_Vert )
        FillRect(FEUX_X, FEU_VERT_Y, FEUX_HAUTEUR, FEUX_LARGEUR, COULEUR_VERT);
      else
        if( Feu_Configuration )
          FillRect(FEUX_X, FEU_BLEU_Y, FEUX_HAUTEUR, FEUX_LARGEUR, COULEUR_BLEU);

  } /* End of if */
#ifdef ECRAN_EXT
    backgroundLayer.swapBuffers();
#endif

} /* End of AfficheFeux */

/*********************
 * Affiche_A
 * 
 * Affiche un A en x et y avec la couleur définie
 * 
 * ***********************/
#ifdef ECRAN_INT
void Affiche_A( int x, int y, int color )
#else
void Affiche_A( int x, int y, const rgb24 color )
#endif
{
  // A
  FillRect(x+0,y+3, 3, 10,color);
  FillRect(x+3,y+0,16,3,color);
  FillRect(x+9,y+3,3,10,color);
  FillRect(x+3,y+13, 16, 3,color);


} /* End of Affiche_A */

/*********************
 * Affiche_B
 * 
 * Affiche un B en x et y avec la couleur définie
 * 
 * ***********************/
#ifdef ECRAN_INT
void Affiche_B( int x, int y, int color )
#else
void Affiche_B( int x, int y, const rgb24 color )
#endif
{
  // B
  FillRect(x+0,y+3,3,10,color);
  FillRect(x+3,y+0,5,3,color);
  FillRect(x+11,y+0,5,3,color);
  FillRect(x+8,y+3,3,10,color);
  FillRect(x+16,y+3,3,10,color);
  FillRect(x+0,y+13,19,3,color);


} /* End of Affiche_B */

/*********************
 * Affiche_C
 * 
 * Affiche un C en x et y avec la couleur définie
 * 
 * ***********************/
#ifdef ECRAN_INT
void Affiche_C( int x, int y, int color )
#else
void Affiche_C( int x, int y, const rgb24 color )
#endif
{
  // C
  FillRect(x+0,y+3, 3, 10,color);
  FillRect(x+3,y+0,3,3,color);
  FillRect(x+13,y+0,3,3,color);
  FillRect(x+3,y+13, 13, 3,color);
  FillRect(x+16,y+3, 3, 10,color);


} /* End of Affiche_C */

/*********************
 * Affiche_D
 * 
 * Affiche un D en x et y avec la couleur définie
 * 
 * ***********************/
#ifdef ECRAN_INT
void Affiche_D( int x, int y, int color )
#else
void Affiche_D( int x, int y, const rgb24 color )
#endif
{
  // D
  FillRect(x+0,y+3,3,10,color);
  FillRect(x+3,y+0,13,3,color);
  FillRect(x+16,y+3,3,10,color);
  FillRect(x+0,y+13,19,3,color);


} /* End of Affiche_D */

/*************************
 * AfficheEquipes
 * 
 * Affiche les équipes prochaines ou en cours
 * 
 * ***********************/
void AfficheEquipes( byte equipe )
{
  FillRect(108, 1, 19, 16,COULEUR_NOIR);
  FillRect(172, 15, 19, 16,COULEUR_NOIR);

  if( equipe == EQUIPE_AB )
  {
    // A
    Affiche_A( 108, 1, COULEUR_VERT );
    Affiche_B( 172, 15, COULEUR_VERT );

  } else {

    Affiche_C( 108, 1, COULEUR_VERT );
    Affiche_D( 172, 15, COULEUR_VERT );

  } /* End of if */
#ifdef ECRAN_EXT
    backgroundLayer.swapBuffers();
#endif

} /* End of AfficheEquipes */

/*************************
 * AfficheEquipes
 * 
 * Affiche les équipes prochaines ou en cours
 * 
 * ***********************/
void AfficheLuminosite( void )
{
  // Efface le fond de l'indicateur
  FillRect(172, 2, 19, 9, COULEUR_NOIR);

  switch( Luminosite )
  {
    case 0:
      FillRect( 188, 5, 3, 3, COULEUR_BLEU ); // 0
      break;
    case 1:
      FillRect( 188, 5, 3, 3, COULEUR_BLEU ); // 0
      FillRect( 183, 4, 3, 5, COULEUR_BLEU ); // 1
      break;
    case 2:
      FillRect( 188, 5, 3, 3, COULEUR_BLEU ); // 0
      FillRect( 183, 4, 3, 5, COULEUR_BLEU ); // 1
      FillRect( 178, 3, 3, 7, COULEUR_BLEU ); // 2
      break;
    case 3:
      FillRect( 188, 5, 3, 3, COULEUR_BLEU ); // 0
      FillRect( 183, 4, 3, 5, COULEUR_BLEU ); // 1
      FillRect( 178, 3, 3, 7, COULEUR_BLEU ); // 2
      FillRect( 173, 2, 3, 9, COULEUR_BLEU ); // 3
      break;
  } /* End of switch */

#ifdef ECRAN_EXT
  switch( Luminosite )
  {
    case 0:
      matrix.setBrightness( 10 );
      break;
    case 1:
      matrix.setBrightness( 64 );
      break;
    case 2:
      matrix.setBrightness( 128 );
      break;
    case 3:
      matrix.setBrightness( 255 );
      Serial.println( "      -- Luminosité 255 ");
      break;
  } /* End of switch */
  backgroundLayer.swapBuffers();
#endif

} /* End of AfficheEquipes */

/*************************
 * AfficheChrono
 * 
 * Affiche le chrono
 * 
 * **********************/
#ifdef ECRAN_INT
void AfficheChrono( int time, int color )
#else
void AfficheChrono( int time, const rgb24 color )
#endif
{
  static int old_time = 0;

  // Si le chrono a changé on met à jour
  if( time != old_time )
  {
    DessineNombre( time, color );
    old_time = time;

  } /* End of if */

} /* End of AfficheChrono */

/*****************
 * EffaceEcranLED
 * 
 * Efface tout l'écran
 * 
 * ********************/
void EffaceEcranLED( void )
{
    FillScreen(COULEUR_NOIR); // Tout en noir

} /* End of EffaceEcranLED */

/************************
 * IntialiseEcranLED
 * 
 * Initialise l'écran à LED
 * 
 * *************************/
void IntialiseEcranLED( void )
{
#ifdef ECRAN_INT
  dma_display.begin(); // use default pins
#else
  matrix.addLayer(&backgroundLayer); 
  //matrix.addLayer(&indexedLayer); 
  matrix.begin();

  matrix.setBrightness(10);
  backgroundLayer.swapBuffers();   
#endif

  EffaceEcranLED();
  Serial.println( "Ecran Initialisé");

} /* End of InitialiseEcranLED */

/*************************
 * AfficheBlueTooth
 * 
 * Affiche le logo bluetooth
 * 
 * ******************/
void AfficheLogoBlueTooth( byte etat)
{
#ifdef ECRAN_INT
  int couleur = etat?COULEUR_VERT:COULEUR_BLEU;
#else
  const rgb24 couleur = etat?COULEUR_BLEU:COULEUR_BLEU;
#endif

  FillRect( 5,19, 28, 2, couleur);
  DrawLine( 6,18, 12, 12, couleur);
  DrawLine( 7,18, 12, 13, couleur);
  DrawLine( 8, 18, 13, 13, couleur);
  DrawLine( 11, 27, 26, 12, couleur);
  DrawLine( 12, 27, 26, 13, couleur);
  DrawLine( 13, 27, 27, 13, couleur);
  DrawLine( 14, 14, 27, 27, couleur);
  DrawLine( 13, 14, 26, 27, couleur);
  DrawLine( 13, 15, 25, 27, couleur);
  DrawLine( 27, 15, 30, 18, couleur);
  DrawLine( 27, 14, 31, 18, couleur);
  DrawLine( 28, 14, 32, 18, couleur);
#ifdef ECRAN_EXT
    backgroundLayer.swapBuffers();
#endif

} /* End of AfficheBlueTooth */

/*******************************
 * AfficheLogoIfou
 * 
 * Affiche le logo Ifou et l'année
 * 
 * ***********************/
void AfficheLogoIfou( void )
{

  backgroundLayer.fillScreen({0,0,0});
  drawBitmapIfou(64,0,&logo_ifou);
  backgroundLayer.swapBuffers();

/*
  // I
  DrawLine( 96, 28, 96, 24, COULEUR_IFOU);
  DrawLine( 97, 26, 101, 26, COULEUR_IFOU);
  DrawLine( 102,28,102,24, COULEUR_IFOU);
  // F
  DrawLine( 96,21, 96,17, COULEUR_IFOU);
  DrawLine( 97,21, 102,21, COULEUR_IFOU);
  DrawLine( 99,19,99,21, COULEUR_IFOU);
  // O
  DrawLine( 96,14,96,12, COULEUR_IFOU);
  DrawLine( 97,15,101,15, COULEUR_IFOU);
  DrawLine( 97,11,101,11, COULEUR_IFOU);
  DrawLine( 102,14,102,12, COULEUR_IFOU);
  // U
  DrawLine( 96,8,101,8, COULEUR_IFOU);
  DrawLine( 96,4,101,4, COULEUR_IFOU);
  DrawLine( 102,7, 102,5, COULEUR_IFOU);
*/

  // 2
  DrawPixel( 175,28, COULEUR_IFOU );
  DrawLine( 174,27,174,25 , COULEUR_IFOU);
  DrawLine( 175,24,176,24, COULEUR_IFOU);
  DrawLine( 177,27,177,25, COULEUR_IFOU);
  DrawLine( 178,28, 180,28, COULEUR_IFOU);
  DrawLine( 180,27,180,24, COULEUR_IFOU);
  // 0
  DrawLine( 174,21,174,19, COULEUR_IFOU);
  DrawLine( 175,22,179,22, COULEUR_IFOU);
  DrawLine( 175,18,179,18, COULEUR_IFOU);
  DrawLine( 180,21,180,19, COULEUR_IFOU);

  // 2
  DrawPixel( 175,16, COULEUR_IFOU );
  DrawLine( 174,15,174,13 , COULEUR_IFOU);
  DrawLine( 175,12,176,12, COULEUR_IFOU);
  DrawLine( 177,15,177,13, COULEUR_IFOU);
  DrawLine( 178,16, 180,16, COULEUR_IFOU);
  DrawLine( 180,15,180,12, COULEUR_IFOU);
  // 0
  DrawLine( 174,9,174,7, COULEUR_IFOU);
  DrawLine( 175,10,179,10, COULEUR_IFOU);
  DrawLine( 175,6,179,6, COULEUR_IFOU);
  DrawLine( 180,9,180,7, COULEUR_IFOU);

  Println(0, 0, COULEUR_BLANC, (char *) "Ifou");
  Println(0, 10, COULEUR_BLANC, (char *) "2020");
  Println(0, 20, COULEUR_BLANC, (char *) VERSION );

#ifdef ECRAN_EXT
    backgroundLayer.swapBuffers();
#endif

} /* End of AfficheLogoIfou */

/**************************************************
 * 
 * drawBitmap pour partie gauche du logo de l'abbaye
 * 
 * ***********************************************/

void drawBitmapA1(int16_t x, int16_t y, const gimp_bitmap_a1* bitmap) {
  for(unsigned int i=0; i < bitmap->height; i++) {
    for(unsigned int j=0; j < bitmap->width; j++) {
      rgb24 pixel = { bitmap->pixel_data[(i*bitmap->width + j)*3 + 0],
                      bitmap->pixel_data[(i*bitmap->width + j)*3 + 1],
                      bitmap->pixel_data[(i*bitmap->width + j)*3 + 2] };

      backgroundLayer.drawPixel(x + j, y + i, pixel);
    }
  }
}

/**************************************************
 * 
 * drawBitmap pour partie centrale du logo de l'abbaye
 * 
 * ***********************************************/

void drawBitmapA2(int16_t x, int16_t y, const gimp_bitmap_a2* bitmap) {
  for(unsigned int i=0; i < bitmap->height; i++) {
    for(unsigned int j=0; j < bitmap->width; j++) {
      rgb24 pixel = { bitmap->pixel_data[(i*bitmap->width + j)*3 + 0],
                      bitmap->pixel_data[(i*bitmap->width + j)*3 + 1],
                      bitmap->pixel_data[(i*bitmap->width + j)*3 + 2] };

      backgroundLayer.drawPixel(x + j, y + i, pixel);
    }
  }
}

/**************************************************
 * 
 * drawBitmap pour partie droite du logo de l'abbaye
 * 
 * ***********************************************/

void drawBitmapA3(int16_t x, int16_t y, const gimp_bitmap_a3* bitmap) {
  for(unsigned int i=0; i < bitmap->height; i++) {
    for(unsigned int j=0; j < bitmap->width; j++) {
      rgb24 pixel = { bitmap->pixel_data[(i*bitmap->width + j)*3 + 0],
                      bitmap->pixel_data[(i*bitmap->width + j)*3 + 1],
                      bitmap->pixel_data[(i*bitmap->width + j)*3 + 2] };

      backgroundLayer.drawPixel(x + j, y + i, pixel);
    }
  }
}

/**************************************************
 * 
 * drawBitmap pour Logo Ifou
 * 
 * ***********************************************/

void drawBitmapIfou(int16_t x, int16_t y, const gimp_bitmap_logo_ifou* bitmap) {
  for(unsigned int i=0; i < bitmap->height; i++) {
    for(unsigned int j=0; j < bitmap->width; j++) {
      rgb24 pixel = { bitmap->pixel_data[(i*bitmap->width + j)*3 + 0],
                      bitmap->pixel_data[(i*bitmap->width + j)*3 + 1],
                      bitmap->pixel_data[(i*bitmap->width + j)*3 + 2] };

      backgroundLayer.drawPixel(x + j, y + i, pixel);
    }
  }
}
/**************************************************
 * 
 * AfficheLogoAbbaye
 * 
 * *************************************************/
void AfficheLogoAbbaye( void )
{
  backgroundLayer.fillScreen({0,0,0});
  drawBitmapA1(0,0,&gimp_a1);
  drawBitmapA2(64,0,&gimp_a2);
  drawBitmapA3(128,23,&gimp_a3);
  backgroundLayer.swapBuffers();
  delay( DUREE_AFFICHAGE_LOGO_CIE );

} /* End of AfficheLogoAbbaye */

/**************************
 * setup
 * 
 * **************************/
void setup() {

  Serial.begin(115200);

  Serial.println("*****************************************************");
  Serial.println(" HELLO !");
  Serial.println("*****************************************************");

  IntialiseEcranLED();
  SignalSonore( 2 );
  AfficheLogoIfou();
  delay( DUREE_AFFICHAGE_LOGO_IFOU );

  Serial.println( "Draw bitmap");
  AfficheLogoAbbaye();

  SetupLoRa();
  // Tant que la Télécommande ne s'est pas connecté, on attend

  while( !Telecommande_Connectee )
  {
    Serial.println( "Wait for telecommande...");
    EffaceEcranLED();
    delay( 500 );
    AfficheLogoBlueTooth( FALSE );
    delay( 500 );
    LoopLoRa();

  } /* End of while */

  EffaceEcranLED();
  
  Timer_Chrono_No = Timer_Chrono.setInterval( DUREE_SECONDE, InterruptionDecompte ); // Interruption toutes les secondes pour le chrono
  Feu_Rouge = ON;
  Feu_Orange = OFF;
  Feu_Vert = OFF;
  Phase = P_PRE_VOLEE;
  InitDecompte( Duree_Tir ); // Modif V3.5

  AfficheFeux();
  AfficheEquipes( Equipe_en_Cours );
  AfficheChrono( Decompte, COULEUR_BLANC );

} /* End of setup */

/**************************
 * loop
 * 
 * **************************/
void loop() {

  if( Feux_A_Rafraichir )
  {
    AfficheFeux();
    Feux_A_Rafraichir = FALSE;

  } /* End of if */

  if( Chrono_A_Rafraichir )
  {
    AfficheChrono( Decompte, COULEUR_BLANC );
    Chrono_A_Rafraichir = FALSE;

  } /* End of if */

  // Si il faut délivrer un signal sonore
  if( Son_A_Delivrer > 0 )
  {
    SignalSonore( Son_A_Delivrer );
    Son_A_Delivrer = 0;
  } /* End of if */

  Timer_Chrono.run();

  LoopLoRa();

} /* End of loop */

/************************************
 * DessineNombre
 * 
 * Dessine un nombre à 3 chiffre max
 * 
 * **************************/
#ifdef ECRAN_INT
void DessineNombre( int nombre, int color )
#else
void DessineNombre( int nombre, const rgb24 color )
#endif
{
  int centaines;
  int dizaines;
  int unites;

  centaines = nombre / 100;
  dizaines = (nombre-centaines*100) / 10;
  unites = (nombre-centaines*100-dizaines*10);

  DessineChiffre( centaines, X_CHIFFRE_CENTAINES, Y_CHIFFRE ,color);
  DessineChiffre( dizaines, X_CHIFFRE_DIZAINES, Y_CHIFFRE ,color);
  DessineChiffre( unites, X_CHIFFRE_UNITES, Y_CHIFFRE ,color);

} /* End of DessineNombre */

/************************************
 * DessineSegment
 * 
 * Dessine un des segments en décallage par rapport à origine
 * 
 * ******************************/
#ifdef ECRAN_INT
void DessinSegment( byte segment, int x, int y, int color )
#else
void DessinSegment( byte segment, int x, int y, const rgb24 color )
#endif
{
  switch( segment )
  {
    case 0: // horiz haut
      FillRect(x+0, y+0, 4, 28, color);
    break;
    case 1: // Haut gauche
      FillRect(x+0, y+24, 23, 4, color);
    break;
    case 2: // Haut droite
      FillRect(x+0, y+0, 23, 4, color);
    break;
    case 3: // millieu
      FillRect(x+19, y+0, 4, 28, color);
    break;
    case 4: // Bas Gauche
      FillRect(x+19, y+24, 23, 4, color);
    break;
    case 5: // Bas droite
      FillRect(x+19, y+0, 23, 4, color);
    break;
    case 6: // Horiz bas
      FillRect(x+38, y+0, 4, 28, color);
    break;
    default:
    break;

  } /* End of swith */

} /* End of DessingSegment */

/************************************
 * DessineChiffre
 * 
 * Dessine un chiffre
 * Exception pour le chiffre des centaine: 1 ou rien
 * 
 * **************************/
#ifdef ECRAN_INT
void DessineChiffre( byte chiffre, int x, int y, int color )
#else
void DessineChiffre( byte chiffre, int x, int y, const rgb24 color )
#endif
{
  /* Segments
        0
      1   2
        3
      4   5
        6
        */
  switch( chiffre )
  {
    case 0:
      DessinSegment( 3, x, y, COULEUR_NOIR);
      DessinSegment( 0, x, y, color);
      DessinSegment( 1, x, y, color);
      DessinSegment( 2, x, y, color);
      DessinSegment( 4, x, y, color);
      DessinSegment( 5, x, y, color);
      DessinSegment( 6, x, y, color);
    break;
    case 1:
      DessinSegment( 0, x, y, COULEUR_NOIR);
      DessinSegment( 1, x, y, COULEUR_NOIR);
      DessinSegment( 3, x, y, COULEUR_NOIR);
      DessinSegment( 4, x, y, COULEUR_NOIR);
      DessinSegment( 6, x, y, COULEUR_NOIR);
      DessinSegment( 2, x, y, color);
      DessinSegment( 5, x, y, color);
    break;
    case 2:
      DessinSegment( 1, x, y, COULEUR_NOIR);
      DessinSegment( 5, x, y, COULEUR_NOIR);
      DessinSegment( 0, x, y, color);
      DessinSegment( 2, x, y, color);
      DessinSegment( 3, x, y, color);
      DessinSegment( 4, x, y, color);
      DessinSegment( 6, x, y, color);
    break;
    case 3:
      DessinSegment( 1, x, y, COULEUR_NOIR);
      DessinSegment( 4, x, y, COULEUR_NOIR);
      DessinSegment( 0, x, y, color);
      DessinSegment( 2, x, y, color);
      DessinSegment( 3, x, y, color);
      DessinSegment( 5, x, y, color);
      DessinSegment( 6, x, y, color);
    break;
    case 4:
      DessinSegment( 0, x, y, COULEUR_NOIR);
      DessinSegment( 4, x, y, COULEUR_NOIR);
      DessinSegment( 6, x, y, COULEUR_NOIR);
      DessinSegment( 1, x, y, color);
      DessinSegment( 2, x, y, color);
      DessinSegment( 3, x, y, color);
      DessinSegment( 5, x, y, color);
    break;
    case 5:
      DessinSegment( 2, x, y, COULEUR_NOIR);
      DessinSegment( 4, x, y, COULEUR_NOIR);
      DessinSegment( 0, x, y, color);
      DessinSegment( 1, x, y, color);
      DessinSegment( 3, x, y, color);
      DessinSegment( 5, x, y, color);
      DessinSegment( 6, x, y, color);
    break;
    case 6:
      DessinSegment( 2, x, y, COULEUR_NOIR);
      DessinSegment( 0, x, y, color);
      DessinSegment( 1, x, y, color);
      DessinSegment( 3, x, y, color);
      DessinSegment( 4, x, y, color);
      DessinSegment( 5, x, y, color);
      DessinSegment( 6, x, y, color);
    break;
    case 7:
      DessinSegment( 1, x, y, COULEUR_NOIR);
      DessinSegment( 3, x, y, COULEUR_NOIR);
      DessinSegment( 4, x, y, COULEUR_NOIR);
      DessinSegment( 6, x, y, COULEUR_NOIR);
      DessinSegment( 0, x, y, color);
      DessinSegment( 2, x, y, color);
      DessinSegment( 5, x, y, color);
    break;
    case 8:
      DessinSegment( 0, x, y, color);
      DessinSegment( 1, x, y, color);
      DessinSegment( 2, x, y, color);
      DessinSegment( 3, x, y, color);
      DessinSegment( 4, x, y, color);
      DessinSegment( 5, x, y, color);
      DessinSegment( 6, x, y, color);
    break;
    case 9:
      DessinSegment( 4, x, y, COULEUR_NOIR);
      DessinSegment( 0, x, y, color);
      DessinSegment( 1, x, y, color);
      DessinSegment( 2, x, y, color);
      DessinSegment( 3, x, y, color);
      DessinSegment( 5, x, y, color);
      DessinSegment( 6, x, y, color);
    break;
       
  } /* End of switch */
#ifdef ECRAN_EXT
    backgroundLayer.swapBuffers();
#endif

} /* End of DessineChiffre */
