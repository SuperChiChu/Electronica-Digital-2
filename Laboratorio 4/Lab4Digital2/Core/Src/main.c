/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

#define DEBOUNCE_MS 200

#define SEG_A (1U << 4)
#define SEG_B (1U << 5)
#define SEG_C (1U << 6)
#define SEG_D (1U << 7)
#define SEG_E (1U << 8)
#define SEG_F (1U << 9)
#define SEG_G (1U << 10)

#define DISPLAY_MASK (SEG_A | SEG_B | SEG_C | SEG_D | \
                      SEG_E | SEG_F | SEG_G)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

volatile uint8_t contador_J1 = 0;
volatile uint8_t contador_J2 = 0;

volatile uint8_t juego_activo = 0;


/* Variables para la cuenta regresiva */
volatile uint8_t cuenta_activa = 0;
volatile uint8_t numero_cuenta = 5;

volatile uint32_t tiempo_cuenta = 0;


/* Variables para antirrebote */
volatile uint32_t ultimo_J1 = 0;
volatile uint32_t ultimo_J2 = 0;
volatile uint32_t ultimo_START = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */

void Mostrar_J1(uint8_t patron);
void Mostrar_J2(uint8_t patron);

void Actualizar_J1(void);
void Actualizar_J2(void);

void Display_Apagado(void);
void Display_Numero(uint8_t numero);

void Ganador_J1(void);
void Ganador_J2(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* ---------------------------------------------------------- */
/* MOSTRAR PATRON EN LOS LEDS DEL JUGADOR 1                   */
/* PC2 - PC5                                                  */
/* ---------------------------------------------------------- */
void Mostrar_J1(uint8_t patron)
{
	uint32_t mascara = (0x0F << 2);
	uint32_t salida = ((patron & 0x0F) << 2);

	uint32_t encender = salida & mascara;
	uint32_t apagar = (~salida) & mascara;

	GPIOC->BSRR = encender | (apagar << 16);
}


/* ---------------------------------------------------------- */
/* MOSTRAR PATRON EN LOS LEDS DEL JUGADOR 2                   */
/* PC6 - PC9                                                  */
/* ---------------------------------------------------------- */
void Mostrar_J2(uint8_t patron)
{
	uint32_t mascara = (0x0F << 6);
	uint32_t salida = ((patron & 0x0F) << 6);

	uint32_t encender = salida & mascara;
	uint32_t apagar = (~salida) & mascara;

	GPIOC->BSRR = encender | (apagar << 16);
}


/* ---------------------------------------------------------- */
/* ACTUALIZAR JUGADOR 1                                       */
/*                                                            */
/* 0 -> 0000                                                  */
/* 1 -> 1000                                                  */
/* 2 -> 0100                                                  */
/* 3 -> 0010                                                  */
/* 4 -> 0001                                                  */
/* 5 -> GANADOR                                               */
/* ---------------------------------------------------------- */
void Actualizar_J1(void)
{
	switch (contador_J1)
	{
		case 0:
			Mostrar_J1(0x00);
			break;

		case 1:
			Mostrar_J1(0x08);
			break;

		case 2:
			Mostrar_J1(0x04);
			break;

		case 3:
			Mostrar_J1(0x02);
			break;

		case 4:
			Mostrar_J1(0x01);
			break;

		case 5:
			Ganador_J1();
			break;

		default:
			break;
	}
}


/* ---------------------------------------------------------- */
/* ACTUALIZAR JUGADOR 2                                       */
/* ---------------------------------------------------------- */
void Actualizar_J2(void)
{
	switch (contador_J2)
	{
		case 0:
			Mostrar_J2(0x00);
			break;

		case 1:
			Mostrar_J2(0x08);
			break;

		case 2:
			Mostrar_J2(0x04);
			break;

		case 3:
			Mostrar_J2(0x02);
			break;

		case 4:
			Mostrar_J2(0x01);
			break;

		case 5:
			Ganador_J2();
			break;

		default:
			break;
	}
}


/* ---------------------------------------------------------- */
/* APAGAR DISPLAY                                             */
/*                                                            */
/* ANODO COMUN                                                */
/* HIGH = APAGADO                                             */
/* LOW  = ENCENDIDO                                           */
/* ---------------------------------------------------------- */
void Display_Apagado(void)
{
	GPIOB->BSRR = DISPLAY_MASK;
}


/* ---------------------------------------------------------- */
/* MOSTRAR NUMERO EN DISPLAY                                  */
/*                                                            */
/* Display de ANODO COMUN                                     */
/* Primero apagamos todos los segmentos.                      */
/* Luego ponemos en LOW solamente los segmentos necesarios.   */
/* ---------------------------------------------------------- */
void Display_Numero(uint8_t numero)
{
	uint32_t segmentos = 0;

	switch (numero)
	{
		case 0:
			segmentos = SEG_A | SEG_B | SEG_C |
					   SEG_D | SEG_E | SEG_F;
			break;

		case 1:
			segmentos = SEG_B | SEG_C;
			break;

		case 2:
			segmentos = SEG_A | SEG_B | SEG_D |
					   SEG_E | SEG_G;
			break;

		case 3:
			segmentos = SEG_A | SEG_B | SEG_C |
					   SEG_D | SEG_G;
			break;

		case 4:
			segmentos = SEG_B | SEG_C | SEG_F |
					   SEG_G;
			break;

		case 5:
			segmentos = SEG_A | SEG_C | SEG_D |
					   SEG_F | SEG_G;
			break;

		default:
			Display_Apagado();
			return;
	}

	/*
	 * Primero todos HIGH = apagados
	 */
	GPIOB->BSRR = DISPLAY_MASK;

	/*
	 * Los bits 16-31 del BSRR ponen el pin en LOW.
	 * En anodo comun LOW = segmento encendido.
	 */
	GPIOB->BSRR = (segmentos << 16);
}


/* ---------------------------------------------------------- */
/* GANADOR JUGADOR 1                                         */
/* ---------------------------------------------------------- */
void Ganador_J1(void)
{
	juego_activo = 0;

	/* J1 = 1111 */
	GPIOC->BSRR =
			(1U << 2) |
			(1U << 3) |
			(1U << 4) |
			(1U << 5);

	/* J2 = 0000 */
	GPIOC->BSRR =
			(1U << (6 + 16)) |
			(1U << (7 + 16)) |
			(1U << (8 + 16)) |
			(1U << (9 + 16));

	/* Mostrar ganador 1 */
	Display_Numero(1);
}


/* ---------------------------------------------------------- */
/* GANADOR JUGADOR 2                                         */
/* ---------------------------------------------------------- */
void Ganador_J2(void)
{
	juego_activo = 0;

	/* J1 = 0000 */
	GPIOC->BSRR =
			(1U << (2 + 16)) |
			(1U << (3 + 16)) |
			(1U << (4 + 16)) |
			(1U << (5 + 16));

	/* J2 = 1111 */
	GPIOC->BSRR =
			(1U << 6) |
			(1U << 7) |
			(1U << 8) |
			(1U << 9);

	/* Mostrar ganador 2 */
	Display_Numero(2);
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

  /* USER CODE BEGIN 2 */

  Mostrar_J1(0x00);
  Mostrar_J2(0x00);

  Display_Apagado();

  juego_activo = 0;
  cuenta_activa = 0;

  /*
   * Permitir que la primera pulsacion sea reconocida
   * inmediatamente.
   */
  ultimo_J1 = HAL_GetTick() - DEBOUNCE_MS;
  ultimo_J2 = HAL_GetTick() - DEBOUNCE_MS;
  ultimo_START = HAL_GetTick() - DEBOUNCE_MS;

  /* USER CODE END 2 */

  /* Initialize leds */
  BSP_LED_Init(LED2);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


	/*
	 * ----------------------------------------------------------
	 * CUENTA REGRESIVA
	 * ----------------------------------------------------------
	 *
	 * Esta rutina NO bloquea el procesador.
	 *
	 * Cada vez que pasan 1000 ms:
	 *
	 * 5 -> 4 -> 3 -> 2 -> 1 -> 0
	 *
	 */
	if (cuenta_activa)
	{
		uint32_t ahora = HAL_GetTick();

		if ((ahora - tiempo_cuenta) >= 1000)
		{
			tiempo_cuenta = ahora;

			if (numero_cuenta > 0)
			{
				numero_cuenta--;

				Display_Numero(numero_cuenta);
			}

			else
			{
				/*
				 * El 0 ya estuvo mostrado durante 1 segundo.
				 * Termina la cuenta regresiva.
				 */
				cuenta_activa = 0;

				Display_Apagado();

				/*
				 * Ahora SI empieza la carrera.
				 */
				juego_activo = 1;
			}
		}
	}


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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9,
                          GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9,
                          GPIO_PIN_SET);

  /*Configure GPIO pins : PC2 PC3 PC4 PC5
                           PC6 PC7 PC8 PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_J1_Pin BTN_J2_Pin BTN_START_Pin */
  GPIO_InitStruct.Pin = BTN_J1_Pin|BTN_J2_Pin|BTN_START_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : USART_TX_Pin USART_RX_Pin */
  GPIO_InitStruct.Pin = USART_TX_Pin|USART_RX_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB10 PB4 PB5 PB6
                           PB7 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


/* ---------------------------------------------------------- */
/* INTERRUPCIONES DE LOS BOTONES                              */
/* ---------------------------------------------------------- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	uint32_t ahora = HAL_GetTick();


	/* ------------------------------------------------------ */
	/* BOTON START                                            */
	/* ------------------------------------------------------ */
	if (GPIO_Pin == BTN_START_Pin)
	{
		if ((ahora - ultimo_START) >= DEBOUNCE_MS)
		{
			ultimo_START = ahora;

			/*
			 * Deshabilitar jugadores inmediatamente.
			 */
			juego_activo = 0;

			/*
			 * Reiniciar carrera.
			 */
			contador_J1 = 0;
			contador_J2 = 0;

			Mostrar_J1(0x00);
			Mostrar_J2(0x00);

			/*
			 * Iniciar cuenta regresiva.
			 */
			numero_cuenta = 5;
			cuenta_activa = 1;

			tiempo_cuenta = ahora;

			/*
			 * Mostrar 5 inmediatamente.
			 */
			Display_Numero(5);
		}
	}


	/* ------------------------------------------------------ */
	/* BOTON JUGADOR 1                                       */
	/* ------------------------------------------------------ */
	else if (GPIO_Pin == BTN_J1_Pin)
	{
		/*
		 * Solo se permite avanzar cuando la carrera
		 * ya esta activa.
		 */
		if (juego_activo)
		{
			if ((ahora - ultimo_J1) >= DEBOUNCE_MS)
			{
				ultimo_J1 = ahora;

				contador_J1++;

				Actualizar_J1();
			}
		}
	}


	/* ------------------------------------------------------ */
	/* BOTON JUGADOR 2                                       */
	/* ------------------------------------------------------ */
	else if (GPIO_Pin == BTN_J2_Pin)
	{
		if (juego_activo)
		{
			if ((ahora - ultimo_J2) >= DEBOUNCE_MS)
			{
				ultimo_J2 = ahora;

				contador_J2++;

				Actualizar_J2();
			}
		}
	}
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

#ifdef USE_FULL_ASSERT

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
