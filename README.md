# TP_Bus_Reseau

---
Ce répertoire contient un projet réseau implémentant l'interfacage de différents dispositifs communicant via différents bus et protocoles réseaux.
- Une carte RasberryPi héberge une server Web créé avec Flask. Cette carte a été programmé sous un envirronement Linux via une connection SSH.
- Une liaison UART permet à la RasberryPi de communiquer avec une carte STM32 Nucleo qui pilote deux capteurs (Pression, température). La communication à ces capteurs se fait via un bus I2C
- Un bus CAN permet de commander un servo-moteur.
---
2.1. Capteur BMP280

2.1.1.
L'adresse est 111011x. 
Les 6 bits de poids forts sont fixés. 
The last bit is changeable by SDO
Connecter SDO au GND => 1110110 (0x76) est l'adresse esclave; connecter VDDIO au GND => 1110111 (0x77) est l'adresse esclave.

2.1.2.
Le registre d'identification est le register 0xD0 “id”
Le registre “id” contient le numéro d'identification de puce chip_id[7:0], qui est 0x58. Ce nombre peut être lu as soon dès que le système a terminé son power-on-reset. 

2.1.3.
Le BMP280 offre 3 modes : sleep mode, forced mode et normal mode. Il est possible de choisir le mode en modifiant les bits mode[1:0] dans le registre de contrôle 0xF4. 

-> mode[1:0] Mode

'00' Sleep mode

'01' and '10' Forced mode

'11' Normal mode

2.1.4.
Les données d'étalonnage sont stockées dans les registre calib00 à calib25, aux adresses 0x88 à 0xA1 (en little endian).

2.1.5.
La température se trouve dans les registres temp_xlsb (4 bits), temp_lsb (8 bits) et temp_mlsb (8 bits), aux adresses 0xFA, 0xFB et 0xFC.

2.1.6.
La pression se trouve dans les registres press_xlsb (4 bits), press_lsb (8 bits) et press_mlsb (8 bits), aux adresses 0xF7, 0xF8 et 0xF9.

2.1.7.
La documentation du capteur BMP280 fournit une formule permettant de calculer les perssion et température compensées à partir des registres de calibration. Deux implémentations sont fournies, une en float avec un code plus court et une autre en int plus légère en ressources. 

---
4.2
@api.route permet d' "organiser" le serveur web (ajout d'un nouveau sous-URL)

<int:index> récupère une valeur entière et la met dans une variable nommée int.

