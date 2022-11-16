/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "capteurBMP.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART_TX_BUFFER_SIZE 300
#define UART_RX_BUFFER_SIZE 1
#define CMD_BUFFER_SIZE 64
#define MAX_ARGS 9
// LF = line feed, saut de ligne
#define ASCII_LF 0x0A
// CR = carriage return, retour chariot
#define ASCII_CR 0x0D
// DEL = delete
#define ASCII_DEL 0x7F
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t prompt[]="\r\nuser@Nucleo-F446>>";
uint8_t started[]=
		"\r\n*-----------------------------*"
		"\r\n| Welcome on Nucleo-F446 |"
		"\r\n*-----------------------------*"
		"\r\n";
uint8_t newline[]="\r\n";
uint8_t cmdNotFound[]="Command not found\r\n";
uint32_t uartRxReceived;
uint8_t uartRxBuffer[UART_RX_BUFFER_SIZE];
uint8_t uartTxBuffer[UART_TX_BUFFER_SIZE];
uint8_t commandList[] =
		"\r\nhelp :		Affiche cette liste"
		"\r\nhello :		Try to say hello"
		"\r\nGET_T :		Température compensée sur 10 caractères"
		"\r\nGET_P :		Pression compensée sur 10 caractères"
		"\r\nSET_K = ... :	Fixe le coefficient K (en 1/100e)"
		"\r\nGET_K :		Coefficient K sur 10 caractères"
		"\r\nGET_A :		Angle sur 10 caractères";
uint8_t BMP280_I2Caddr = 0x77<<1;
uint8_t calibrationParam[calibSize];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void I2C1_test(void);
void BMPconfig(void);
void BMPcalibrate(char affiche);
void affichMes(char compenser);
double bmp280_compensate_T_double(BMP280_S32_t adc_T);
double bmp280_compensate_P_double(BMP280_S32_t adc_P);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void I2C1_test(void){
	int I2C_transmit = 0xD0;
	int I2C_receive = 0x00000000;

	HAL_I2C_Master_Transmit(&hi2c1,BMP280_I2Caddr, &I2C_transmit, 1, HAL_MAX_DELAY);
	HAL_I2C_Master_Receive(&hi2c1, BMP280_I2Caddr, &I2C_receive, 1, HAL_MAX_DELAY);

	printf("I2C1.1 : '%x'\r\n", I2C_receive); //affiche <<I2C1.1 : '58'>>
}

void BMPconfig(){
	uint8_t normalMode = 0b11;
	uint8_t presOvSamp_x16 = 0b101<<2;
	uint8_t tempOvSamp_x02 = 0b010<<5;
	uint8_t ctrl_meas = 0xF4;
	uint8_t I2C_transmitMsg[2] = {ctrl_meas, tempOvSamp_x02|presOvSamp_x16|normalMode};
	uint8_t I2C_receiveMsg[2];

	HAL_I2C_Master_Transmit(&hi2c1,BMP280_I2Caddr, I2C_transmitMsg, 2, HAL_MAX_DELAY);
	HAL_I2C_Master_Receive(&hi2c1, BMP280_I2Caddr, I2C_receiveMsg, 1, HAL_MAX_DELAY);
	printf("BMPconfig: '%x'\r\n", I2C_receiveMsg[0]); //affiche <<BMPconfig: '57'>>
}

void BMPcalibrate(char affiche){
	uint8_t I2C_transmitMsgV = calib00;
	uint8_t I2C_receiveMsg[calibSize];
	int i;

	HAL_I2C_Master_Transmit(&hi2c1,BMP280_I2Caddr, &I2C_transmitMsgV, 1, HAL_MAX_DELAY);
	HAL_I2C_Master_Receive(&hi2c1, BMP280_I2Caddr, I2C_receiveMsg, calibSize, HAL_MAX_DELAY);

	for(i=0;i<calibSize;i++){
		calibrationParam[i] = I2C_receiveMsg[i];
		if (affiche) printf("Config register %x: '%x'\r\n", calib00 + i, I2C_receiveMsg[i]);
	}
}

void affichMes(char compenser){
	uint8_t I2C_transmitMsgV = press_msb;
	uint8_t I2C_receiveMsg[measureSize];
	int measTemp, measPress, realTemp, realPress;

	HAL_I2C_Master_Transmit(&hi2c1,BMP280_I2Caddr, &I2C_transmitMsgV, 1, HAL_MAX_DELAY);
	HAL_I2C_Master_Receive(&hi2c1, BMP280_I2Caddr, I2C_receiveMsg, measureSize, HAL_MAX_DELAY);
	measPress = I2C_receiveMsg[0]<<(12)|I2C_receiveMsg[1]<<(4)|I2C_receiveMsg[2]>>(4);
	measTemp = I2C_receiveMsg[3]<<(12)|I2C_receiveMsg[4]<<(4)|I2C_receiveMsg[5]>>(4);
	if (!compenser){
		printf("pression: '%x'\r\n", measPress);
		printf("temperature: '%x'\r\n", measTemp);
	}
	else{
		//measTemp = 519888;
		//measPress = 415148;

		realTemp = bmp280_compensate_T_double(measTemp);
		realPress = bmp280_compensate_P_double(measPress);

		printf("pression: '%d'\r\n", realPress/25600);
		printf("temperature: '%d'\r\n", realTemp/100);
	}
}

// t_fine carries fine temperature as global value
BMP280_S32_t t_fine;
// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123�? equals 51.23 DegC.
// Returns temperature in DegC, double precision. Output value of “51.23” equals 51.23 DegC.
// t_fine carries fine temperature as global value
BMP280_S32_t t_fine;
double bmp280_compensate_T_double(BMP280_S32_t adc_T)
{
	unsigned short dig_T1 = calibrationParam[0] | calibrationParam[1] <<16;
	signed short dig_T2 = calibrationParam[2] | calibrationParam[3] <<16;
	signed short dig_T3 = calibrationParam[4] | (calibrationParam[5] <<16);



	double var1, var2, T;
	var1 = (((double)adc_T)/16384.0-((double)dig_T1)/1024.0)*((double)dig_T2);
	var2 = ((((double)adc_T)/131072.0-((double)dig_T1)/8192.0)*
			(((double)adc_T)/131072.0-((double) dig_T1)/8192.0))*((double)dig_T3);
	t_fine = (BMP280_S32_t)(var1 + var2);
	T = (var1 + var2) / 5120.0;
	return T;
}
// Returns pressure in Pa as double. Output value of “96386.2” equals 96386.2 Pa = 963.862 hPa
double bmp280_compensate_P_double(BMP280_S32_t adc_P)
{

	unsigned short dig_P1 = calibrationParam[6] | calibrationParam[7] <<16;
	signed short dig_P2 = calibrationParam[8] | calibrationParam[9] <<16;
	signed short dig_P3 = calibrationParam[10] | (calibrationParam[11] <<16);
	signed short dig_P4 = calibrationParam[12] | calibrationParam[13] <<16;
	signed short dig_P5 = calibrationParam[14] | calibrationParam[15] <<16;
	signed short dig_P6 = calibrationParam[16] | (calibrationParam[17] <<16);
	signed short dig_P7 = calibrationParam[18] | calibrationParam[19] <<16;
	signed short dig_P8 = calibrationParam[20] | calibrationParam[21] <<16;
	signed short dig_P9 = calibrationParam[22] | (calibrationParam[23] <<16);


	double var1, var2, p;
	var1 = ((double)t_fine/2.0) - 64000.0;
	var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
	var2 = var2 + var1 * ((double)dig_P5) * 2.0;
	var2 = (var2/4.0)+(((double)dig_P4) * 65536.0);
	var1 = (((double)dig_P3) * var1 * var1 / 524288.0 + ((double)dig_P2) * var1) / 524288.0;
	var1 = (1.0 + var1 / 32768.0)*((double)dig_P1);
	if (var1 == 0.0)
	{
		return 0; // avoid exception caused by division by zero
	}
	p = 1048576.0 - (double)adc_P;
	p = (p - (var2 / 4096.0)) * 6250.0 / var1;
	var1 = ((double)dig_P9) * p * p / 2147483648.0;
	var2 = p * ((double)dig_P8) / 32768.0;
	p = p + (var1 + var2 + ((double)dig_P7)) / 16.0;
	return p;
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* USER CODE BEGIN 1 */
	char	 	cmdBuffer[CMD_BUFFER_SIZE];
	int 		idx_cmd;
	char* 		argv[MAX_ARGS];
	int		 	argc = 0;
	char*		token;
	int 		newCmdReady = 0;
	int			speedValue = 0;
	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_CAN1_Init();
	MX_I2C1_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	/* USER CODE BEGIN 2 */

	memset(argv,NULL,MAX_ARGS*sizeof(char*));
	memset(cmdBuffer,NULL,CMD_BUFFER_SIZE*sizeof(char));
	memset(uartRxBuffer,NULL,UART_RX_BUFFER_SIZE*sizeof(char));
	memset(uartTxBuffer,NULL,UART_TX_BUFFER_SIZE*sizeof(char));

	HAL_UART_Receive_IT(&huart1, uartRxBuffer, UART_RX_BUFFER_SIZE);
	HAL_Delay(10);
	HAL_UART_Transmit(&huart1, started, sizeof(started), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart1, prompt, sizeof(prompt), HAL_MAX_DELAY);

	char afficheurCompteur = 0;

	HAL_CAN_Start(&hcan1);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */


	printf("\r\nHelloWorld!\r\n\n\n");
	I2C1_test();
	BMPconfig();
	BMPcalibrate(1);

	uint8_t pData = 0;

	//Interface CAN :
	/*
	CAN_TxHeaderTypeDef Header =  { .StdId = 0x60,
			.ExtId = 0,
			.IDE = CAN_ID_STD,
			.RTR = CAN_RTR_DATA,
			.DLC = 3,
			.TransmitGlobalTime = DISABLE};


	uint8_t aData[] = {0, 90, 0x01};
	uint32_t pTxMailbox[0];

	uint32_t status1 = 12;
	uint32_t status2 = 12;
	printf("%d, %d\r\n",status1, status2);
	*/


	while (1)
	{

		//UART 2

		if(uartRxReceived){
			switch(uartRxBuffer[0]){
			// Nouvelle ligne, instruction à traiter
			case ASCII_CR:
				HAL_UART_Transmit(&huart1, newline, sizeof(newline), HAL_MAX_DELAY);
				cmdBuffer[idx_cmd] = '\0';
				argc = 0;
				token = strtok(cmdBuffer, " ");
				while(token!=NULL){
					argv[argc++] = token;
					token = strtok(NULL, " ");
				}

				idx_cmd = 0;
				newCmdReady = 1;
				break;

				// Suppression du dernier caractère
			case ASCII_DEL:
				cmdBuffer[idx_cmd--] = '\0';
				HAL_UART_Transmit(&huart1, uartRxBuffer, UART_RX_BUFFER_SIZE, HAL_MAX_DELAY);
				break;

				// Nouveau caractère
			default:
				cmdBuffer[idx_cmd++] = uartRxBuffer[0];
				HAL_UART_Transmit(&huart1, uartRxBuffer, UART_RX_BUFFER_SIZE, HAL_MAX_DELAY);
			}
			uartRxReceived = 0;
		}

		if(newCmdReady){

			// Showing commands
			if(strcmp(argv[0],"help")==0){
				sprintf(uartTxBuffer,commandList);
				HAL_UART_Transmit(&huart1, uartTxBuffer, sizeof(commandList), HAL_MAX_DELAY);
			}

			// Saying hello
			else if(strcmp(argv[0],"hello")==0){
				sprintf(uartTxBuffer,"Hello human !");
				HAL_UART_Transmit(&huart1, uartTxBuffer, strlen(uartTxBuffer), HAL_MAX_DELAY);
			}

			// Get temperature
			else if(strcmp(argv[0],"GET_T")==0){
				uint8_t I2C_transmitMsgV = press_msb;
				uint8_t I2C_receiveMsg[measureSize];
				int measTemp;

				HAL_I2C_Master_Transmit(&hi2c1,BMP280_I2Caddr, &I2C_transmitMsgV, 1, HAL_MAX_DELAY);
				HAL_I2C_Master_Receive(&hi2c1, BMP280_I2Caddr, I2C_receiveMsg, measureSize, HAL_MAX_DELAY);
				measTemp = I2C_receiveMsg[3]<<(12)|I2C_receiveMsg[4]<<(4)|I2C_receiveMsg[5]>>(4);
				realTemp = bmp280_compensate_T_double(measTemp);


				sprintf(uartTxBuffer,"T = %x°C\n\r", realTemp);
				HAL_UART_Transmit(&huart1, uartTxBuffer, strlen(uartTxBuffer), HAL_MAX_DELAY);
			}

			// Get pressure
			else if(strcmp(argv[0],"GET_P")==0){
				int measPress;
				uint8_t I2C_transmitMsgV = press_msb;
				uint8_t I2C_receiveMsg[measureSize];

				HAL_I2C_Master_Transmit(&hi2c1,BMP280_I2Caddr, &I2C_transmitMsgV, 1, HAL_MAX_DELAY);
				HAL_I2C_Master_Receive(&hi2c1, BMP280_I2Caddr, I2C_receiveMsg, measureSize, HAL_MAX_DELAY);
				measPress = I2C_receiveMsg[0]<<(12)|I2C_receiveMsg[1]<<(4)|I2C_receiveMsg[2]>>(4);
				realPress = bmp280_compensate_P_double(measPress);

				sprintf(uartTxBuffer,"P = %xPa\n\r", realPress);
				HAL_UART_Transmit(&huart1, uartTxBuffer, strlen(uartTxBuffer), HAL_MAX_DELAY);

			}

			// Set K
			else if(strcmp(argv[0],"SET_K")==0){
				sprintf(uartTxBuffer,"SET_K=OK");
				HAL_UART_Transmit(&huart1, uartTxBuffer, strlen(uartTxBuffer), HAL_MAX_DELAY);
			}

			// Get coefficent K
			else if(strcmp(argv[0],"GET_K")==0){
				sprintf(uartTxBuffer,"K = ...");
				HAL_UART_Transmit(&huart1, uartTxBuffer, strlen(uartTxBuffer), HAL_MAX_DELAY);
			}

			// Get angle
			else if(strcmp(argv[0],"GET_A")==0){
				sprintf(uartTxBuffer,"A = ...°");
				HAL_UART_Transmit(&huart1, uartTxBuffer, strlen(uartTxBuffer), HAL_MAX_DELAY);
			}



			//Command not found
			else if(strcmp(argv[0],"get")==0)
			{
				HAL_UART_Transmit(&huart1, cmdNotFound, sizeof(cmdNotFound), HAL_MAX_DELAY);
			}
			else{
				HAL_UART_Transmit(&huart1, cmdNotFound, sizeof(cmdNotFound), HAL_MAX_DELAY);
			}
			HAL_UART_Transmit(&huart1, prompt, sizeof(prompt), HAL_MAX_DELAY);
			newCmdReady = 0;
		}



		/*
		//Interface CAN :

		if(HAL_OK != HAL_CAN_AddTxMessage(&hcan1, &Header, aData, pTxMailbox)){
			printf("error(1)\r\n");
		}
		status1 = HAL_CAN_IsTxMessagePending(&hcan1, pTxMailbox);
		//HAL_Delay(1000);
		HAL_Delay(1000);

		aData[0] = 1;
		status2 = HAL_CAN_IsTxMessagePending(&hcan1, pTxMailbox);
		if(HAL_OK != HAL_CAN_AddTxMessage(&hcan1, &Header, aData, pTxMailbox)){
			printf("error (2)\r\n");
		}
		HAL_Delay(1000);
		aData[0] = 0;
		printf("%d,%d\r\n",status1, status2);

		*/






		//Strange UART

		//HAL_UART_Receive(USART1, &pData, sizeof(uint8_t), HAL_MAX_DELAY);
		//printf("%d lettre: %c\r\n",afficheurCompteur, pData);
		//HAL_Delay(2000);
		//HAL_UART_Transmit(USART1, &pData, sizeof(uint8_t), HAL_MAX_DELAY);


		//I2C :
		/*
		affichMes(0);
		affichMes(1);
		afficheurCompteur = 1 - afficheurCompteur;
		printf("%d\r\n", afficheurCompteur);
		HAL_Delay(2000);
		 */

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 16;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 2;
	RCC_OscInitStruct.PLL.PLLR = 2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback (UART_HandleTypeDef * huart){
	uartRxReceived = 1;
	HAL_UART_Receive_IT(&huart1, uartRxBuffer, UART_RX_BUFFER_SIZE);
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
