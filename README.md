# Chronometre_Arc
Chronomètre de tir à l'arc

Ce chronomètre permet l’entraînement des archers à la compétition. Le chronomètre permet de choisir les équipes A/B ou C/D ainsi que de configurer les durées de pré-tir et de tir. Le chronomètre déclenche la sonnerie de pré-tir et de tir. La télécommande fournie avec le chronomètre permet de configurer les paramètres du chronomètre, de lancer le tir, de mettre le tir en pause ou de stopper le tir.
Les fichiers pour le code du chronomètre ainsi que la télécomande sont fournis.
Le modèle 3D pour le télécommande est également fourni au format Fusion 360.
Les plans électroniques pour la carte du chronomètre et celle de la télécommande sont fournis.

Coût d'environs 200 euros pour la partie électronique.

Le chronomètre est constitué de 3 écrans LED 64x32 pour avoir une des chiffres de 12 cm de haut. Les trois écrans sont installés verticalement.
Le chronomètre est piloté par un ESP32 et connecté à la télécommande en LoRa pour une grande portée.
La télécommande fonctionne avec un arduino nano et une puce LoRa.

Attention, erreur de conception, l'ampli est connecté sur une pate de l'esp32 qui n'est pas prévue pour passer un son mp3 ce qui rend impossible de jouer une musique.
Les sons pour démarrer ou stopper le tir ne sont donc pas très agréables.

