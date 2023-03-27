/**
  ******************************************************************************
  * @file    Multi/Examples/MotionControl/IHM01A1_ExampleFor1Motor/Src/main.c 
  * @author  IPC Rennes
  * @version V1.10.0
  * @date    March 16th, 2018
  * @brief   This example shows how to use 1 IHM01A1 expansion board
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>

/** @defgroup IHM01A1_Example_for_1_motor_device
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
 static volatile uint16_t gLastError;
 UART_HandleTypeDef huart2;

 //UART
#define TAM_BUF 200
uint8_t tx_buff[] = "mensaje de prueba\n\r";
uint8_t rx_buff[TAM_BUF];
uint8_t prueba[TAM_BUF] = "5.4,4,3";

char value[10] = "958";
char *eptr;
double hora, minuto, segundo, dia, mes, anio;
double latitud, longitud, altitud, UTCOffset, GCR, backtrack, ang_max;


//Motor
L6474_Init_t gL6474InitParams =
{
    160,                               /// Acceleration rate in step/s2. Range: (0..+inf).
    160,                               /// Deceleration rate in step/s2. Range: (0..+inf). 
    30,                              /// Maximum speed in step/s. Range: (30..10000].
    30,                               ///Minimum speed in step/s. Range: [30..10000).
    250,                               ///Torque regulation current in mA. (TVAL register) Range: 31.25mA to 4000mA.
    750,                               ///Overcurrent threshold (OCD_TH register). Range: 375mA to 6000mA.
    L6474_CONFIG_OC_SD_ENABLE,         ///Overcurrent shutwdown (OC_SD field of CONFIG register). 
    L6474_CONFIG_EN_TQREG_TVAL_USED,   /// Torque regulation method (EN_TQREG field of CONFIG register).
	L6474_STEP_SEL_1,//L6474_STEP_SEL_1_16,//L6474_STEP_SEL_1,               /// Step selection (STEP_SEL field of STEP_MODE register).
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


/* Private function prototypes -----------------------------------------------*/
static void MyFlagInterruptHandler(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

/* Private functions ---------------------------------------------------------*/

volatile uint8_t modo = 0;
volatile uint8_t hardstop = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

	if(GPIO_Pin == GPIO_PIN_2){
		hardstop = 1;
		BSP_MotorControl_HardStop(0);

	}
	else if(GPIO_Pin == GPIO_PIN_1){
		modo = 2;
	}
	else if (GPIO_Pin == BSP_MOTOR_CONTROL_BOARD_FLAG_PIN)
	{
		BSP_MotorControl_FlagInterruptHandler();
	}
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  int32_t pos;
  uint16_t mySpeed;

  /* STM32xx HAL library initialization */
  HAL_Init();
  
  /* Configure the system clock */
  SystemClock_Config();

  //Inicialización interrupciones
  MX_GPIO_Init();
  MX_USART2_UART_Init();
    
//----- Init of the Motor control library 
/* Set the L6474 library to use 1 device */
  BSP_MotorControl_SetNbDevices(BSP_MOTOR_CONTROL_BOARD_ID_L6474, 1);
  /* When BSP_MotorControl_Init is called with NULL pointer,                  */
  /* the L6474 registers and parameters are set with the predefined values from file   */
  /* l6474_target_config.h, otherwise the registers are set using the   */
  /* L6474_Init_t pointer structure                */
  /* The first call to BSP_MotorControl_Init initializes the first device     */
  /* whose Id is 0.                                                           */
  /* The nth call to BSP_MotorControl_Init initializes the nth device         */
  /* whose Id is n-1.                                                         */
  /* Uncomment the call to BSP_MotorControl_Init below to initialize the      */
  /* device with the structure gL6474InitParams declared in the the main.c file */
  /* and comment the subsequent call having the NULL pointer                   */
  BSP_MotorControl_Init(BSP_MOTOR_CONTROL_BOARD_ID_L6474, &gL6474InitParams);
  //BSP_MotorControl_Init(BSP_MOTOR_CONTROL_BOARD_ID_L6474, NULL);
  
  /* Attach the function MyFlagInterruptHandler (defined below) to the flag interrupt */
  BSP_MotorControl_AttachFlagInterrupt(MyFlagInterruptHandler);

  /* Attach the function Error_Handler (defined below) to the error Handler*/
  BSP_MotorControl_AttachErrorHandler(Error_Handler);
  
  //UART


  /* Get current position of device 0*/
//  pos = BSP_MotorControl_GetPosition(0);
//  printf("pos inicial = %ld", pos);
  /* Set the current position of device 0 to be the Home position */
//  BSP_MotorControl_SetHome(0, pos);
  //BSP_MotorControl_SelectStepMode(0,STEP_MODE_FULL);


/*  // Move device 0 in the FORWARD direction
  BSP_MotorControl_Move(0, FORWARD, 50);

  // Wait for the motor of device 0 ends moving
  BSP_MotorControl_WaitWhileActive(0);

  pos = BSP_MotorControl_GetPosition(0);
  printf("pos ida= %ld", pos);

  // Wait for 2 seconds
  HAL_Delay(2000);

  // Move device 0 in the BACKWARD direction
  BSP_MotorControl_Move(0, BACKWARD, 50);

  // Wait for the motor of device 0 ends moving
  BSP_MotorControl_WaitWhileActive(0);

  pos = BSP_MotorControl_GetPosition(0);
  printf("pos vuelta= %ld", pos);

  // Wait for 2 seconds
  HAL_Delay(2000);

  //microstepping
  BSP_MotorControl_SelectStepMode(0,L6474_STEP_SEL_1_16);


    // Move device 0 in the FORWARD direction
    BSP_MotorControl_Move(0, FORWARD, 800);

    // Wait for the motor of device 0 ends moving
    BSP_MotorControl_WaitWhileActive(0);

    pos = BSP_MotorControl_GetPosition(0);
    printf("pos ida= %ld", pos);

    // Wait for 2 seconds
    HAL_Delay(2000);

    // Move device 0 in the BACKWARD direction
    BSP_MotorControl_Move(0, BACKWARD, 800);

    // Wait for the motor of device 0 ends moving
    BSP_MotorControl_WaitWhileActive(0);

    pos = BSP_MotorControl_GetPosition(0);
    printf("pos vuelta= %ld", pos);

    // Wait for 2 seconds
    HAL_Delay(2000);
*/

/*  //Arranque, llevar seguidor a posición de final de carrera para calibración
  while (modo != 2){
	  //BSP_MotorControl_Run(0, BACKWARD);
	  BSP_MotorControl_Move(0, BACKWARD, 1);
	  BSP_MotorControl_WaitWhileActive(0);
  }
  pos = BSP_MotorControl_GetPosition(0) - 1600/3.0; //Posición a la que se encuentra el zenit respecto del final de carrera (pos - 60º)
  BSP_MotorControl_SetMark(0, pos); //Asignams el zenit como marca
  BSP_MotorControl_SetHome(0, pos); //Asignamos la posición del final de carrera como casa, la posición de referencia 0 ahora es este valor
  pos = BSP_MotorControl_GetPosition(0);

  //BSP_MotorControl_Move(0, FORWARD, 534);
  //BSP_MotorControl_WaitWhileActive(0);
  //pos = BSP_MotorControl_GetPosition(0);
  pos = 1600/3.0; //Posición a la que se encuentra el zenit respecto del final de carrera (60º)
  BSP_MotorControl_SetMark(0, pos); //Asignams el zenit como marca
*/

  while(1){
  int posicion = BSP_MotorControl_GetPosition(0);
  BSP_MotorControl_SetHome(0, posicion);
  BSP_MotorControl_GoTo(0, 1);
  BSP_MotorControl_WaitWhileActive(0);
  }
  /* Infinite loop */
  while(1)
  {



	  //UART
	  //HAL_UART_Transmit(&huart2, tx_buff,strlen((char*)tx_buff), 1000);
	  //HAL_Delay(1000);
	  /*HAL_UART_Receive_IT(&huart2, rx_buff, TAM_BUF);

	  	  int parametro = 1;
	  	  uint8_t delimitador[] = ",";

	  	  char *token = strtok(rx_buff, delimitador);
	  	  if(token != NULL){
	  		  while(token != NULL){
	  			  switch (parametro){
	  			  case 1:
	  				  hora = strtod(token, &eptr);
	  				  break;
	  			  case 2:
	  				  minuto = strtod(token, &eptr);
	  				  break;
	  			  case 3:
	  				  segundo = strtod(token, &eptr);
	  				  break;
	  			  case 4:
	  				  dia = strtod(token, &eptr);
	  				  break;
	  			  case 5:
	  				  mes = strtod(token, &eptr);
	  				  break;
	  			  case 6:
	  				  anio = strtod(token, &eptr);
	  				  break;
	  			  case 7:
	  				  latitud = strtod(token, &eptr);
	  				  break;
	  			  case 8:
	  				  longitud = strtod(token, &eptr);
	  				  break;
	  			  case 9:
	  				  altitud = strtod(token, &eptr);
	  				  break;
	  			  case 10:
	  				  UTCOffset = strtod(token, &eptr);
	  				  break;
	  			  case 11:
	  				  GCR = strtod(token, &eptr);
	  				  break;
	  			  case 12:
	  				  backtrack = strtod(token, &eptr);
	  				  break;
	  			  case 13:
	  				  ang_max = strtod(token, &eptr);
	  				  break;
	  			  default:
	  				  break;
	  			  }
	  			  parametro++;
	  			  token = strtok(NULL, delimitador);
	  		  }
	  	  }*/

/*	  BSP_MotorControl_GoHome(0);
	  BSP_MotorControl_WaitWhileActive(0);
	  pos = BSP_MotorControl_GetPosition(0);
	  HAL_Delay(2000);

	  BSP_MotorControl_GoMark(0);
	  BSP_MotorControl_WaitWhileActive(0);
	  pos = BSP_MotorControl_GetPosition(0);
	  HAL_Delay(2000);*/


	 /* if (hardstop == 1){
		  BSP_MotorControl_WaitWhileActive(0);
		  BSP_MotorControl_CmdDisable(0);
	  }
	  else{
		  if(modo == 2){
			modo = 0;
			BSP_MotorControl_Move(0, BACKWARD, 500);
			BSP_MotorControl_WaitWhileActive(0);
			pos = BSP_MotorControl_GetPosition(0);
		  }
	  }*/



/*	  if(modo == 1){
		modo = 0;
	  	BSP_MotorControl_Move(0, FORWARD, 50);
	  	BSP_MotorControl_WaitWhileActive(0);
	  	pos = BSP_MotorControl_GetPosition(0);
	  }
	  else if(modo == 2){
		modo = 0;
		BSP_MotorControl_Move(0, BACKWARD, 50);
		BSP_MotorControl_WaitWhileActive(0);
		pos = BSP_MotorControl_GetPosition(0);
	  }*/


	  /* Request device 0 to go position -6400 */
    //BSP_MotorControl_Move(0, FORWARD, 50);

    /* Wait for the motor of device 0 ends moving */
//    BSP_MotorControl_WaitWhileActive(0);

    /* Request device 0 to go position 6400 */
  //  BSP_MotorControl_Move(0, BACKWARD, 50);

    /* Wait for the motor of device 0 ends moving */
    //BSP_MotorControl_WaitWhileActive(0);
  }
}

/**
  * @brief  This function is the User handler for the flag interrupt
  * @param  None
  * @retval None
  */
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
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler(1);
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PB2 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

}

int _write(int file, char *ptr, int len){
	int DataIdx;

	for(DataIdx = 0; DataIdx < len; DataIdx++){
		ITM_SendChar(*ptr++);
	}
	return len;
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  error number of the error
  * @retval None
  */
void Error_Handler(uint16_t error)
{
  /* Backup error number */
  gLastError = error;
  
  /* Infinite loop */
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
