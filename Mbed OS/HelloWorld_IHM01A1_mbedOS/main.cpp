/**
 ******************************************************************************
 * @file    main.cpp
 * @author  Davide Aliprandi, STMicroelectronics
 * @version V1.0.0
 * @date    October 14th, 2015
 * @brief   mbed test application for the STMicroelectronics X-NUCLEO-IHM01A1
 *          Motor Control Expansion Board: control of 1 motor.
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
#include "rtos.h"

/* Helper header files. */
#include "DevSPI.h"

/* Component specific header files. */
#include "L6474.h"


/* Definitions ---------------------------------------------------------------*/

/* Number of steps. */
#define STEPS_1 (400 * 8)   /* 1 revolution given a 400 steps motor configured at 1/8 microstep mode. */

/* Delay in milliseconds. */
#define DELAY_1 1000
#define DELAY_2 2000
#define DELAY_3 6000
#define DELAY_4 8000

/* Speed in pps (Pulses Per Second).
   In Full Step mode: 1 pps = 1 step/s).
   In 1/N Step Mode:  N pps = 1 step/s). */
#define SPEED_1 2400
#define SPEED_2 1200


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
    L6474_STEP_SEL_1_8,               /* Step selection (STEP_SEL field of STEP_MODE register). */
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
 * @param  None
 * @retval None
 * @note   If needed, implement it, and then attach and enable it:
 *           + motor->attach_flag_irq(&flag_irq_handler);
 *           + motor->enable_flag_irq();
 *         To disable it:
 *           + motor->disble_flag_irq();
 */
void flag_irq_handler(void)
{
    /* Set ISR flag. */
    motor->isr_flag = TRUE;

    /* Get the value of the status register. */
    unsigned int status = motor->get_status();

    /* Check NOTPERF_CMD flag: if set, the command received by SPI can't be performed. */
    /* This often occures when a command is sent to the L6474 while it is not in HiZ state. */
    if ((status & L6474_STATUS_NOTPERF_CMD) == L6474_STATUS_NOTPERF_CMD) {
        printf("    WARNING: \"FLAG\" interrupt triggered. Non-performable command detected when updating L6474's registers while not in HiZ state.\r\n");
    }
    
    /* Reset ISR flag. */
    motor->isr_flag = FALSE;
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

    /* Attaching and enabling interrupt handlers. */
    motor->attach_flag_irq(&flag_irq_handler);
    motor->enable_flag_irq();

    /* Printing to the console. */
    printf("Motor Control Application Example for 1 Motor\r\n\n");


    /*----- Moving. -----*/

    /* Printing to the console. */
    printf("--> Moving forward %d steps.\r\n", STEPS_1);

    /* Moving N steps in the forward direction. */
    motor->move(StepperMotor::FWD, STEPS_1);

    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Getting current position. */
    int position = motor->get_position();
    
    /* Printing to the console. */
    printf("    Position: %d.\r\n", position);

    /* Waiting. */
    wait_ms(DELAY_1);


    /*----- Changing the motor setting. -----*/

    /* Printing to the console. */
    printf("--> Setting Torque Regulation Current to 500[mA].\r\n");

    /* Increasing the torque regulation current to 500[mA]. */
    motor->set_parameter(L6474_TVAL, 500);

    /* Printing to the console. */
    printf("--> Doubling the microsteps.\r\n");

    /* Doubling the microsteps. */
    if (!motor->set_step_mode((StepperMotor::step_mode_t) STEP_MODE_1_16)) {
        printf("    Step Mode not allowed.\r\n");
    }

    /* Waiting. */
    wait_ms(DELAY_1);

    /* Printing to the console. */
    printf("--> Setting Home.\r\n");

    /* Setting the current position to be the home position. */
    motor->set_home();

    /* Getting current position. */
    position = motor->get_position();
    
    /* Printing to the console. */
    printf("    Position: %d.\r\n", position);
    
    /* Waiting. */
    wait_ms(DELAY_2);


    /*----- Moving. -----*/
    
    /* Printing to the console. */
    printf("--> Moving backward %d steps.\r\n", STEPS_1);

    /* Moving N steps in the backward direction. */
    motor->move(StepperMotor::BWD, STEPS_1);
    
    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Getting current position. */
    position = motor->get_position();
    
    /* Printing to the console. */
    printf("    Position: %d.\r\n", position);

    /* Waiting. */
    wait_ms(DELAY_1);


    /*----- Going to a specified position. -----*/

    /* Printing to the console. */
    printf("--> Going to position %d.\r\n", STEPS_1);
    
    /* Requesting to go to a specified position. */
    motor->go_to(STEPS_1);
    
    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Getting current position. */
    position = motor->get_position();
    
    /* Printing to the console. */
    printf("    Position: %d.\r\n", position);
    
    /* Waiting. */
    wait_ms(DELAY_2);

    
    /*----- Going Home. -----*/

    /* Printing to the console. */
    printf("--> Going Home.\r\n");
    
    /* Requesting to go to home. */
    motor->go_home();
    
    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Getting current position. */
    position = motor->get_position();

    /* Printing to the console. */
    printf("    Position: %d.\r\n", position);

    /* Waiting. */
    wait_ms(DELAY_2);


    /*----- Running. -----*/

    /* Printing to the console. */
    printf("--> Running backward for %d seconds.\r\n", DELAY_3 / 1000);

    /* Requesting to run backward. */
    motor->run(StepperMotor::BWD);

    /* Waiting. */
    wait_ms(DELAY_3);

    /* Getting current speed. */
    int speed = motor->get_speed();

    /* Printing to the console. */
    printf("    Speed: %d.\r\n", speed);

    /*----- Increasing the speed while running. -----*/

    /* Printing to the console. */
    printf("--> Increasing the speed while running again for %d seconds.\r\n", DELAY_3 / 1000);

    /* Increasing the speed. */
    motor->set_max_speed(SPEED_1);

    /* Waiting. */
    wait_ms(DELAY_3);

    /* Getting current speed. */
    speed = motor->get_speed();

    /* Printing to the console. */
    printf("    Speed: %d.\r\n", speed);


    /*----- Decreasing the speed while running. -----*/

    /* Printing to the console. */
    printf("--> Decreasing the speed while running again for %d seconds.\r\n", DELAY_4 / 1000);

    /* Decreasing the speed. */
    motor->set_max_speed(SPEED_2);

    /* Waiting. */
    wait_ms(DELAY_4);

    /* Getting current speed. */
    speed = motor->get_speed();

    /* Printing to the console. */
    printf("    Speed: %d.\r\n", speed);


    /*----- Hard Stop. -----*/

    /* Printing to the console. */
    printf("--> Hard Stop.\r\n");

    /* Requesting to immediatly stop. */
    motor->hard_stop();

    /* Waiting while the motor is active. */
    motor->wait_while_active();

    /* Waiting. */
    wait_ms(DELAY_2);


    /*----- Infinite Loop. -----*/

    /* Printing to the console. */
    printf("--> Infinite Loop...\r\n");

    /* Setting the current position to be the home position. */
    motor->set_home();

    /* Infinite Loop. */
    while (true) {
        /* Requesting to go to a specified position. */
        motor->go_to(STEPS_1 >> 1);

        /* Waiting while the motor is active. */
        motor->wait_while_active();

        /* Requesting to go to a specified position. */
        motor->go_to(- (STEPS_1 >> 1));

        /* Waiting while the motor is active. */
        motor->wait_while_active();
    }
}
