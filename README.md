# alerte-maree
Le programme est composé de deux éléments distincts, chacun avec son code

- ESP8266 : calcule la dernière heure de maree haute. Sert à calculer une valeur x qui est envoyée à colorduino et correspond au temps passé depuis la dernière pleine mer.
-> tourne sur un nodemcu lolin (prévoir un modèle avec sortie 5V)

- colorduino : prend en entrée la dernière heure de maree et affiche le graphique sur la matrice de LEDS
-> tourne sur un arduino colorduino branché à l'esp8266 en mode esclave SPI
