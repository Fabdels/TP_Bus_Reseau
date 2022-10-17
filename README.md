# TP_Bus_Reseau


2.1. Capteur BMP280

1.

L'adresse est 111011x. 
Les 6 bits de poids forts sont fixés. 
The last bit is changeable by SDO
Connecter SDO au GND => 1110110 (0x76) est l'adresse esclave; connecter VDDIO au GND => 1110111 (0x77) est l'adresse esclave.


2.

Le registre d'identification est le register 0xD0 “id”
Le registre “id” contient le numéro d'identification de puce chip_id[7:0], qui est 0x58. Ce nombre peut être lu as soon dès que le système a terminé son power-on-reset. 


3.

Le BMP280 offre 3 modes : sleep mode, forced mode et normal mode. Il est possible de choisir le mode en modifiant les bits mode[1:0] dans le registre de contrôle 0xF4. 

mode[1:0] Mode
00 Sleep mode
01 and 10 Forced mode
11 Normal mode

4.

Les données d'étalonnage sont stockées dans les registre calib00 à calib25, aux adresses 0x88 à 0xA1 (en little endian).

5.

La température se trouve dans les registres temp_xlsb (4 bits), temp_lsb (8 bits) et temp_mlsb (8 bits), aux adresses 0xFA, 0xFB et 0xFC.

6.

La pression se trouve dans les registres press_xlsb (4 bits), press_lsb (8 bits) et press_mlsb (8 bits), aux adresses 0xF7, 0xF8 et 0xF9.




7.

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
BMP280_S32_t t_fine;
BMP280_S32_t bmp280_compensate_T_int32(BMP280_S32_t adc_T)
{
BMP280_S32_t var1, var2, T;
var1 = ((((adc_T>>3) – ((BMP280_S32_t)dig_T1<<1))) * ((BMP280_S32_t)dig_T2)) >> 11;
var2 = (((((adc_T>>4) – ((BMP280_S32_t)dig_T1)) * ((adc_T>>4) – ((BMP280_S32_t)dig_T1))) >> 12) *
((BMP280_S32_t)dig_T3)) >> 14;
t_fine = var1 + var2;
T = (t_fine * 5 + 128) >> 8;
return T;
}
“”–
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
BMP280_U32_t bmp280_compensate_P_int64(BMP280_S32_t adc_P)
{
BMP280_S64_t var1, var2, p;
var1 = ((BMP280_S64_t)t_fine) – 128000;
var2 = var1 * var1 * (BMP280_S64_t)dig_P6;
var2 = var2 + ((var1*(BMP280_S64_t)dig_P5)<<17);
var2 = var2 + (((BMP280_S64_t)dig_P4)<<35);
var1 = ((var1 * var1 * (BMP280_S64_t)dig_P3)>>8) + ((var1 * (BMP280_S64_t)dig_P2)<<12);
var1 = (((((BMP280_S64_t)1)<<47)+var1))*((BMP280_S64_t)dig_P1)>>33;
if (var1 == 0)
{
return 0; // avoid exception caused by division by zero
}
p = 1048576-adc_P;
p = (((p<<31)-var2)*3125)/var1;
var1 = (((BMP280_S64_t)dig_P9) * (p>>13) * (p>>13)) >> 25;
var2 = (((BMP280_S64_t)dig_P8) * p) >> 19;
p = ((p + var1 + var2) >> 8) + (((BMP280_S64_t)dig_P7)<<4);
return (BMP280_U32_t)p;



