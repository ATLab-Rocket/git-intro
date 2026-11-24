/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : STM32F407 UART Bidirectional Communication
 *
 * Project         : ATlab_UART_F407
 * MCU             : STM32F407VET6
 * Peripheral      : USART1
 *
 * UART Settings:
 * Baud Rate       : 9600
 * Word Length     : 8 Bits
 * Parity          : None
 * Stop Bits       : 1
 *
 * TX Pin          : PA9
 * RX Pin          : PA10
 *
 * Communication:
 * STM32F407 sends "HELLO"
 * STM32L476 replies "ACK"
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

#define UART_TIMEOUT_MS 1000U
#define UART_DELAY_MS   1000U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* UART transmission data */
uint8_t uart_hello[5] = {
    'H', 'E', 'L', 'L', 'O'
};

/* UART reception buffer */
uint8_t uart_ack[3] = {0};

/* Communication counters */
volatile uint32_t uart_ack_count = 0;
volatile uint32_t uart_error_count = 0;

/* Detailed error counters */
volatile uint32_t uart_tx_error_count = 0;
volatile uint32_t uart_rx_error_count = 0;
volatile uint32_t uart_invalid_ack_count = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

static void MX_GPIO_Init(void);

static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */

int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration----------------------------------------------------*/

    /* Reset of all peripherals */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    /* USER CODE BEGIN 2 */

    /*
     * Allow the other STM32 board
     * some time to initialize.
     */
    HAL_Delay(500);

    /* USER CODE END 2 */

    /* Infinite loop */

    /* USER CODE BEGIN WHILE */

    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

        /*
         * Step 1:
         * Clear the reception buffer.
         */

        uart_ack[0] = 0;
        uart_ack[1] = 0;
        uart_ack[2] = 0;

        /*
         * Step 2:
         * Send "HELLO" to STM32L476.
         */

        HAL_StatusTypeDef tx_status;

        tx_status = HAL_UART_Transmit(
            &huart1,
            uart_hello,
            sizeof(uart_hello),
            UART_TIMEOUT_MS
        );

        if (tx_status == HAL_OK)
        {
            /*
             * Step 3:
             * Wait for "ACK" from STM32L476.
             */

            HAL_StatusTypeDef rx_status;

            rx_status = HAL_UART_Receive(
                &huart1,
                uart_ack,
                sizeof(uart_ack),
                UART_TIMEOUT_MS
            );

            if (rx_status == HAL_OK)
            {
                /*
                 * Step 4:
                 * Verify received ACK.
                 */

                if (
                    uart_ack[0] == 'A' &&
                    uart_ack[1] == 'C' &&
                    uart_ack[2] == 'K'
                )
                {
                    /*
                     * Communication successful.
                     */

                    uart_ack_count++;
                }
                else
                {
                    /*
                     * Invalid response received.
                     */

                    uart_invalid_ack_count++;
                    uart_error_count++;
                }
            }
            else
            {
                /*
                 * Reception failed or timed out.
                 */

                uart_rx_error_count++;
                uart_error_count++;
            }
        }
        else
        {
            /*
             * Transmission failed.
             */

            uart_tx_error_count++;
            uart_error_count++;
        }

        /*
         * Step 5:
         * Wait before the next transmission.
         */

        HAL_Delay(UART_DELAY_MS);

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

    /* Configure the main internal regulator output voltage */

    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(
        PWR_REGULATOR_VOLTAGE_SCALE1
    );

    /* Initialize RCC Oscillators */

    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI;

    RCC_OscInitStruct.HSIState = RCC_HSI_ON;

    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Initialize CPU, AHB and APB clocks */

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_HSI;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV1;

    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;

    if (
        HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_0
        ) != HAL_OK
    )
    {
        Error_Handler();
    }
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

    huart1.Init.BaudRate = 9600;

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
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */

static void MX_GPIO_Init(void)
{
    /* USER CODE BEGIN MX_GPIO_Init_1 */

    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* USER CODE BEGIN MX_GPIO_Init_2 */

    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief This function is executed in case of error occurrence.
 * @retval None
 */

void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */

    /*
     * Disable interrupts and stop execution
     * when a critical error occurs.
     */

    __disable_irq();

    while (1)
    {
    }

    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

/**
 * @brief Reports the name of the source file
 *        and the source line number.
 *
 * @param file: pointer to source file name
 * @param line: source line number
 * @retval None
 */

void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */

    /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */
