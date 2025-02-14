/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
char lidar_rx_buff[9] = {0};
char RS485_rx_buff[9] = {0};
char buff[] = "00.00 cm";
uint8_t cmd[] = {0x5A, 0x05, 0x07, 0x01, 0x67}; // Comando exemplo (substitua pelo comando correto)
uint8_t rcv_byte,
/*----*/rcv_byte1,
/*----*/flag_pacote_completo,
/*----*/flag_pacote_completo1,
/*----*/auxbuff1,
/*----*/init_rcv,
/*----*/auxbuff = 0;
uint16_t altura_sensor,
/*-----*/cont[8]= {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
uint8_t leitura_buffer(void);
void trata_uart_tfmini(void);
void trata_string(void);
void transmite_tfmini(void);
void trata_uart_rs485(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART2) {	/*USART 2 RS485*/
		if(rcv_byte1 == '[')
			init_rcv = 1;
		if(init_rcv){
			RS485_rx_buff[auxbuff] = rcv_byte1;
			auxbuff++;
		}
		if(rcv_byte == ']'){
			flag_pacote_completo1 = 1;
			auxbuff = 0;
		}
		HAL_UART_Receive_IT(&huart2, (uint8_t*)&rcv_byte1, sizeof(rcv_byte1));	/*RECEBE UM BYTE*/
	}
	else if(huart->Instance == USART1) {	/*USART 1 SENSOR LIDAR TFMINI PLUS*/
		lidar_rx_buff[auxbuff] = rcv_byte;
		auxbuff++;
		if(auxbuff>=9){
			flag_pacote_completo = 1;
			auxbuff = 0;
		}
		else
			HAL_UART_Receive_IT(&huart1, (uint8_t*)&rcv_byte, sizeof(rcv_byte));	/*RECEBE UM BYTE*/
	}
}
void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef * htim){//ENDEREÇO DE HTIM COMO PARAMETRO
	if(htim->Instance == TIM3){ 	//IRA LER O MEMBRO DA ESTRUTURA APONTADO PELO PONTEIRO PASSADO COMO ARGUMENTO (*htim/nome da estrutura) DA FUNC CALL BACK
		cont[0]++;							/*FUNÇÃO DE TRATAMENTO DE INTERRUPÇÃO POR OVERFLOW TIMER3*/ /*Instance É A FLAG QUE INDICA QUAL TIMER GEROU A INTERRUPÇÃO*/
		if(cont[0]>=250){
			HAL_GPIO_TogglePin(LPCB_GPIO_Port, LPCB_Pin);
			cont[0] = 0;
		}
	}
}
uint8_t leitura_buffer(void){	/*LE O CONTEUDO VINDO DA UART E FAZ A CONVERSÃO JUNTANDO A PARTE ALTA E BAIXA DO VALOR*/
	for(int i = 0; i<7; i++){
		if(lidar_rx_buff[i] == 0x59 && lidar_rx_buff[i+1] == 0x59){
			altura_sensor = (lidar_rx_buff[i+3] << 8) | lidar_rx_buff[i+2];
			return 0;
		}
	}
	return 1;
}
void trata_uart_tfmini(void){	/*TRATA O PACOTE RECEBIDO NA UART 3 ENVIADO PELO SENSOR*/
	leitura_buffer();
	flag_pacote_completo = 0;
}
/*SER�? IMPLEMENTADO DEPOIS*/
void trata_string(void){
	char altura[6] = {0};
	sprintf(altura,"%04d",altura_sensor);
}
void transmite_tfmini(void){	/*TRATA A STRING E TRANSMITE VIA RF*/
	char buff1[12] = "0000 cm\n";
	sprintf(buff1, "[G,RX,%04d]", altura_sensor);
	HAL_GPIO_WritePin(F_CONTROL_GPIO_Port, F_CONTROL_Pin, GPIO_PIN_SET);		/*SETA O DISPOSITIVO PARA TRANSMISSÃO*/
	HAL_UART_Transmit(&huart2, (uint8_t*)buff1, sizeof(buff1), 0xFF);
	HAL_GPIO_WritePin(F_CONTROL_GPIO_Port, F_CONTROL_Pin, GPIO_PIN_RESET);		/*SETA O DISPOSITIVO PARA RECEBIMENTO*/
}
void trata_uart_rs485(void){
	char temp[] = "ERROR";
	if(RS485_rx_buff[1] == 'g' && RS485_rx_buff[3] == 'T'){
		flag_pacote_completo1 = 0;
		init_rcv = 0;
		transmite_tfmini();
	}
	else{
		flag_pacote_completo1 = 0;
		init_rcv = 0;
		HAL_GPIO_WritePin(F_CONTROL_GPIO_Port, F_CONTROL_Pin, GPIO_PIN_SET);		/*SETA O DISPOSITIVO PARA TRANSMISSÃO*/
		HAL_UART_Transmit(&huart2, (uint8_t*)temp, sizeof(temp), 0xFF);
		HAL_GPIO_WritePin(F_CONTROL_GPIO_Port, F_CONTROL_Pin, GPIO_PIN_RESET);		/*SETA O DISPOSITIVO PARA RECEBIMENTO*/
	}
	HAL_UART_Receive_IT(&huart2, (uint8_t*)&rcv_byte1, sizeof(rcv_byte1));	/*RECEBE UM BYTE*/
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* USER CODE BEGIN 1 */

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
	MX_I2C1_Init();
	MX_TIM3_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	/* USER CODE BEGIN 2 */
	HAL_TIM_Base_Start_IT(&htim3); //INCIA TIMER3 COM INTERRUPÇÃO NO OVERFLOW
	HAL_GPIO_WritePin(F_CONTROL_GPIO_Port, F_CONTROL_Pin, GPIO_PIN_RESET);		/*SETA O DISPOSITIVO PARA RECEBIMENTO*/
	HAL_UART_Receive_IT(&huart1, (uint8_t*)&rcv_byte, 1);
	HAL_UART_Receive_IT(&huart2, (uint8_t*)&rcv_byte1, 1);
	transmite_tfmini();
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		if(flag_pacote_completo1){
			trata_uart_rs485();
		}
		if(flag_pacote_completo){
			trata_uart_tfmini();
			HAL_UART_Receive_IT(&huart1, (uint8_t*)&rcv_byte, sizeof(rcv_byte));/*TRIGGER PARA RECEBER NOVAMENTE SENSOR LIDAR*/
		}
		HAL_Delay(100);
		transmite_tfmini();
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

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void)
{

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 100000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief TIM3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM3_Init(void)
{

	/* USER CODE BEGIN TIM3_Init 0 */

	/* USER CODE END TIM3_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	/* USER CODE BEGIN TIM3_Init 1 */

	/* USER CODE END TIM3_Init 1 */
	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 287;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 249;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM3_Init 2 */

	/* USER CODE END TIM3_Init 2 */

}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void)
{

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 9600;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LPCB_GPIO_Port, LPCB_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(F_CONTROL_GPIO_Port, F_CONTROL_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : LPCB_Pin */
	GPIO_InitStruct.Pin = LPCB_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(LPCB_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : F_CONTROL_Pin */
	GPIO_InitStruct.Pin = F_CONTROL_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(F_CONTROL_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : LD1_Pin */
	GPIO_InitStruct.Pin = LD1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(LD1_GPIO_Port, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
