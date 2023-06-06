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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include <stdlib.h>
#include <string.h>
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
RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim10;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM10_Init(void);
/* USER CODE BEGIN PFP */
float angle_to_step(float angle);
float step_to_angle(float step);
void update_pos();
void motor_move(float angle);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

volatile int state = 0;
/*
 * 0: Pre-homing state
 * 1: Normal function
 * 2: Exception. FC1 interruption mode
 * 3: Exception. FC2 interruption mode
 * */
volatile int sweep = 0;
/*
 * 0: No sweep
 * 1: Sweep
 * */
volatile int32_t pos0 = 0; //step in which the motor is
volatile int32_t pos1 = 0; //step in which the motor is
volatile int32_t pos2 = 0; //step in which the motor is
volatile float angle = 0; //value of the angle the motor has to be at any time
volatile int step = 0;
volatile int counter = 0;
volatile int counter_calibration = 0;
/*
 * 0: No step
 * 1: Step
 * */

volatile int direction = 1;
/*
 * 1: Increment
 *-1: Decrement
 * */

volatile int flag_end_movement = 0;

/* 0: movement not finished
 * 1: movement finished
 * */

// Angle constants
float max_angle = 50; //max sweeping angle
float min_angle = -50; //min sweeping angle
float FC1_angle = -55.1; //security angle
float FC2_angle = 55.1; //calibration angle
float increment = 0.5;

// Timer constants

uint32_t sweep_time = 60; // s

//Initialization parameters

static volatile uint16_t gLastError;
L6474_Init_t gL6474InitParams =
{
    160,                               /// Acceleration rate in step/s2. Range: (0..+inf).
    160,                               /// Deceleration rate in step/s2. Range: (0..+inf).
    800,                              /// Maximum speed in step/s. Range: (30..10000].
    800,                               ///Minimum speed in step/s. Range: [30..10000).
    500,                               ///Torque regulation current in mA. (TVAL register) Range: 31.25mA to 4000mA.
    1000,                               ///Overcurrent threshold (OCD_TH register). Range: 375mA to 6000mA.
    L6474_CONFIG_OC_SD_ENABLE,         ///Overcurrent shutwdown (OC_SD field of CONFIG register).
    L6474_CONFIG_EN_TQREG_TVAL_USED,   /// Torque regulation method (EN_TQREG field of CONFIG register).
    L6474_STEP_SEL_1_16,               /// Step selection (STEP_SEL field of STEP_MODE register).
    L6474_SYNC_SEL_1_2,                /// Sync selection (SYNC_SEL field of STEP_MODE register).
    L6474_FAST_STEP_12us,              /// Fall time value (T_FAST field of T_FAST register). Range: 2us to 32us.
    L6474_TOFF_FAST_8us,               /// Maximum fast decay time (T_OFF field of T_FAST register). Range: 2us to 32us.
    3,                                 /// Minimum ON time in us (TON_MIN register). Range: 0.5us to 64us.
    21,                                /// Minimum OFF time in us (TOFF_MIN register). Range: 0.5us to 64us.
    L6474_CONFIG_TOFF_044us,           /// Target Swicthing Period (field TOFF of CONFIG register).
    L6474_CONFIG_SR_320V_us,           /// Slew rate (POW_SR field of CONFIG register).
    L6474_CONFIG_INT_16MHZ,            /// Clock setting (OSC_CLK_SEL field of CONFIG register).
    (L6474_ALARM_EN_OVERCURRENT      |
     L6474_ALARM_EN_THERMAL_SHUTDOWN |
     L6474_ALARM_EN_THERMAL_WARNING  |
     L6474_ALARM_EN_UNDERVOLTAGE     |
     L6474_ALARM_EN_SW_TURN_ON       |
     L6474_ALARM_EN_WRONG_NPERF_CMD)    /// Alarm (ALARM_EN register).
};
//Shield functions
void ErrorHandler_Shield(uint16_t error);
void MyFlagInterruptHandler(void)
{
  /* Get the value of the status register via the L6474 command GET_STATUS */
  uint16_t statusRegister = BSP_MotorControl_CmdGetStatus(0);

  /* Check HIZ flag: if set, power brigdes are disabled */
  if ((statusRegister & L6474_STATUS_HIZ) == L6474_STATUS_HIZ)
  {
    // HIZ state
    // Action to be customized
  }

  /* Check direction bit */
  if ((statusRegister & L6474_STATUS_DIR) == L6474_STATUS_DIR)
  {
    // Forward direction is set
    // Action to be customized
  }
  else
  {
    // Backward direction is set
    // Action to be customized
  }

  /* Check NOTPERF_CMD flag: if set, the command received by SPI can't be performed */
  /* This often occures when a command is sent to the L6474 */
  /* while it is in HIZ state */
  if ((statusRegister & L6474_STATUS_NOTPERF_CMD) == L6474_STATUS_NOTPERF_CMD)
  {
      // Command received by SPI can't be performed
     // Action to be customized
  }

  /* Check WRONG_CMD flag: if set, the command does not exist */
  if ((statusRegister & L6474_STATUS_WRONG_CMD) == L6474_STATUS_WRONG_CMD)
  {
     //command received by SPI does not exist
     // Action to be customized
  }

  /* Check UVLO flag: if not set, there is an undervoltage lock-out */
  if ((statusRegister & L6474_STATUS_UVLO) == 0)
  {
     //undervoltage lock-out
     // Action to be customized
  }

  /* Check TH_WRN flag: if not set, the thermal warning threshold is reached */
  if ((statusRegister & L6474_STATUS_TH_WRN) == 0)
  {
    //thermal warning threshold is reached
    // Action to be customized
  }

  /* Check TH_SHD flag: if not set, the thermal shut down threshold is reached */
  if ((statusRegister & L6474_STATUS_TH_SD) == 0)
  {
    //thermal shut down threshold is reached
    // Action to be customized
  }

  /* Check OCD  flag: if not set, there is an overcurrent detection */
  if ((statusRegister & L6474_STATUS_OCD) == 0)
  {
    //overcurrent detection
    // Action to be customized
  }

}

//Interruptions Callback
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

	if(GPIO_Pin == GPIO_PIN_2){ //Left FC
		//Motor HardStop
		BSP_MotorControl_HardStop(0);
		BSP_MotorControl_HardStop(1);
		BSP_MotorControl_HardStop(2);
		state = 2;

	}
	else if(GPIO_Pin == GPIO_PIN_1){ //Right FC
		//Motor HardStop
		BSP_MotorControl_HardStop(0);
		BSP_MotorControl_HardStop(1);
		BSP_MotorControl_HardStop(2);
		state = 3;

	}
	else if (GPIO_Pin == BSP_MOTOR_CONTROL_BOARD_FLAG_PIN) //Shield interrupt handler
	{
		BSP_MotorControl_FlagInterruptHandler();
	}
}

//Sweeper timer
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){

	if (htim == &htim10){

	  //HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_12);
	  counter++;

	  if (counter >= sweep_time){
		  sweep = 1; //start sweeping
		  counter = 0;
	  }
	}



}


float angle_to_step(float angle){
	float conversion = angle * 164.8;
	return conversion;
}

float step_to_angle(float step){
	float conversion = angle / 164.8;
	return conversion;
}

void update_pos(){
	pos0 = BSP_MotorControl_GetPosition(0);
	pos1 = BSP_MotorControl_GetPosition(1);
	pos2 = BSP_MotorControl_GetPosition(2);
    angle = pos0 / 164.8;
}

void motor_move(float angle_to_go){
	update_pos();

	if (angle_to_go > angle){

		BSP_MotorControl_Move(0, FORWARD, 1);
		BSP_MotorControl_Move(1, FORWARD, 1);
		BSP_MotorControl_Move(2, FORWARD, 1);
		BSP_MotorControl_WaitWhileActive(0);
		BSP_MotorControl_WaitWhileActive(1);
		BSP_MotorControl_WaitWhileActive(2);
		update_pos();
	}

	else if (angle_to_go < angle){

		BSP_MotorControl_Move(0, BACKWARD, 1);
		BSP_MotorControl_Move(1, BACKWARD, 1);
		BSP_MotorControl_Move(2, BACKWARD, 1);
		BSP_MotorControl_WaitWhileActive(0);
		BSP_MotorControl_WaitWhileActive(1);
		BSP_MotorControl_WaitWhileActive(2);
		update_pos();
	}
	else {
		BSP_MotorControl_HardStop(0);
		flag_end_movement = 1;
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, RESET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, RESET);

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

  /* Initialise all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_TIM10_Init();
  /* USER CODE BEGIN 2 */

  //Driver initialisation
  BSP_MotorControl_SetNbDevices(BSP_MOTOR_CONTROL_BOARD_ID_L6474, 3);

  /* Initialisation of first device */
  BSP_MotorControl_Init(BSP_MOTOR_CONTROL_BOARD_ID_L6474, &gL6474InitParams);
  /* Initialisation of second device */
  BSP_MotorControl_Init(BSP_MOTOR_CONTROL_BOARD_ID_L6474, &gL6474InitParams);
  /* Initialisation of third device */
  BSP_MotorControl_Init(BSP_MOTOR_CONTROL_BOARD_ID_L6474, &gL6474InitParams);

  BSP_MotorControl_AttachFlagInterrupt(MyFlagInterruptHandler);
  BSP_MotorControl_AttachErrorHandler(ErrorHandler_Shield);
  state = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  if (state == 0){ //First calibration
		  BSP_MotorControl_SetMaxSpeed(0, 800);
		  BSP_MotorControl_SetMinSpeed(0, 800);
		  BSP_MotorControl_Move(0, FORWARD, 1);//Forward == Towards calibration FC
		  BSP_MotorControl_Move(1, FORWARD, 1);
		  BSP_MotorControl_Move(2, FORWARD, 1);
		  BSP_MotorControl_WaitWhileActive(0);
		  BSP_MotorControl_WaitWhileActive(1);
		  BSP_MotorControl_WaitWhileActive(2);
		  update_pos();
	  }
	  else if (state == 1){ //Normal behaviour
		  BSP_MotorControl_SetMaxSpeed(0, 600);
		  BSP_MotorControl_SetMinSpeed(0, 600);
		  if (sweep == 1){

			  if (direction == 1){

				  motor_move(min_angle);
				  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, SET);
				  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, RESET);

				  if (flag_end_movement == 1){
					  // movement finished -> reset and change direction
					  flag_end_movement = 0;
					  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, RESET);
					  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, RESET);
					  direction = -1;
					  sweep = 0;
					  counter_calibration++;
				  }
			  }

			  else if (direction == -1){

				  motor_move(max_angle);
				  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, RESET);
				  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, SET);

				  if (flag_end_movement == 1){
					  // movement finished -> reset and change direction
					  flag_end_movement = 0;
					  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, RESET);
					  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, RESET);
					  direction = 1;
					  sweep = 0;
					  counter_calibration++;
				  }
			  }
		  }
		  if (counter_calibration > 50){

			  counter_calibration = 0;
			  counter = 0;

			  HAL_TIM_Base_Stop_IT(&htim10);
			  BSP_MotorControl_GoHome(0);
			  BSP_MotorControl_GoHome(1);
			  BSP_MotorControl_GoHome(2);
			  BSP_MotorControl_WaitWhileActive(0);
			  BSP_MotorControl_WaitWhileActive(1);
			  BSP_MotorControl_WaitWhileActive(2);
			  update_pos();

			  state = 0;
		  }
	  }

	  else if (state == 2){ //calibration FC

		  // Start timer
		  HAL_TIM_Base_Start_IT(&htim10);

		  //Set angle and step parameters
		  pos0 = BSP_MotorControl_GetPosition(0);
		  pos1 = BSP_MotorControl_GetPosition(1);
		  pos2 = BSP_MotorControl_GetPosition(2);
		  BSP_MotorControl_SetHome(0, pos0 - angle_to_step(FC2_angle));
		  BSP_MotorControl_SetHome(1, pos1 - angle_to_step(FC2_angle));
		  BSP_MotorControl_SetHome(2, pos2 - angle_to_step(FC2_angle));
		  update_pos();

		  //Move motors to starting position
		  while(angle > max_angle){
			  motor_move(max_angle);
		  }

		  // Go to state 1
		  state = 1;
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, RESET);
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, RESET);
	  }

	  else if (state == 3){ //secutiry FC
		  angle = FC1_angle;
		  HAL_TIM_Base_Stop_IT(&htim10);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief TIM10 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM10_Init(void)
{

  /* USER CODE BEGIN TIM10_Init 0 */

  /* USER CODE END TIM10_Init 0 */

  /* USER CODE BEGIN TIM10_Init 1 */

  /* USER CODE END TIM10_Init 1 */
  htim10.Instance = TIM10;
  htim10.Init.Prescaler = 8400 - 1;
  htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim10.Init.Period = 10000 - 1;
  htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM10_Init 2 */

  /* USER CODE END TIM10_Init 2 */

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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC4 PC5 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PB1 PB2 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int _write(int file, char *ptr, int len){
	int DataIdx;

	for(DataIdx = 0; DataIdx < len; DataIdx++){
		ITM_SendChar(*ptr++);
	}
	return len;
}

void ErrorHandler_Shield(uint16_t error)
{
  /* Backup error number */
  gLastError = error;

  /* Infinite loop */
  while(1)
  {
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
