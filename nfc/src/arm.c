#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* SM13 includes */
#include "debug.h"
#include "mot_pap.h"
#include "rtu_com_hmi.h"

#define arm_TASK_PRIORITY ( configMAX_PRIORITIES - 2 )

struct mot_pap arm;
static void arm_supervisor_task();

QueueHandle_t arm_queue = NULL;

static void arm_task(void* par)
{
	struct mot_pap_msg *msg_rcv;

	while (1) {
		if (xQueueReceive(arm_queue, &msg_rcv, portMAX_DELAY) == pdPASS)
		{
			lDebug(Debug, "arm: command received");

			switch (msg_rcv->type)
			{

			case MOT_PAP_TYPE_FREE_RUNNING:
				lDebug(Debug, "Arm free_run_direction: %d", msg_rcv->free_run_direction);
				/*	cw/ccw limits!!	*/
				if (msg_rcv->free_run_direction) { lDebug(Info, "Giro Anti-horario"); }else { lDebug(Debug, "Giro Horario"); }
				lDebug(Info, "Arm free_run_speed: %d", msg_rcv->free_run_speed);
				break;
			case MOT_PAP_TYPE_CLOSED_LOOP:	//PID
				lDebug(Info, "Arm closed_loop_setpoint: %x", msg_rcv->closed_loop_setpoint);
				//calcular error de posici�n
				break;
			default:
				lDebug(Info, "STOP ARM");
				break;
			}

			vPortFree(msg_rcv);

		}
	}

}

static void arm_supervisor_task()
{
	arm.dir = MOT_PAP_TYPE_STOP;
	arm.posCmd = 0xFFEE;
	arm.posAct = 0xFFEE;
	arm.freq = 5;
}


void arm_init()
{
	
	arm_queue = xQueueCreate(5, sizeof(struct mot_pap_msg*));
	//xTaskCreate(arm_task, "arm", configMINIMAL_STACK_SIZE, NULL, arm_TASK_PRIORITY, NULL);
	xTaskCreate(arm_task, "arm", configMINIMAL_STACK_SIZE*2, NULL, 4, NULL);
	lDebug(Debug, "arm.c", "arm_task - TaskCreate"); //Pablo Priority Debug: Borrar
}

struct mot_pap *arm_get_status(void)
{
	arm_supervisor_task();
	return &arm;
}
