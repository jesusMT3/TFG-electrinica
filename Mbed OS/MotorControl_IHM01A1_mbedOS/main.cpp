/**
 ******************************************************************************
 * @file    main.cpp
 * @author  Davide Aliprandi, STMicrolectronics
 * @version V1.0.0
 * @date    October 14th, 2015
 * @brief   mbed test application for the STMicroelectronics X-NUCLEO-IHM01A1
 *          Motor Control Expansion Board: control of 1 motor showing the usage
 *          of all the related APIs.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
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

/* mbed specific header files. */
#include "mbed.h"

/* Helper header files. */
#include "DevSPI.h"

/* Component specific header files. */
#include "L6474.h"


/* Variables -----------------------------------------------------------------*/

/* Initialization parameters. */
L6474_init_t init = {
    160,                              /* Acceleration rate in pps^2. Range: (0..+inf). */
    160,                              /* Deceleration rate in pps^2. Range: (0..+inf). */
    1600,                             /* Maximum speed in pps. Range: (30..10000]. */
    800,                              /* Minimum speed in pps. Range: [30..10000). */
    250,                              /* Torque regulation current in mA. Range: 31.25mA to 4000mA. */
    L6474_OCD_TH_750mA,               /* Overcurrent threshold (OCD_TH register). */
    L6474_CONFIG_OC_SD_ENABLE,        /* Overcurrent shutwdown (OC_SD field of CONFIG register). */
    L6474_CONFIG_EN_TQREG_TVAL_USED,  /* Torque regulation method (EN_TQREG field of CONFIG register). */
    L6474_STEP_SEL_1_16,              /* Step selection (STEP_SEL field of STEP_MODE register). */
    L6474_SYNC_SEL_1_2,               /* Sync selection (SYNC_SEL field of STEP_MODE register). */
    L6474_FAST_STEP_12us,             /* Fall time value (T_FAST field of T_FAST register). Range: 2us to 32us. */
    L6474_TOFF_FAST_8us,              /* Maximum fast decay time (T_OFF field of T_FAST register). Range: 2us to 32us. */
    3,                                /* Minimum ON time in us (TON_MIN register). Range: 0.5us to 64us. */
    21,                               /* Minimum OFF time in us (TOFF_MIN register). Range: 0.5us to 64us. */
    L6474_CONFIG_TOFF_044us,          /* Target Swicthing Period (field TOFF of CONFIG register). */
    L6474_CONFIG_SR_320V_us,          /* Slew rate (POW_SR field of CONFIG register). */
    L6474_CONFIG_INT_16MHZ,           /* Clock setting (OSC_CLK_SEL field of CONFIG register). */
    L6474_ALARM_EN_OVERCURRENT |
    L6474_ALARM_EN_THERMAL_SHUTDOWN |
    L6474_ALARM_EN_THERMAL_WARNING |
    L6474_ALARM_EN_UNDERVOLTAGE |
    L6474_ALARM_EN_SW_TURN_ON |
    L6474_ALARM_EN_WRONG_NPERF_CMD    /* Alarm (ALARM_EN register). */
};

/* Motor Control Component. */
L6474 *motor;


/* Functions -----------------------------------------------------------------*/

/**
  * @brief  This is an example of user handler for the flag interrupt.
  *         Empty parts can be implemented by the user upon needs.
  * @param  None.
  * @retval None.
  * @note   If needed, implement it, and then attach and enable it:
  *           + motor->AttachFlagIRQ(&FlagIRQHandler);
  *           + motor->EnableFlagIRQ();
  *         To disable it:
  *           + motor->DisbleFlagIRQ();
  */
void FlagIRQHandler(void)
{
    /* Set ISR flag. */
    motor->isr_flag = TRUE;

    /* Get the value of the status register. */
    unsigned int status = motor->get_status();
    
    /* Check HIZ flag: if set, power brigdes are disabled. */
    if ((status & L6474_STATUS_HIZ) == L6474_STATUS_HIZ)
    { /* HIZ state. Action to be customized. */ }
    
    /* Check direction. */
    if ((status & L6474_STATUS_DIR) == L6474_STATUS_DIR)
    { /* Forward direction is set. Action to be customized. */ }
    else
    { /* Backward direction is set. Action to be customized. */ }
    
    /* Check NOTPERF_CMD flag: if set, the command received by SPI can't be performed. */
    /* This often occures when a command is sent to the L6474 while it is not in HiZ state. */
    if ((status & L6474_STATUS_NOTPERF_CMD) == L6474_STATUS_NOTPERF_CMD)
    { /* Command received by SPI can't be performed. Action to be customized. */ }
     
    /* Check WRONG_CMD flag: if set, the command does not exist. */
    if ((status & L6474_STATUS_WRONG_CMD) == L6474_STATUS_WRONG_CMD)
    { /* The command received by SPI does not exist. Action to be customized. */ }
    
    /* Check UVLO flag: if not set, there is an undervoltage lock-out. */
    if ((status & L6474_STATUS_UVLO) == 0)
    { /* Undervoltage lock-out. Action to be customized. */ }
    
    /* Check TH_WRN flag: if not set, the thermal warning threshold is reached. */
    if ((status & L6474_STATUS_TH_WRN) == 0)
    { /* Thermal warning threshold is reached. Action to be customized. */ }
    
    /* Check TH_SHD flag: if not set, the thermal shut down threshold is reached. */
    if ((status & L6474_STATUS_TH_SD) == 0)
    { /* Thermal shut down threshold is reached. Action to be customized. */ }
    
    /* Check OCD  flag: if not set, there is an overcurrent detection. */
    if ((status & L6474_STATUS_OCD) == 0)
    { /* Overcurrent detection. Action to be customized. */ }

    /* Reset ISR flag. */
    motor->isr_flag = FALSE;
}

/**
  * @brief  This is an example of user handler for the errors.
  * @param  error error-code.
  * @retval None
  * @note   If needed, implement it, and then attach it:
  *           + motor->AttachErrorHandler(&ErrorHandler);
  */
void ErrorHandler(uint16_t error)
{
    /* Printing to the console. */
    printf("Error: %d.\r\n", error);
    
    /* Aborting the program. */
    exit(EXIT_FAILURE);
}


/* Main ----------------------------------------------------------------------*/

int main()
{
    /*----- Initialization. -----*/

    /* Initializing SPI bus. */
    DevSPI dev_spi(D11, D12, D13);

    /* Initializing Motor Control Component. */
    motor = new L6474(D2, D8, D7, D9, D10, dev_spi);
    if (motor->init(&init) != COMPONENT_OK) {
        exit(EXIT_FAILURE);
    }

    /* Attaching and enabling the user handler for the flag interrupt. */
    motor->attach_flag_irq(&FlagIRQHandler);
    motor->enable_flag_irq();

    /* Printing to the console. */
    printf("Motor Control Application Example for 1 Motor\r\n\n");


    /*----- Moving forward 16000 steps. -----*/

    /* Printing to the console. */
    printf("--> Moving forward 16000 steps.\r\n");

    /* Moving 16000 steps in the forward direction. */
    motor->move(StepperMotor::FWD, 16000);
    
    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Waiting 2 seconds. */
    wait_ms(2000);

    
    /*----- Moving backward 16000 steps. -----*/
    
    /* Printing to the console. */
    printf("--> Moving backward 16000 steps.\r\n");
    
    /* Moving 16000 steps in the backward direction. */
    motor->move(StepperMotor::BWD, 16000);

    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Printing to the console. */
    printf("--> Setting Home.\r\n");

    /* Setting the current position to be the home position. */
    motor->set_home();

    /* Waiting 2 seconds. */
    wait_ms(2000);


    /*----- Going to a specified position. -----*/

    /* Printing to the console. */
    printf("--> Going to position -6400.\r\n");
    
    /* Requesting to go to a specified position. */
    motor->go_to(-6400);
    
    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Getting current position. */
    int position = motor->get_position();
    
    /* Printing to the console. */
    printf("    Position: %d.\r\n", position);
    
    /* Printing to the console. */
    printf("--> Setting a mark.\r\n");

    /* Setting the current position to be the mark position. */
    motor->set_mark();

    /* Waiting 2 seconds. */
    wait_ms(2000);

    
    /*----- Going Home. -----*/

    /* Printing to the console. */
    printf("--> Going Home.\r\n");
    
    /* Requesting to go to home */
    motor->go_home();  
    
    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Getting current position. */
    position = motor->get_position();

    /* Printing to the console. */
    printf("    Position: %d.\r\n", position);

    /* Waiting 2 seconds. */
    wait_ms(2000);


    /*----- Going to a specified position. -----*/

    /* Printing to the console. */
    printf("--> Going to position 6400.\r\n");
    
    /* Requesting to go to a specified position. */
    motor->go_to(6400);
    
    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Getting current position. */
    position = motor->get_position();

    /* Printing to the console. */
    printf("    Position: %d.\r\n", position);

    /* Waiting 2 seconds. */
    wait_ms(2000);


    /*----- Going to mark which was set previously after going to -6400. -----*/

    /* Printing to the console. */
    printf("--> Going to the mark set previously.\r\n");
    
    /* Requesting to go to mark position. */
    motor->go_mark();  
    
    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Getting current position. */
    position = motor->get_position();

    /* Printing to the console. */
    printf("    Position: %d.\r\n", position);

    /* Waiting 2 seconds. */
    wait_ms(2000);


    /*----- Running backward. -----*/

    /* Printing to the console. */
    printf("--> Running backward.\r\n");

    /* Requesting to run backward. */
    motor->run(StepperMotor::BWD);

    /* Waiting until delay has expired. */
    wait_ms(5000);

    /* Getting current speed. */
    int speed = motor->get_speed();

    /* Printing to the console. */
    printf("    Speed: %d.\r\n", speed);


    /*----- Increasing the speed while running. -----*/

    /* Printing to the console. */
    printf("--> Increasing the speed while running.\r\n");

    /* Increasing the speed. */
    motor->set_max_speed(2400);

    /* Waiting until delay has expired. */
    wait_ms(5000);

    /* Getting current speed. */
    speed = motor->get_speed();

    /* Printing to the console. */
    printf("    Speed: %d.\r\n", speed);


    /*----- Decreasing the speed while running. -----*/

    /* Printing to the console. */
    printf("--> Decreasing the speed while running.\r\n");

    /* Decreasing the speed. */
    motor->set_max_speed(1200);

    /* Waiting until delay has expired. */
    wait_ms(5000);

    /* Getting current speed. */
    speed = motor->get_speed();

    /* Printing to the console. */
    printf("    Speed: %d.\r\n", speed);


    /*----- Increasing acceleration while running. -----*/

    /* Printing to the console. */
    printf("--> Increasing acceleration while running.\r\n");

    /* Increasing the acceleration. */
    motor->set_acceleration(480);

    /* Waiting until delay has expired. */
    wait_ms(5000);

    /* Increasing the speed. */
    motor->set_max_speed(2400);

    /* Waiting until delay has expired. */
    wait_ms(5000);

    /* Getting current speed. */
    speed = motor->get_speed();

    /* Printing to the console. */
    printf("    Speed: %d.\r\n", speed);


    /*----- Increasing deceleration while running. -----*/

    /* Printing to the console. */
    printf("--> Increasing deceleration while running.\r\n");

    /* Increasing the deceleration. */
    motor->set_deceleration(480);

    /* Waiting until delay has expired. */
    wait_ms(5000);

    /* Decreasing the speed. */
    motor->set_max_speed(1200);

    /* Waiting until delay has expired. */
    wait_ms(5000);

    /* Getting current speed. */
    speed = motor->get_speed();

    /* Printing to the console. */
    printf("    Speed: %d.\r\n", speed);


    /*----- Requesting soft-stop while running. -----*/

    /* Printing to the console. */
    printf("--> Requesting soft-stop while running.\r\n");

    /* Requesting soft stop. */
    motor->soft_stop();

    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Waiting 2 seconds. */
    wait_ms(2000);


    /*----- Requesting hard-stop while running. -----*/

    /* Printing to the console. */
    printf("--> Running forward.\r\n");
    
    /* Requesting to run in forward direction. */
    motor->run(StepperMotor::FWD);

    /* Waiting until delay has expired. */
    wait_ms(5000);

    /* Printing to the console. */
    printf("--> Requesting hard-stop while running.\r\n");

    /* Requesting to immediatly stop. */
    motor->hard_stop();

    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Waiting 2 seconds. */
    wait_ms(2000);


    /*----- GOTO stopped by soft-stop. -----*/

    /* Printing to the console. */
    printf("--> Going to position 20000.\r\n");
    
    /* Requesting to go to a specified position. */
    motor->go_to(20000);  
    
    /* Waiting while the motor is active. */
    wait_ms(5000);

    /* Printing to the console. */
    printf("--> Requiring soft-stop while running.\r\n");

    /* Requesting to perform a soft stop */
    motor->soft_stop();

    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Waiting 2 seconds. */
    wait_ms(2000);


    /*----- Reading inexistent register to test "MyFlagInterruptHandler". -----*/

    /* Printing to the console. */
    printf("--> Reading inexistent register to test \"MyFlagInterruptHandler\".\r\n");

    /*
     * Trying to read an inexistent register.
     * The flag interrupt should be raised and the "MyFlagInterruptHandler"
     * function called.
     */
    motor->get_parameter(0x1F);

    /* Waiting 0.5 seconds. */
    wait_ms(500);


    /*----- Changing step mode to full step mode. -----*/

    /* Printing to the console. */
    printf("--> Changing step mode to full step mode.\r\n");

    /* Selecting full step mode. */
    if (!motor->set_step_mode((StepperMotor::step_mode_t) STEP_MODE_FULL)) {
        printf("    Step Mode not allowed.\r\n");
    }

    /* Setting speed and acceleration to be consistent with full step mode. */
    motor->set_max_speed(100);
    motor->set_min_speed(50);
    motor->set_acceleration(10);
    motor->set_deceleration(10);

    /* Requesting to go to a specified position. */
    motor->go_to(200);

    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Getting current position */
    position = motor->get_position();

    /* Printing to the console. */
    printf("    Position: %d.\r\n", position);

    /* Waiting 2 seconds. */
    wait_ms(2000);
    

    /*----- Restoring 1/16 microstepping mode. -----*/

    /* Printing to the console. */
    printf("--> Restoring 1/16 microstepping mode.\r\n");

    /* Resetting to 1/16 microstepping mode */
    if (!motor->set_step_mode((StepperMotor::step_mode_t) STEP_MODE_1_16)) {
        printf("    Step Mode not allowed.\r\n");
    }

    /* Update speed, acceleration, deceleration for 1/16 microstepping mode*/
    motor->set_max_speed(1600);
    motor->set_min_speed(800);
    motor->set_acceleration(160);
    motor->set_deceleration(160);  


    /*----- Infinite Loop. -----*/

    /* Printing to the console. */
    printf("--> Infinite Loop...\r\n");

    /* Infinite Loop. */
    while (true) {
        /* Requesting to go to a specified position. */
        motor->go_to(-6400);

        /* Waiting while the motor is active. */
        motor->wait_while_active();

        /* Requesting to go to a specified position. */
        motor->go_to(6400);

        /* Waiting while the motor is active. */
        motor->wait_while_active();
    }
}
