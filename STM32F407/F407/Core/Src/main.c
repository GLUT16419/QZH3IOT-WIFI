/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - STM32F407通过ESP8266连接阿里云IoT
  ******************************************************************************
  */
/* USER CODE END Header */
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mqtt_client.h"
#include "esp8266.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEVICE_NAME "STM32F407"
#define MQTT_PUB_TOPIC "/YOUR_PRODUCT_KEY/YOUR_DEVICE_NAME/user/update"
#define SEND_INTERVAL_MS 5000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

osThreadId_t mqttTaskHandle;
osThreadId_t sensorTaskHandle;
osThreadId_t espTaskHandle;
osMutexId_t uartMutexHandle;
const osMutexAttr_t uartMutexAttributes = {
  "UartMutex",
  osMutexPrioInherit,
  NULL,
  0U
};

/* USER CODE BEGIN PV */
typedef enum {
  STATE_INIT = 0,
  STATE_WIFI_CONNECT,
  STATE_TCP_CONNECT,
  STATE_MQTT_CONNECT,
  STATE_RUNNING
} State_t;

volatile State_t current_state = STATE_INIT;
char esp_rx_buffer[128];
float last_temperature = 25.0f;
float last_humidity = 50.0f;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART3_UART_Init(void);
void StartMqttTask(void *argument);
void StartSensorTask(void *argument);
void StartEspTask(void *argument);

/* USER CODE BEGIN PFP */
void generate_random_sensor_data(float *temp, float *humidity);
void send_sensor_data(float temp, float humidity);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void uart1_print(const char *str);
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();

  uart1_print("\r\n========== STM32F407 ESP8266 MQTT ==========\r\n");
  {
    char buf[64];
    snprintf(buf, sizeof(buf), "System Clock: %lu Hz\r\n", SystemCoreClock);
    uart1_print(buf);
  }
  uart1_print("USART1: Debug Port (115200 baud)\r\n");
  uart1_print("USART3: ESP8266 Communication\r\n");
  uart1_print("==========================================\r\n");

  osKernelInitialize();

  uartMutexHandle = osMutexNew(&uartMutexAttributes);

  osThreadNew(StartEspTask, NULL, &(const osThreadAttr_t){.name="EspTask", .stack_size=512, .priority=osPriorityHigh});
  osThreadNew(StartMqttTask, NULL, &(const osThreadAttr_t){.name="MqttTask", .stack_size=512, .priority=osPriorityNormal});
  osThreadNew(StartSensorTask, NULL, &(const osThreadAttr_t){.name="SensorTask", .stack_size=512, .priority=osPriorityNormal});

  osKernelStart();

  while (1) {}
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) Error_Handler();
}

/**
  * @brief USART3 Initialization Function
  */
static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK) Error_Handler();
}

/**
  * @brief USART1 Initialization Function - 调试串口
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

/**
  * @brief GPIO Initialization Function - ESP8266控制引脚
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(EN_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RST_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
void uart1_print(const char *str)
{
  HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
}

void generate_random_sensor_data(float *temp, float *humidity)
{
  *temp = 20.0f + (float)(rand() % 300) / 10.0f;
  *humidity = 30.0f + (float)(rand() % 600) / 10.0f;
}

void send_sensor_data(float temp, float humidity)
{
  if (current_state != STATE_RUNNING) return;

  char payload[100];
  snprintf(payload, sizeof(payload),
           "{\"temperature\":%.1f,\"humidity\":%.1f,\"sourceDevice\":\"STM32F407\"}",
           temp, humidity);

  if (!mqtt_publish(MQTT_PUB_TOPIC, payload)) {
    uart1_print("[SEND] Publish failed, closing connection...\r\n");
    esp8266_send_command("AT+CIPCLOSE\r\n", "OK", 1000);
    current_state = STATE_TCP_CONNECT;
  }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartEspTask */
/**
  * @brief ESP8266初始化任务
  */
/* USER CODE END Header_StartEspTask */
void StartEspTask(void *argument)
{
  uart1_print("[ESP8266] Waiting 3s before initialization...\r\n");
  HAL_Delay(3000);

  HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_SET);
  HAL_Delay(100);

  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);
  HAL_Delay(3000);

  if (esp8266_init(&huart3, uartMutexHandle)) {
    uart1_print("[ESP8266] Initialization completed!\r\n");
  }

  uart1_print("[ESP8266] State machine starting...\r\n");

  while (1) {
    switch (current_state) {
      case STATE_INIT:
        uart1_print("\n[STATE] INIT - Testing ESP8266...\r\n");
        if (esp8266_send_command("AT\r\n", "OK", 2000) == ESP8266_OK) {
          uart1_print("[OK] ESP8266 responding, moving to WiFi connect\r\n");
          current_state = STATE_WIFI_CONNECT;
        } else {
          uart1_print("[FAIL] ESP8266 not responding, retrying...\r\n");
          osDelay(2000);
        }
        break;

      case STATE_WIFI_CONNECT:
        uart1_print("\n[STATE] WIFI_CONNECT...\r\n");
        if (esp8266_connect_wifi(WIFI_SSID, WIFI_PASSWORD) == ESP8266_OK) {
          uart1_print("[OK] WiFi Connected!\r\n");
          osDelay(1000);
          uart1_print("[WIFI] Preparing for TCP...\r\n");
          esp8266_send_command("AT+CIPCLOSE\r\n", "OK", 2000);
          osDelay(500);
          uart1_print("[WIFI] Setting single connection mode...\r\n");
          if (esp8266_send_command("AT+CIPMUX=0\r\n", "OK", 2000) == ESP8266_OK) {
            uart1_print("[OK] Ready for TCP!\r\n");
            current_state = STATE_TCP_CONNECT;
          }
        } else {
          uart1_print("[FAIL] WiFi connection failed, retrying...\r\n");
          osDelay(3000);
        }
        break;

      case STATE_TCP_CONNECT:
        uart1_print("\n[STATE] TCP_CONNECT...\r\n");
        if (mqtt_connect() == ESP8266_OK) {
          uart1_print("[OK] TCP & MQTT Connected!\r\n");
          current_state = STATE_RUNNING;
        } else {
          uart1_print("[FAIL] TCP/MQTT failed, retrying TCP...\r\n");
          osDelay(2000);
        }
        break;

      case STATE_MQTT_CONNECT:
        current_state = STATE_TCP_CONNECT;
        break;

      case STATE_RUNNING:
        osDelay(1000);
        break;

      default:
        current_state = STATE_INIT;
        break;
    }
    osDelay(100);
  }
}

/* USER CODE BEGIN Header_StartMqttTask */
/**
  * @brief MQTT连接任务（已合并到EspTask）
  */
/* USER CODE END Header_StartMqttTask */
void StartMqttTask(void *argument)
{
  osDelay(1000);
  while (1) {
    osDelay(10000);
  }
}

/* USER CODE BEGIN Header_StartSensorTask */
/**
  * @brief 传感器数据采集和发送任务
  */
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
  while (1) {
    if (current_state == STATE_RUNNING) {
      generate_random_sensor_data(&last_temperature, &last_humidity);
      send_sensor_data(last_temperature, last_humidity);
    }
    osDelay(SEND_INTERVAL_MS);
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM12) {
    HAL_IncTick();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{}
#endif
