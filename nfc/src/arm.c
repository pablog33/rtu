#include "arm.h"
#include "mot_pap.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"

#include "ad2s1210.h"
#include "debug.h"
#include "dout.h"
#include "pid.h"
#include "tmr.h"

#define ARM_TASK_PRIORITY ( configMAX_PRIORITIES - 2 )
#define ARM_SUPERVISOR_TASK_PRIORITY ( configMAX_PRIORITIES )

QueueHandle_t arm_queue = NULL;

SemaphoreHandle_t arm_supervisor_semaphore;

static struct mot_pap arm;
static struct ad2s1210 rdc;
static struct pid pid;

/**
 * @brief 	handles the arm movement.
 * @param 	par		: unused
 * @return	never
 * @note	Receives commands from arm_queue
 */
static void arm_task(void *par)
{
	struct mot_pap_msg *msg_rcv;

	while (true) {
		if (xQueueReceive(arm_queue, &msg_rcv, portMAX_DELAY) == pdPASS) {
			lDebug(Info, "arm: command received");

			mot_pap_init_limits(&arm);

			switch (msg_rcv->type) {
			case MOT_PAP_TYPE_FREE_RUNNING:
				mot_pap_move_free_run(&arm, msg_rcv->free_run_direction,
						msg_rcv->free_run_speed);
				break;

			case MOT_PAP_TYPE_CLOSED_LOOP:
				mot_pap_move_closed_loop(&arm, msg_rcv->closed_loop_setpoint);
				break;

			default:
				mot_pap_stop(&arm);
				break;
			}

			vPortFree(msg_rcv);
		}
	}
}

/**
 * @brief	checks if soft limits are reached, if stalled and if position reached in closed loop.
 * @param 	par	: unused
 * @return	never
 */
static void arm_supervisor_task(void *par)
{
	while (true) {
		xSemaphoreTake(arm_supervisor_semaphore, portMAX_DELAY);
		mot_pap_supervise(&arm);
	}
}

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle arm movements.
 * @return	nothing
 */
void arm_init()
{
	arm_queue = xQueueCreate(5, sizeof(struct mot_pap_msg*));

	arm.name = "arm";
	arm.type = MOT_PAP_TYPE_STOP;
	arm.cwLimit = 65535;
	arm.ccwLimit = 0;
	arm.last_dir = MOT_PAP_DIRECTION_CW;
	arm.half_pulses = 0;
	arm.offset = 0;

	rdc.gpios.reset = &poncho_rdc_reset;
	rdc.gpios.sample = &poncho_rdc_sample;
	rdc.gpios.wr_fsync = &poncho_rdc_arm_wr_fsync;
	rdc.resolution = 16;
	rdc.fclkin = 8192000;
	rdc.fexcit = 2000;
	ad2s1210_init(&rdc);

	arm.rdc = &rdc;

	pid_controller_init(&pid, 10, 20, 20, 20, 100);

	arm.pid = &pid;

	arm.gpios.direction = &dout_arm_dir;
	arm.gpios.pulse = &dout_arm_pulse;

	arm.tmr.started = false;
	arm.tmr.lpc_timer = LPC_TIMER1;
	arm.tmr.rgu_timer_rst = RGU_TIMER1_RST;
	arm.tmr.clk_mx_timer = CLK_MX_TIMER1;
	arm.tmr.timer_IRQn = TIMER1_IRQn;

	tmr_init(&arm.tmr);

	arm_supervisor_semaphore = xSemaphoreCreateBinary();
	arm.supervisor_semaphore = arm_supervisor_semaphore;

	if (arm_supervisor_semaphore != NULL) {
		// Create the 'handler' task, which is the task to which interrupt processing is deferred
		xTaskCreate(arm_supervisor_task, "ArmSupervisor",
		2048,
		NULL, ARM_SUPERVISOR_TASK_PRIORITY, NULL);
		lDebug(Info, "arm: supervisor task created");
	}

	xTaskCreate(arm_task, "Arm", 512, NULL,
	ARM_TASK_PRIORITY, NULL);

	lDebug(Info, "arm: task created");
}

/**
 * @brief	handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @return	nothing
 * @note 	calls the supervisor task every x number of generated steps
 */
void TIMER1_IRQHandler(void)
{
	if (tmr_match_pending(&(arm.tmr))) {
		mot_pap_isr(&arm);
	}
}

/**
 * @brief	gets arm RDC position
 * @return	RDC position
 */
uint16_t arm_get_RDC_position()
{
	return ad2s1210_read_position(arm.rdc);
}


/**
 * @brief	sets arm offset
 * @param 	offset		: RDC position for 0 degrees
 * @return	nothing
 */
void arm_set_offset(uint16_t offset)
{
	arm.offset = offset;
}

/**
 * @brief	sets arm CW limit
 * @param 	pos		: RDC position where the limit is reached
 * @return	nothing
 */
void arm_set_cwLimit(uint16_t pos)
{
	arm.cwLimit = pos;
}

/**
 * @brief	sets arm CCW limit
 * @param 	pos		: RDC position where the limit is reached
 * @return	nothing
 */
void arm_set_ccwLimit(uint16_t pos)
{
	arm.ccwLimit = pos;
}

/**
 * @brief	returns status of the arm task.
 * @return 	copy of status structure of the task
 */
struct mot_pap *arm_get_status(void) /* GPa 201207 retorna (*) */
{
	return &arm; /* GPa 201207 retorna (&) */
}

