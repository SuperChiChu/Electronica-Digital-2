/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "audio_vf_pwm.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define NOTE_F4   349
#define NOTE_GS4  415
#define NOTE_A4   440

#define NOTE_C5   523
#define NOTE_E5   659
#define NOTE_F5   698
#define NOTE_GS5  831
#define NOTE_A5   880

#define REST      0

#define DAC_SAMPLE_RATE 8000U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

ADC_HandleTypeDef hadc1;

DAC_HandleTypeDef hdac;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

volatile uint32_t audio_index = 0;
volatile uint8_t audio_playing = 0;
volatile uint8_t audio_finished = 0;

volatile uint8_t dac_playing = 0;
volatile uint32_t dac_phase_acc = 0;
volatile uint32_t dac_phase_step = 0;

const uint16_t sine_lut[64] =
{
    2048, 2248, 2447, 2642, 2831, 3013, 3185, 3346,
    3495, 3630, 3750, 3853, 3939, 4007, 4056, 4085,
    4095, 4085, 4056, 4007, 3939, 3853, 3750, 3630,
    3495, 3346, 3185, 3013, 2831, 2642, 2447, 2248,
    2048, 1847, 1648, 1453, 1264, 1082, 910, 749,
    600, 465, 345, 242, 156, 88, 39, 10,
    0, 10, 39, 88, 156, 242, 345, 465,
    600, 749, 910, 1082, 1264, 1453, 1648, 1847
};

const uint16_t dac_notes[] =
{
    NOTE_A4, NOTE_A4, NOTE_A4, NOTE_F4, NOTE_C5,
    NOTE_A4, NOTE_F4, NOTE_C5, NOTE_A4, REST,

    NOTE_E5, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_C5,
    NOTE_GS4, NOTE_F4, NOTE_C5, NOTE_A4, REST,

    NOTE_A5, NOTE_A4, NOTE_A4,
    NOTE_A5, NOTE_GS5, NOTE_A5
};

const uint16_t dac_durations[] =
{
    500, 500, 500, 350, 150,
    500, 350, 150, 650, 350,

    500, 500, 500, 350, 150,
    500, 350, 150, 650, 350,

    500, 250, 250,
    400, 300, 300
};

const uint16_t DAC_AUDIO_LENGTH =
    sizeof(dac_notes) / sizeof(dac_notes[0]);

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM6_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
static void MX_DAC_Init(void);

/* USER CODE BEGIN PFP */

void uart_print(const char *text);
void print_menu(void);
void play_audio_pwm(void);
void play_audio_dac(void);
void dac_set_tone(uint32_t frequency);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void uart_print(const char *text)
{
    HAL_UART_Transmit(&huart2,
                      (uint8_t *)text,
                      strlen(text),
                      HAL_MAX_DELAY);
}

void print_menu(void)
{
    uart_print("\r\n============================\r\n");
    uart_print("    REPRODUCTOR DE AUDIO\r\n");
    uart_print("============================\r\n");
    uart_print("1. Reproducir Audio PWM\r\n");
    uart_print("2. Reproducir Audio DAC\r\n");
    uart_print("\r\nIngrese una opcion: ");
}

void play_audio_pwm(void)
{
    dac_playing = 0;

    HAL_TIM_Base_Stop_IT(&htim6);
    HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);

    audio_index = 0;
    audio_finished = 0;
    audio_playing = 1;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 128);

    __HAL_TIM_SET_COUNTER(&htim3, 0);
    __HAL_TIM_SET_COUNTER(&htim6, 0);

    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)
    {
        Error_Handler();
    }

    while (audio_finished == 0)
    {
    }

    HAL_TIM_Base_Stop_IT(&htim6);

    audio_playing = 0;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);

    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

void dac_set_tone(uint32_t frequency)
{
    if (frequency == REST)
    {
        dac_phase_step = 0;

        HAL_DAC_SetValue(&hdac,
                         DAC_CHANNEL_1,
                         DAC_ALIGN_12B_R,
                         2048);
    }
    else
    {
        dac_phase_step =
            (uint32_t)(((uint64_t)frequency *
                        64ULL *
                        65536ULL) /
                       DAC_SAMPLE_RATE);
    }
}

void play_audio_dac(void)
{
    audio_playing = 0;

    HAL_TIM_Base_Stop_IT(&htim6);

    __HAL_TIM_SET_COMPARE(&htim3,
                          TIM_CHANNEL_1,
                          0);

    HAL_TIM_PWM_Stop(&htim3,
                     TIM_CHANNEL_1);

    dac_phase_acc = 0;
    dac_phase_step = 0;
    dac_playing = 1;

    if (HAL_DAC_Start(&hdac,
                      DAC_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_DAC_SetValue(&hdac,
                     DAC_CHANNEL_1,
                     DAC_ALIGN_12B_R,
                     2048);

    __HAL_TIM_SET_COUNTER(&htim6, 0);

    if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)
    {
        Error_Handler();
    }

    for (uint16_t i = 0; i < DAC_AUDIO_LENGTH; i++)
    {
        dac_set_tone(dac_notes[i]);

        HAL_Delay(
            (uint32_t)dac_durations[i] * 85U / 100U
        );

        dac_set_tone(REST);

        HAL_Delay(
            (uint32_t)dac_durations[i] * 15U / 100U
        );
    }

    dac_set_tone(REST);

    dac_playing = 0;

    HAL_TIM_Base_Stop_IT(&htim6);

    HAL_DAC_SetValue(&hdac,
                     DAC_CHANNEL_1,
                     DAC_ALIGN_12B_R,
                     0);

    HAL_DAC_Stop(&hdac,
                 DAC_CHANNEL_1);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        if (audio_playing == 1)
        {
            if (audio_index < AUDIO_VF_PWM_LENGTH)
            {
                __HAL_TIM_SET_COMPARE(&htim3,
                                      TIM_CHANNEL_1,
                                      audio_vf_pwm[audio_index]);

                audio_index++;
            }
            else
            {
                __HAL_TIM_SET_COMPARE(&htim3,
                                      TIM_CHANNEL_1,
                                      0);

                audio_finished = 1;
            }
        }
        else if (dac_playing == 1)
        {
            if (dac_phase_step > 0)
            {
                uint8_t lut_index;

                dac_phase_acc += dac_phase_step;

                lut_index =
                    (dac_phase_acc >> 16) & 0x3F;

                HAL_DAC_SetValue(&hdac,
                                 DAC_CHANNEL_1,
                                 DAC_ALIGN_12B_R,
                                 sine_lut[lut_index]);
            }
        }
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

    uint8_t rx;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_TIM6_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_DAC_Init();

  /* USER CODE BEGIN 2 */

    print_menu();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
      if (HAL_UART_Receive(&huart2,
                           &rx,
                           1,
                           HAL_MAX_DELAY) == HAL_OK)
      {
          if (rx == '1')
          {
              uart_print("\r\nReproduciendo Audio PWM...\r\n");

              play_audio_pwm();

              uart_print("\r\nReproduccion terminada.\r\n");

              print_menu();
          }
          else if (rx == '2')
          {
              uart_print("\r\nReproduciendo Audio DAC...\r\n");

              play_audio_dac();

              uart_print("\r\nReproduccion terminada.\r\n");

              print_menu();
          }
          else if ((rx != '\r') && (rx != '\n'))
          {
              uart_print("\r\nOpcion invalida.\r\n");

              print_menu();
          }
      }

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

  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(
      PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_HSI;

  RCC_OscInitStruct.HSIState =
      RCC_HSI_ON;

  RCC_OscInitStruct.HSICalibrationValue =
      RCC_HSICALIBRATION_DEFAULT;

  RCC_OscInitStruct.PLL.PLLState =
      RCC_PLL_ON;

  RCC_OscInitStruct.PLL.PLLSource =
      RCC_PLLSOURCE_HSI;

  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
      Error_Handler();
  }

  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK |
      RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 |
      RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource =
      RCC_SYSCLKSOURCE_PLLCLK;

  RCC_ClkInitStruct.AHBCLKDivider =
      RCC_SYSCLK_DIV1;

  RCC_ClkInitStruct.APB1CLKDivider =
      RCC_HCLK_DIV2;

  RCC_ClkInitStruct.APB2CLKDivider =
      RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct,
                          FLASH_LATENCY_2) != HAL_OK)
  {
      Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc1.Instance = ADC1;

  hadc1.Init.ClockPrescaler =
      ADC_CLOCK_SYNC_PCLK_DIV4;

  hadc1.Init.Resolution =
      ADC_RESOLUTION_12B;

  hadc1.Init.ScanConvMode =
      DISABLE;

  hadc1.Init.ContinuousConvMode =
      DISABLE;

  hadc1.Init.DiscontinuousConvMode =
      DISABLE;

  hadc1.Init.ExternalTrigConvEdge =
      ADC_EXTERNALTRIGCONVEDGE_NONE;

  hadc1.Init.ExternalTrigConv =
      ADC_SOFTWARE_START;

  hadc1.Init.DataAlign =
      ADC_DATAALIGN_RIGHT;

  hadc1.Init.NbrOfConversion = 1;

  hadc1.Init.DMAContinuousRequests =
      DISABLE;

  hadc1.Init.EOCSelection =
      ADC_EOC_SINGLE_CONV;

  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
      Error_Handler();
  }

  sConfig.Channel =
      ADC_CHANNEL_0;

  sConfig.Rank = 1;

  sConfig.SamplingTime =
      ADC_SAMPLETIME_3CYCLES;

  if (HAL_ADC_ConfigChannel(&hadc1,
                            &sConfig) != HAL_OK)
  {
      Error_Handler();
  }
}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{
  DAC_ChannelConfTypeDef sConfig = {0};

  hdac.Instance = DAC;

  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
      Error_Handler();
  }

  sConfig.DAC_Trigger =
      DAC_TRIGGER_NONE;

  sConfig.DAC_OutputBuffer =
      DAC_OUTPUTBUFFER_ENABLE;

  if (HAL_DAC_ConfigChannel(&hdac,
                            &sConfig,
                            DAC_CHANNEL_1) != HAL_OK)
  {
      Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;

  htim3.Init.Prescaler = 9;

  htim3.Init.CounterMode =
      TIM_COUNTERMODE_UP;

  htim3.Init.Period = 255;

  htim3.Init.ClockDivision =
      TIM_CLOCKDIVISION_DIV1;

  htim3.Init.AutoReloadPreload =
      TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
      Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger =
      TIM_TRGO_RESET;

  sMasterConfig.MasterSlaveMode =
      TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(
          &htim3,
          &sMasterConfig) != HAL_OK)
  {
      Error_Handler();
  }

  sConfigOC.OCMode =
      TIM_OCMODE_PWM1;

  sConfigOC.Pulse = 128;

  sConfigOC.OCPolarity =
      TIM_OCPOLARITY_HIGH;

  sConfigOC.OCFastMode =
      TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(&htim3,
                                &sConfigOC,
                                TIM_CHANNEL_1) != HAL_OK)
  {
      Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim3);
}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim6.Instance = TIM6;

  htim6.Init.Prescaler = 83;

  htim6.Init.CounterMode =
      TIM_COUNTERMODE_UP;

  htim6.Init.Period = 124;

  htim6.Init.AutoReloadPreload =
      TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
      Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger =
      TIM_TRGO_UPDATE;

  sMasterConfig.MasterSlaveMode =
      TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(
          &htim6,
          &sMasterConfig) != HAL_OK)
  {
      Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;

  huart2.Init.BaudRate = 115200;

  huart2.Init.WordLength =
      UART_WORDLENGTH_8B;

  huart2.Init.StopBits =
      UART_STOPBITS_1;

  huart2.Init.Parity =
      UART_PARITY_NONE;

  huart2.Init.Mode =
      UART_MODE_TX_RX;

  huart2.Init.HwFlowCtl =
      UART_HWCONTROL_NONE;

  huart2.Init.OverSampling =
      UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart2) != HAL_OK)
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

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(LD2_GPIO_Port,
                    LD2_Pin,
                    GPIO_PIN_RESET);

  GPIO_InitStruct.Pin =
      B1_Pin;

  GPIO_InitStruct.Mode =
      GPIO_MODE_IT_FALLING;

  GPIO_InitStruct.Pull =
      GPIO_NOPULL;

  HAL_GPIO_Init(B1_GPIO_Port,
                &GPIO_InitStruct);

  GPIO_InitStruct.Pin =
      LD2_Pin;

  GPIO_InitStruct.Mode =
      GPIO_MODE_OUTPUT_PP;

  GPIO_InitStruct.Pull =
      GPIO_NOPULL;

  GPIO_InitStruct.Speed =
      GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(LD2_GPIO_Port,
                &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();

  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file,
                   uint32_t line)
{
}

#endif
