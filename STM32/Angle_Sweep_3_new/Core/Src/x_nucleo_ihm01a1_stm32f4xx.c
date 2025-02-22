/**
  ******************************************************************************
  * @file    x_nucleo_ihm01a1_stm32f4xx.c
  * @author  IPC Rennes
  * @version V1.7.0
  * @date    August 11th, 2016
  * @brief   BSP driver for x-nucleo-ihm01a1 Nucleo extension board 
  *  (based on L6474)
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */ 
  
/* Includes ------------------------------------------------------------------*/
#include "x_nucleo_ihm01a1_stm32f4xx.h"

/** @addtogroup BSP
  * @{
  */ 

/** @defgroup X_NUCLEO_IHM01A1_STM32F4XX  NUCLEO IHM01A1 X_NUCLEO_IHM01A1_STM32F4XX
  * @{
  */   
    
/* Private constants ---------------------------------------------------------*/    

/** @defgroup IHM01A1_Private_Constants IHM01A1 Private Constants
  * @{
  */   
    
/// Timer Prescaler
#define TIMER_PRESCALER (1024)

/// SPI Maximum Timeout values for flags waiting loops
#define SPIx_TIMEOUT_MAX                      ((uint32_t)0x1000)

/**
  * @}
  */ 

/* Private variables ---------------------------------------------------------*/

/** @defgroup IHM01A1_Board_Private_Variables IHM01A1 Board Private Variables
  * @{
  */       
/// SPI handler declaration
static SPI_HandleTypeDef SpiHandle;
/// Timer handler for PWM1
TIM_HandleTypeDef hTimPwm1;
/// imer handler for PWM2
TIM_HandleTypeDef hTimPwm2;
/// Timer handler for PWM3
TIM_HandleTypeDef hTimPwm3;
/**
  * @}
  */ 

/** @defgroup IHM01A1_Board_Private_Function_Prototypes IHM01A1 Board Private Function Prototypes
  * @{
  */   
   
void L6474_Board_Delay(uint32_t delay);         //Delay of the requested number of milliseconds
void L6474_Board_DisableIrq(void);              //Disable Irq
void L6474_Board_EnableIrq(void);               //Enable Irq
void L6474_Board_GpioInit(uint8_t deviceId);   //Initialise GPIOs used for L6474s
void L6474_Board_Pwm1SetFreq(uint16_t newFreq); //Set PWM1 frequency and start it
void L6474_Board_Pwm2SetFreq(uint16_t newFreq); //Set PWM2 frequency and start it  
void L6474_Board_Pwm3SetFreq(uint16_t newFreq); //Set PWM3 frequency and start it
void L6474_Board_PwmInit(uint8_t deviceId);    //Init the PWM of the specified device
void L6474_Board_PwmStop(uint8_t deviceId);    //Stop the PWM of the specified device
void L6474_Board_ReleaseReset(uint8_t deviceId); //Reset the L6474 reset pin 
void L6474_Board_Reset(uint8_t deviceId);       //Set the L6474 reset pin 
void L6474_Board_SetDirectionGpio(uint8_t deviceId, uint8_t gpioState); //Set direction GPIO
uint8_t L6474_Board_SpiInit(void);   //Initialise the SPI used for L6474s
uint8_t L6474_Board_SpiWriteBytes(uint8_t *pByteToTransmit, uint8_t *pReceivedByte, uint8_t nbDevices); //Write bytes to the L6474s via SPI


/**
  * @}
  */


/** @defgroup  IHM01A1_Board_Private_Functions IHM01A1 Board Private Functions
  * @{
  */   

/******************************************************//**
 * @brief This function provides an accurate delay in milliseconds
 * @param[in] delay  time length in milliseconds
 * @retval None
 **********************************************************/
void L6474_Board_Delay(uint32_t delay)
{
  HAL_Delay(delay);
}

/******************************************************//**
 * @brief This function disable the interruptions
  * @retval None
 **********************************************************/
void L6474_Board_DisableIrq(void)
{
  __disable_irq();
}

/******************************************************//**
 * @brief This function enable the interruptions
 * @retval None
 **********************************************************/
void L6474_Board_EnableIrq(void)
{
  __enable_irq();
}

/******************************************************//**
 * @brief  Initiliases the GPIOs used by the L6474s
 * @param[in] deviceId (from 0 to 2)
 * @retval None
  **********************************************************/
void L6474_Board_GpioInit(uint8_t deviceId)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  if (deviceId ==0)
  {
    /* GPIO Ports Clock Enable */
    __GPIOC_CLK_ENABLE();
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    
    /* Configure L6474 - Flag pin -------------------------------------------*/
    GPIO_InitStruct.Pin = BSP_MOTOR_CONTROL_BOARD_FLAG_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
    HAL_GPIO_Init(BSP_MOTOR_CONTROL_BOARD_FLAG_PORT, &GPIO_InitStruct);
    
   /* Set Priority of External Line Interrupt used for the Flag interrupt*/ 
    HAL_NVIC_SetPriority(EXTI_MCU_LINE_IRQn, 5, 0);
      
    /* Enable the External Line Interrupt used for the Flag interrupt*/
    HAL_NVIC_EnableIRQ(EXTI_MCU_LINE_IRQn);    

    /* Configure L6474 - CS pin ---------------------------------------------*/
    GPIO_InitStruct.Pin = BSP_MOTOR_CONTROL_BOARD_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
    HAL_GPIO_Init(BSP_MOTOR_CONTROL_BOARD_CS_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_CS_PORT, BSP_MOTOR_CONTROL_BOARD_CS_PIN, GPIO_PIN_SET); 
    
    /* Configure L6474 - STBY/RESET pin -------------------------------------*/
    GPIO_InitStruct.Pin = BSP_MOTOR_CONTROL_BOARD_RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
    HAL_GPIO_Init(BSP_MOTOR_CONTROL_BOARD_RESET_PORT, &GPIO_InitStruct);
    L6474_Board_Reset(0);  
  }
  
  switch (deviceId)
  {
    case 0:
      /* Configure L6474 - DIR pin for first device  -------------------------------*/
      GPIO_InitStruct.Pin = BSP_MOTOR_CONTROL_BOARD_DIR_1_PIN;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
      HAL_GPIO_Init(BSP_MOTOR_CONTROL_BOARD_DIR_1_PORT, &GPIO_InitStruct);
      break;
    
    case 1:
      /* Configure L6474 - DIR pin for second device   ----------------------------*/
      GPIO_InitStruct.Pin = BSP_MOTOR_CONTROL_BOARD_DIR_2_PIN;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
      HAL_GPIO_Init(BSP_MOTOR_CONTROL_BOARD_DIR_2_PORT, &GPIO_InitStruct);
      break;

    case 2:
      /* Configure L6474 - DIR pin for third device   ----------------------------*/
      GPIO_InitStruct.Pin = BSP_MOTOR_CONTROL_BOARD_DIR_3_PIN;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
      HAL_GPIO_Init(BSP_MOTOR_CONTROL_BOARD_DIR_3_PORT, &GPIO_InitStruct);    
        break;
  }
}

/******************************************************//**
 * @brief  Sets the frequency of PWM1 used by device 0
 * @param[in] newFreq in Hz
 * @retval None
 * @note The frequency is directly the current speed of the device
 **********************************************************/
void L6474_Board_Pwm1SetFreq(uint16_t newFreq)
{
  uint32_t sysFreq = HAL_RCC_GetSysClockFreq();
  uint32_t period = (sysFreq/ (TIMER_PRESCALER * BSP_MOTOR_CONTROL_BOARD_PWM1_FREQ_RESCALER * (uint32_t)newFreq)) - 1;
  
  __HAL_TIM_SetAutoreload(&hTimPwm1, period);
  __HAL_TIM_SetCompare(&hTimPwm1, BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM1, period >> 1);
  HAL_TIM_PWM_Start_IT(&hTimPwm1, BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM1);  
}

/******************************************************//**
 * @brief  Sets the frequency of PWM2 used by device 1
 * @param[in] newFreq in Hz
 * @retval None
 * @note The frequency is directly the current speed of the device
 **********************************************************/
void L6474_Board_Pwm2SetFreq(uint16_t newFreq)
{
  uint32_t sysFreq = HAL_RCC_GetSysClockFreq();
  uint32_t period = (sysFreq/ (TIMER_PRESCALER * BSP_MOTOR_CONTROL_BOARD_PWM2_FREQ_RESCALER  * (uint32_t)newFreq)) - 1;
  
  __HAL_TIM_SetAutoreload(&hTimPwm2, period);
  __HAL_TIM_SetCompare(&hTimPwm2, BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM2, period >> 1);
  HAL_TIM_PWM_Start_IT(&hTimPwm2, BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM2);
}
/******************************************************//**
 * @brief  Sets the frequency of PWM3 used by device 2
 * @param[in] newFreq in Hz
 * @retval None
 * @note The frequency is directly the current speed of the device
 **********************************************************/
void L6474_Board_Pwm3SetFreq(uint16_t newFreq)
{
  uint32_t sysFreq = HAL_RCC_GetSysClockFreq();
  /* Double the frequency as the SW is generated by SW */
  uint32_t period = (sysFreq/ (TIMER_PRESCALER * BSP_MOTOR_CONTROL_BOARD_PWM3_FREQ_RESCALER * (uint32_t)newFreq)) - 1;
  
  __HAL_TIM_SetAutoreload(&hTimPwm3, period);
  __HAL_TIM_SetCompare(&hTimPwm3, BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM3, period >> 1);
  HAL_TIM_PWM_Start_IT(&hTimPwm3, BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM3);  
}

/******************************************************//**
 * @brief  Initialises the PWM uses by the specified device
 * @param[in] deviceId (from 0 to 2)
 * @retval None
 * @note Device 0 uses PWM1 based on timer 1 
 * Device 1 uses PWM 2 based on timer 2
 * Device 2 uses PWM3 based timer 0
 **********************************************************/
void L6474_Board_PwmInit(uint8_t deviceId)
{
  TIM_OC_InitTypeDef sConfigOC;
  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_HandleTypeDef *pHTim;
  uint32_t  channel;

  switch (deviceId)
  {

  case 0:
  default:
      pHTim = &hTimPwm1;
      pHTim->Instance = BSP_MOTOR_CONTROL_BOARD_TIMER_PWM1;
      channel = BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM1;

      break;
    case  1:
      pHTim = &hTimPwm2;
      pHTim->Instance = BSP_MOTOR_CONTROL_BOARD_TIMER_PWM2;
      channel = BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM2;
      break;


    case 2:
      pHTim = &hTimPwm3;
      pHTim->Instance = BSP_MOTOR_CONTROL_BOARD_TIMER_PWM3;
      channel = BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM3;
      break;
  }
  pHTim->Init.Prescaler = TIMER_PRESCALER -1;
  
  pHTim->Init.Period = 0;
  pHTim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_PWM_Init(pHTim);
  
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(pHTim, &sConfigOC, channel);
  
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(pHTim, &sMasterConfig);
}

/******************************************************//**
 * @brief  Stops the PWM uses by the specified device
 * @param[in] deviceId (from 0 to 2)
 * @retval None
 **********************************************************/
void L6474_Board_PwmStop(uint8_t deviceId)
{
  switch (deviceId)
  {
    case 0:
       HAL_TIM_PWM_Stop(&hTimPwm1,BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM1);
    
      break;
    case  1:
      HAL_TIM_PWM_Stop(&hTimPwm2,BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM2);
      
      break;
    case 2:
       HAL_TIM_PWM_Stop(&hTimPwm3,BSP_MOTOR_CONTROL_BOARD_CHAN_TIMER_PWM3);
      
      break;
    default:
      break;//ignore error
  }
}

/******************************************************//**
 * @brief  Releases the L6474 reset (pin set to High) of all devices
 * @param[in] deviceId (from 0 to 2)
 * @retval None
 **********************************************************/
void L6474_Board_ReleaseReset(uint8_t deviceId)
{ 
  HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_RESET_PORT, BSP_MOTOR_CONTROL_BOARD_RESET_PIN, GPIO_PIN_SET); 
}

/******************************************************//**
 * @brief  Resets the L6474 (reset pin set to low) of all devices
 * @param[in] deviceId (from 0 to 2)
 * @retval None
 **********************************************************/
void L6474_Board_Reset(uint8_t deviceId)
{
  HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_RESET_PORT, BSP_MOTOR_CONTROL_BOARD_RESET_PIN, GPIO_PIN_RESET); 
}

/******************************************************//**
 * @brief  Set the GPIO used for the direction
 * @param[in] deviceId (from 0 to 2)
 * @param[in] gpioState state of the direction gpio (0 to reset, 1 to set)
 * @retval None
 **********************************************************/
void L6474_Board_SetDirectionGpio(uint8_t deviceId, uint8_t gpioState)
{
  switch (deviceId)
  {
    case 2:
      HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_DIR_3_PORT, BSP_MOTOR_CONTROL_BOARD_DIR_3_PIN, (GPIO_PinState)gpioState); 
      break;
    case 1:
      HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_DIR_2_PORT, BSP_MOTOR_CONTROL_BOARD_DIR_2_PIN, (GPIO_PinState)gpioState); 
      break;
    case 0:
      HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_DIR_1_PORT, BSP_MOTOR_CONTROL_BOARD_DIR_1_PIN, (GPIO_PinState)gpioState); 
      break;
    default:
      ;
  }
}

/******************************************************//**
 * @brief  Initialise the SPI used by L6474
 * @retval HAL_OK if SPI transaction is OK, HAL_KO else
 **********************************************************/
uint8_t L6474_Board_SpiInit(void)
{
  HAL_StatusTypeDef status;
  
  /* Initialises the SPI  --------------------------------------------------*/
  SpiHandle.Instance               = SPIx;
  SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32; 
  SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
  SpiHandle.Init.CLKPhase          = SPI_PHASE_2EDGE;    
  SpiHandle.Init.CLKPolarity       = SPI_POLARITY_HIGH;
  SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
  SpiHandle.Init.CRCPolynomial     = 7;
  SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
  SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  SpiHandle.Init.NSS               = SPI_NSS_SOFT;
  SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLED;
  
  SpiHandle.Init.Mode = SPI_MODE_MASTER;
  
  status = HAL_SPI_Init(&SpiHandle);
  
  return (uint8_t) status;
}
/******************************************************//**
 * @brief  Write and read SPI byte to the L6474
 * @param[in] pByteToTransmit pointer to the byte to transmit
 * @param[in] pReceivedByte pointer to the received byte
 * @param[in] nbDevices Number of device in the SPI chain
 * @retval HAL_OK if SPI transaction is OK, HAL_KO else 
 **********************************************************/
uint8_t L6474_Board_SpiWriteBytes(uint8_t *pByteToTransmit, uint8_t *pReceivedByte, uint8_t nbDevices)
{
  HAL_StatusTypeDef status;
  uint32_t i;
  HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_CS_PORT, BSP_MOTOR_CONTROL_BOARD_CS_PIN, GPIO_PIN_RESET); 
  for (i = 0; i < nbDevices; i++)
  {
    status = HAL_SPI_TransmitReceive(&SpiHandle, pByteToTransmit, pReceivedByte, 1, SPIx_TIMEOUT_MAX);
    if (status != HAL_OK)
    {
      break;
    }
    pByteToTransmit++;
    pReceivedByte++;
  }
  HAL_GPIO_WritePin(BSP_MOTOR_CONTROL_BOARD_CS_PORT, BSP_MOTOR_CONTROL_BOARD_CS_PIN, GPIO_PIN_SET); 
  
  return (uint8_t) status;  
}

/**
  * @}
  */

/**
  * @}
  */    

/**
  * @}
  */ 
    
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
