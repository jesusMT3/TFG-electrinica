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
#include "angulo.h"
#include "RTC.h"
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

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

//Variables comunicación UART
#define TAM_BUF 200
uint8_t tx_buff[TAM_BUF]; //Buffer de transmisión de datos
uint8_t rx_buff[TAM_BUF]; //Buffer de recibir datos
char *eptr; //Puntero para separación cadena

//Variables para guardado en memoria RAM
#define BKPSRAM_BASE 0x20017000 //Dirección del primer byte donde se va a almacenar la información
#define OFFSET 8                //Tamaño de un double (8 bytes)

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//Variables motor
static volatile uint16_t gLastError;

//Variables efemérides
struct constantes c;                         //constantes utilizadas en los cálculos de las efemérides
struct tiempo t = {20,07,2022,20,31,50};      //estructura que contiene la fecha y hora
struct localizacion local = {40.45,-3.73,0}; //valores de la ubicación del seguidor
struct ephemeris efemerides;                 //ángulos más importantes del cálculo de las efemérides
struct posSol pos;                           //coordenadas cartesianas de la posición del Sol
struct posSol pos_final;                     //coordenadas cartesianas de la posición del Sol teniendo en cuenta seguidor desviado
float wid;                                   //�?ngulo óptimo de inclinación para módulos monofaciales
float prev_wid;                              //�?ngulo wid cálculo anterior
volatile float widc = 0;                     //�?ngulo óptimo de inclinación para módulos bifaciales


//Variables RTC
volatile RTC_TimeTypeDef sTime; //Inicialización hora
volatile RTC_DateTypeDef sDate; //Inicialización fecha

//Valores inicialización motores
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
	L6474_STEP_SEL_1,//L6474_STEP_SEL_1_16,               /// Step selection (STEP_SEL field of STEP_MODE register).
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

//Variables estados funcionamiento
volatile uint8_t inicio = 0;    //Variable usada al arrancar el programa para calibrar el motor
volatile uint8_t modo = 0;      //modo = 0: llevar el seguidor a la posición horizontal, modo = 1: seguidor moviéndose según efemérides
volatile uint8_t izquierda = 0; //final de carrera izquierdo pulsado
volatile uint8_t derecha = 0;   //final de carrera derecho pulsado

volatile uint8_t usart_tx = 0;  //Botón de transmisión de datos pulsado
volatile uint8_t usart_rx = 0;  //Botón de recibir datos pulsado

volatile float posicion;        //variable con la posición del motor
volatile float ang_eq;          //Conversión del ángulo de grados a pasos

volatile int amanecer = 0;      //Variable para calibrado en el amanecer
volatile int hardstop = 0;      //Variable para parada seguidor si se pulsan dos veces el FC de la izquierda

//Funciones
//Funciones control Shield
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

//Función llamada UART
void parada_usart(){

	//Parada motor
	 BSP_MotorControl_WaitWhileActive(0);
	 BSP_MotorControl_CmdDisable(0);

	 //Código transmisión datos
	while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4)){
	  usart_tx = 1;
	  //Actualización fecha y hora
	  t = get_time(hrtc);
	  //Vaciado buffer
	  tx_buff[0] = '\0';
	  //Declaración variables
	  uint8_t s_hora[10], s_minuto[10], s_segundo[10], s_dia[10], s_mes[10], s_anio[10];
	  uint8_t s_latitud[10], s_longitud[10], s_altitud[10], s_UTCOffset[10], s_GCR[10], s_backtrack[10], s_ang_max[10];
	  //Conversión de double a string y añadido de coma y fin de palabra
	  sprintf(s_hora, "%f", t.hora);
	  s_hora[8] = ',';
	  s_hora[9] = '\0';
	  sprintf(s_minuto, "%f", t.minuto);
	  s_minuto[8] = ',';
	  s_minuto[9] = '\0';
	  sprintf(s_segundo, "%f", t.segundo);
	  s_segundo[8] = ',';
	  s_segundo[9] = '\0';
	  sprintf(s_dia, "%f", t.dia);
	  s_dia[8] = ',';
	  s_dia[9] = '\0';
	  sprintf(s_mes, "%f", t.mes);
	  s_mes[8] = ',';
	  s_mes[9] = '\0';
	  sprintf(s_anio, "%f", t.anio);
	  s_anio[8] = ',';
	  s_anio[9] = '\0';
	  sprintf(s_latitud, "%f", local.latitud);
	  s_latitud[8] = ',';
	  s_latitud[9] = '\0';
	  sprintf(s_longitud, "%f", local.longitud);
	  s_longitud[8] = ',';
	  s_longitud[9] = '\0';
	  sprintf(s_altitud, "%f", local.altitud);
	  s_altitud[8] = ',';
	  s_altitud[9] = '\0';
	  sprintf(s_UTCOffset, "%f", c.UTCOffset);
	  s_UTCOffset[8] = ',';
	  s_UTCOffset[9] = '\0';
	  sprintf(s_GCR, "%f", c.GCR);
	  s_GCR[8] = ',';
	  s_GCR[9] = '\0';
	  sprintf(s_backtrack, "%f", c.backtrack);
	  s_backtrack[8] = ',';
	  s_backtrack[9] = '\0';
	  sprintf(s_ang_max, "%f", c.angulo_max);
	  s_ang_max[8] = ',';
	  s_ang_max[9] = '\0';

	  //Formación de la cadena a enviar uniendo todas las cadenas de los datos
	  strcat(tx_buff, s_hora);
	  strcat(tx_buff, s_minuto);
	  strcat(tx_buff, s_segundo);
	  strcat(tx_buff, s_dia);
	  strcat(tx_buff, s_mes);
	  strcat(tx_buff, s_anio);
	  strcat(tx_buff, s_latitud);
	  strcat(tx_buff, s_longitud);
	  strcat(tx_buff, s_altitud);
	  strcat(tx_buff, s_UTCOffset);
	  strcat(tx_buff, s_GCR);
	  strcat(tx_buff, s_backtrack);
	  strcat(tx_buff, s_ang_max);

	  //Para una buena transmisión la cadena debe finalizar con \n\r\0
	  tx_buff[117]='\n';
	  tx_buff[118]='\r';
	  tx_buff[119]='\0';

	  //Transmisión del buffer por UART
	  HAL_UART_Transmit_IT(&huart2, tx_buff, strlen((char*)tx_buff));
	}
	usart_tx = 0;

	//Código recepción de datos
	while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5)){
	  usart_rx = 1;
	  //Doble lectura de los datos porque la primera cadena a veces contiene basura
	  HAL_UART_Receive_IT(&huart2, rx_buff, TAM_BUF);
	  HAL_UART_Receive_IT(&huart2, rx_buff, TAM_BUF);

	  int parametro = 1;
	  uint8_t delimitador[] = ",";

	  //Separación de la cadena y almacenamiento de cada valor en su variable
	  char *token = strtok(rx_buff, delimitador);
	  if(token != NULL){
		  while(token != NULL){
			  switch (parametro){
			  case 1:
				  t.hora = strtod(token, &eptr); //Conversión a double de la string
				  break;
			  case 2:
				  t.minuto = strtod(token, &eptr);
				  break;
			  case 3:
				  t.segundo = strtod(token, &eptr);
				  break;
			  case 4:
				  t.dia = strtod(token, &eptr);
				  break;
			  case 5:
				  t.mes = strtod(token, &eptr);
				  break;
			  case 6:
				  t.anio = strtod(token, &eptr);
				  break;
			  case 7:
				  local.latitud = strtod(token, &eptr);
				  break;
			  case 8:
				  local.longitud = strtod(token, &eptr);
				  break;
			  case 9:
				  local.altitud = strtod(token, &eptr);
				  break;
			  case 10:
				  c.UTCOffset = strtod(token, &eptr);
				  break;
			  case 11:
				  c.GCR = strtod(token, &eptr);
				  break;
			  case 12:
				  c.backtrack = strtod(token, &eptr);
				  break;
			  case 13:
				  c.angulo_max = strtod(token, &eptr);
				  break;
			  default:
				  break;
			  }
			  parametro++;
			  token = strtok(NULL, delimitador);
		  }
	  }
	  if(parametro >= 13){
		  set_time(hrtc, t); //Una vez recibidos todos los datos, actualización del RTC
	  }
	}
	usart_rx = 0;
}

//Callback de las interrupciones
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

	if(GPIO_Pin == GPIO_PIN_2){ //Final de carrera de la izquierda
		BSP_MotorControl_HardStop(0); //Parada brusca motor
		modo = 0;                     //Indicación modo de ir a posición seguridad
		izquierda = 1;                //Final de carrera pulsado izquierdo
	}
	else if(GPIO_Pin == GPIO_PIN_1){ //Final de carrera derecho
		inicio = 1;                   //Calibración motor inicial finalizada
		BSP_MotorControl_HardStop(0); //Parada brusca motor
		modo = 0;                     //Indicación modo de ir a posición de seguridad
		derecha = 1;                  //Final de carrera pulsado derecho
	}
	else if (GPIO_Pin == BSP_MOTOR_CONTROL_BOARD_FLAG_PIN) //Interrupción si la Shield detecta algo mal
	{
		BSP_MotorControl_FlagInterruptHandler();
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

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  //Inicialización driver
  BSP_MotorControl_SetNbDevices(BSP_MOTOR_CONTROL_BOARD_ID_L6474, 1);         //Se establece número de motores a controlar
  BSP_MotorControl_Init(BSP_MOTOR_CONTROL_BOARD_ID_L6474, &gL6474InitParams); //Parámetros iniciales del motor
  BSP_MotorControl_AttachFlagInterrupt(MyFlagInterruptHandler);               //Asociar controlador de interrupciones
  BSP_MotorControl_AttachErrorHandler(ErrorHandler_Shield);                   //Asociar función control errores

  //Inicialización programa la primera vez que se carga el programa
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != 0xBEBE){
	  //Habilitar acceso a la escritura de la memoria
	  HAL_PWR_EnableBkUpAccess();

	  //Inicialización constantes efemérides
	  c = constantes_defecto();

	  //Almacenado datos en la memoria RAM para evitar pérdida con RESET
	  *(__IO double *) (BKPSRAM_BASE) = local.latitud;
	  *(__IO double *) (BKPSRAM_BASE + 8) = local.longitud;
	  *(__IO double *) (BKPSRAM_BASE + 2*8) = local.altitud;
	  *(__IO double *) (BKPSRAM_BASE + 3*8) = c.UTCOffset;
	  *(__IO double *) (BKPSRAM_BASE + 4*8) = c.GCR;
	  *(__IO double *) (BKPSRAM_BASE + 5*8) = c.backtrack;
	  *(__IO double *) (BKPSRAM_BASE + 6*8) = c.angulo_max;
	  *(__IO double *) (BKPSRAM_BASE + 7*8) = c.temperatura;
	  *(__IO double *) (BKPSRAM_BASE + 8*8) = c.presion;

	  //Inicialización del tiempo en el RTC
	  set_time(hrtc, t);

	  //Dato con el que comprobamos si es la primera carga del programa o hay un RESET modificando su valor
	  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0xBEBE);

	  //Deshabilitar acceso a la escritura de la memoria
	  HAL_PWR_DisableBkUpAccess();

	  //Calibración motor: mover motor hasta activación final de carrera derecho
	    while (inicio == 0){
	  	  BSP_MotorControl_Move(0, BACKWARD, 1);
	  	  BSP_MotorControl_WaitWhileActive(0);
	    }

  }else{
	  modo = 1; //en el caso de que suceda un RESET, inicializar el programa en funcionamiento normal
  }

  //Lectura de los valores de la RAM para iniciar las variables
  local.latitud = (*(__IO double *) (BKPSRAM_BASE));
  local.longitud = (*(__IO double *) (BKPSRAM_BASE + 8));
  local.altitud = (*(__IO double *) (BKPSRAM_BASE + 2*8));
  c.UTCOffset = (*(__IO double *) (BKPSRAM_BASE + 3*8));
  c.GCR = (*(__IO double *) (BKPSRAM_BASE + 4*8));
  c.backtrack = (*(__IO double *) (BKPSRAM_BASE + 5*8));
  c.angulo_max = (*(__IO double *) (BKPSRAM_BASE + 6*8));
  c.temperatura = (*(__IO double *) (BKPSRAM_BASE + 7*8));
  c.presion = (*(__IO double *) (BKPSRAM_BASE + 8*8));
  posicion = (*(__IO double *) (BKPSRAM_BASE + 9*8));
  //Calibración del HOME del motor con la posición antes del reset
  BSP_MotorControl_SetHome(0, -posicion);
  posicion = BSP_MotorControl_GetPosition(0);


  //Inicialización ángulo previo
  t = get_time(hrtc);
  efemerides = ephemeritas(c, t, local);
  pos = pos_Sol(efemerides.Azimut, efemerides.Aparente_Elevacion);
  pos_final = pos_Sol_inclinado_y_desviado(pos, 0, 0);
  prev_wid = angulo_track_wid(pos_final);

  //Conversión de grados a pasos del ángulo máximo
  float sesenta = floor(c.angulo_max*19/1.8);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  //Parada del seguidor si se pulsan 2 o más veces el FC de la izquierda
	  if(hardstop >= 3){
		  BSP_MotorControl_GoHome(0);
		  BSP_MotorControl_WaitWhileActive(0);
		  BSP_MotorControl_HardStop(0);
		  BSP_MotorControl_WaitWhileActive(0);
		  BSP_MotorControl_CmdDisable(0);

		  //Actualización del valor de posición del motor guardado en memoria, habilitando y deshabilitando acceso a memoria
		  HAL_PWR_EnableBkUpAccess();
		  *(__IO double *) (BKPSRAM_BASE + 9*8) = 0;
		  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0xBEBE);
		  //Deshabilitar escritura en memoria
		  HAL_PWR_DisableBkUpAccess();

		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET); //Apagado led verde
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);   //Encendido led rojo
	  }
	  else{
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET); //Encendido led verde
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);   //Apagado led rojo

		  //Si modo == 0 es que el seguidor se encuentra en uno de los finales de carrera
		  //Si se activa FC derecho recalibrar el 0 del motor
		  //Si se activa FC izquierdo ir a la posición del derecho y recalibrar 0
		  while (modo == 0 && widc != -60){
			  if (izquierda == 1){
				  posicion = BSP_MotorControl_GetPosition(0);
				  izquierda = 0;
				  modo = 1;
				  hardstop++;
				  while(derecha != 1){
					  BSP_MotorControl_Move(0, BACKWARD, 1);
					  BSP_MotorControl_WaitWhileActive(0);
				  }
				  derecha = 0;
				  modo = 1;
				  posicion = BSP_MotorControl_GetPosition(0);
				  posicion = posicion + sesenta;
				  BSP_MotorControl_SetHome(0, posicion);
				  posicion = BSP_MotorControl_GetPosition(0);
			  }
			  else if (derecha == 1){
				  derecha = 0;
				  modo = 1;
				  posicion = BSP_MotorControl_GetPosition(0);
				  posicion = posicion + sesenta;
				  BSP_MotorControl_SetHome(0, posicion);
				  posicion = BSP_MotorControl_GetPosition(0);
				  posicion = BSP_MotorControl_GetPosition(0);
			  }
		  }
		  if(widc == -60){
			  if (derecha == 1){
				  derecha = 0;
				  modo = 1;
				  posicion = BSP_MotorControl_GetPosition(0);
				  posicion = posicion + sesenta;
				  BSP_MotorControl_SetHome(0, posicion);
			  }
			 // izquierda = 0;
		  }

		  if (amanecer == 3){
			  amanecer = 0;
			  //llevar a posición del final carrera
			  while(derecha != 1 ){
				  BSP_MotorControl_Move(0, BACKWARD, 1);
				  BSP_MotorControl_WaitWhileActive(0);
			  }
			  posicion = BSP_MotorControl_GetPosition(0);
			  posicion = posicion + sesenta;
			  BSP_MotorControl_SetHome(0, posicion);

			  //Reset valores
			  derecha = 0;
			  modo = 1;
		  }


		  //Llamada a la función de conexión UART
		  else if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4) || HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5)){
			  //Habilitar escritura en memoria para actualizar modificaciones
			  HAL_PWR_EnableBkUpAccess();

			  //Llamada a función de comunicación UART
			  parada_usart();

			  //Actualización de los datos guardados en la memoria RAM
			  *(__IO double *) (BKPSRAM_BASE) = local.latitud;
			  *(__IO double *) (BKPSRAM_BASE + 8) = local.longitud;
			  *(__IO double *) (BKPSRAM_BASE + 2*8) = local.altitud;
			  *(__IO double *) (BKPSRAM_BASE + 3*8) = c.UTCOffset;
			  *(__IO double *) (BKPSRAM_BASE + 4*8) = c.GCR;
			  *(__IO double *) (BKPSRAM_BASE + 5*8) = c.backtrack;
			  *(__IO double *) (BKPSRAM_BASE + 6*8) = c.angulo_max;
			  *(__IO double *) (BKPSRAM_BASE + 7*8) = c.temperatura;
			  *(__IO double *) (BKPSRAM_BASE + 8*8) = c.presion;
			  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0xBEBE);
			  //Deshabilitar escritura en memoria
			  HAL_PWR_EnableBkUpAccess();
		  }
		  else if (modo == 1){ //Código funcionamiento normal del seguidor, cáclulo del ángulo óptimo y movimiento del motor a esa posición
			  t = get_time(hrtc);
			  efemerides = ephemeritas(c, t, local);
			  pos = pos_Sol(efemerides.Azimut, efemerides.Aparente_Elevacion);
			  pos_final = pos_Sol_inclinado_y_desviado(pos, 0, 0);
			  wid = angulo_track_wid(pos_final);
			  widc = backtracking(c, wid, pos_final);
			  if(prev_wid == 0){
				  if(wid < -60){
					  amanecer = 3;
					  widc = -60;
				  }
				  else{
					  ang_eq = floor(widc*19/1.8); //Conversión del ángulo en grados a pasos del motor
					  BSP_MotorControl_GoTo(0, ang_eq);
					  BSP_MotorControl_WaitWhileActive(0);
				  }
			  }
			  else{
				  ang_eq = floor(widc*19/1.8); //Conversión del ángulo en grados a pasos del motor
				  BSP_MotorControl_GoTo(0, ang_eq);
				  BSP_MotorControl_WaitWhileActive(0);
			  }
			  prev_wid = wid;
		  }
		  posicion = BSP_MotorControl_GetPosition(0);
		  //Actualización del valor de posición del motor guardado en memoria, habilitando y deshabilitando acceso a memoria
		  HAL_PWR_EnableBkUpAccess();
		  *(__IO double *) (BKPSRAM_BASE + 9*8) = posicion;
		  HAL_PWR_DisableBkUpAccess();
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

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
