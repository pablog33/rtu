/* SM13 Remote Terminal Unit -RTU- Prototype TCP Server
 Automatically connects to HMI running on remote client */

/* SM13 Remote Terminal Unit -RTU- Prototype TCP Server
 Automatically connects to HMI running on remote client */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "queue.h"

/*	RTUcomHMI	*/
#include "bitops.h"
#include "debug.h"
#include "lift.h"
#include "mot_pap.h"
#include "rtu_com_hmi.h"
#include "relay.h"

bool stall_detection;

void TaskTriggerMsg(HMICmd_t *pHMICmd) {
	static unsigned char ucPreviousFlagByte;
	static bool bStallPreviousFlag;
	static uint16_t uiPreviousPosCmdArm = 0, uiPreviousPosCmdPole = 0;
	unsigned char ucActualFlagByte, ucEventFlagByte, ucMode_ActualBits,
			ucMode_EventBits;
	bool bSendToArm, bSendToPole, bSendToLift, bControlEnable_EventBit,
			bTypeStop, bTypeFreeRunStart, bTypeAutoStart, bTypeLiftUp,
			bTypeLiftDown;

	ucActualFlagByte = 0x00;
	bSendToArm = FALSE;
	bSendToPole = FALSE;
	bSendToLift = FALSE;
	bTypeStop = FALSE;
	bTypeFreeRunStart = FALSE;
	bTypeAutoStart = FALSE;
	bTypeLiftUp = FALSE;
	bTypeLiftDown = FALSE;

	struct mot_pap_msg *pArmMsg;
	struct mot_pap_msg *pPoleMsg;
	struct lift_msg *pLiftMsg;

	stall_detection = pHMICmd->stallEn;
	if (stall_detection ^ bStallPreviousFlag) {
		if (stall_detection) {
			lDebug(Info, "Stall ENABLED");
		} else {
			lDebug(Info, "Stall DISABLED");
		}
	}
	bStallPreviousFlag = stall_detection;

	/*	-- ucActualFlagByte -- Se consituye un byte donde 3 de sus bits -b0 a b2- representan 
	 b0b1: mode. 00: STOP, 01: FREE RUN, 10: AUTO, 11: LIFT
	 b2: ctrlEn. 0: Disabled, 1: Enabled.
	 b3: freeRunDir. 0:CW, 1: CCW
	 b4: lift Dir. 0: Down, 1: Up
	 b5: Axis. 0: Arm, 1: Pole
	 b6: PosCmdChg. 0: Sin cambios. 1: Con cambios de valores estando el modo en "AUTO"
	 */
	/*	-- mode --	*/
	if (pHMICmd->mode == eStop) {
	} else if (pHMICmd->mode == eFree_run) {
		BitSet(ucActualFlagByte, bit0);
	} else if (pHMICmd->mode == eAuto) {
		BitSet(ucActualFlagByte, bit1);
	} else if (pHMICmd->mode == eLift) {
		BitSet(ucActualFlagByte, bit0);BitSet(ucActualFlagByte, bit1);
	} else {
		lDebug(Info, "Info - prvTrigger: pHMICmd->mode");
	}
	/*	-- ctrlEn --	*/
	if (pHMICmd->ctrlEn == eEnable) {
		BitSet(ucActualFlagByte, bit2);
	}
	/* -- ArmPoleDir -- */
	if (pHMICmd->freeRunDir == eCCW) {
		BitSet(ucActualFlagByte, bit3);
	}
	/* -- Axis -- */
	if (pHMICmd->freeRunAxis == ePole) {
		BitSet(ucActualFlagByte, bit4);
	}
	/* -- LiftDir -- */
	if (pHMICmd->liftDir == eUp) {
		BitSet(ucActualFlagByte, bit5);
	}


	ucEventFlagByte = ucActualFlagByte ^ ucPreviousFlagByte;

	/* -- posCmd -- */
		if (pHMICmd->posCmdArm != uiPreviousPosCmdArm) {

			BitSet(ucEventFlagByte, bit6);

		}

		if (pHMICmd->posCmdPole != uiPreviousPosCmdPole) {

			BitSet(ucEventFlagByte, bit6);

		}

	/* Discriminaci�n de bits para Mode, CtrlEn, y Lift, para manipulaci�n */
	ucMode_ActualBits = 0x03 & ucActualFlagByte;
	ucMode_EventBits = 0x03 & ucEventFlagByte;
	bControlEnable_EventBit = BitStatus(ucEventFlagByte, bit2);

	/*		--	CONDICIONAMIENTOS	--
	 La l�gica que define los mensajes para el disparo (desbloqueo) de las tareas que administran los dispositivos -Arm, Pole, y
	 Lift- solo se ejecutan si se detect� alg�n cambio en los comandos ingresados desde la estaci�n HMI -HMICmd-, los cuales se
	 registran mediante el byte de banderas -ucEventFlagByte- (en �l, cada bit es una bandera). Tambi�n en el caso en que se
	 produzca la deshabilitaci�n de la variable -crtlEn-, en cuya situaci�n se detienen todos los procesos. Mientras -ctrlEn-,
	 se encuentre deshabilitado, no se vuelve a ejecutar esta l�gica de gesti�n de mensajes para inicio tareas
	 */
	if (ucEventFlagByte != 0x00
			&& (pHMICmd->ctrlEn || BitStatus(ucPreviousFlagByte, bit2))) {
		/*	-- Mode Trigger --	*/

		switch (ucMode_ActualBits) {

		case 0x00: /* STOP COMMAND*/

			if (ucMode_EventBits == 0x01 || (BitStatus(ucEventFlagByte, bit4)) ) /*	-- STOP FREE RUN COMMAND --		*/
			{
				lDebug(Info, "STOP FR MODE");
				bTypeStop = TRUE;
				if (pHMICmd->freeRunAxis == eArm) {
					bSendToArm = TRUE;
				} else {
					bSendToPole = TRUE;
				}
				if (pHMICmd->mode != eStop) {
					lDebug(Error,
							"error Condicionamiento STOP FREE RUN COMMAND");
				} /* Deberia corresponder solo al modo STOP */
			}

			else if (ucMode_EventBits == 0x02) /*	-- STOP AUTO COMMAND --		*/
			{
				bSendToArm = TRUE;
				bSendToPole = TRUE;
				lDebug(Info, " STOP AUTO MODE");
				bTypeStop = TRUE;
				if (pHMICmd->mode != eStop) {
					lDebug(Error,
							"error Condicionamiento STOP AUTO MODE COMMAND");
				} /* Deberia corresponder solo al modo Automatico */
			}

			else if (ucMode_EventBits == 0x03) /*	-- STOP LIFT COMMAND --		*/
			{
				lDebug(Info, " STOP LIFT");
				bTypeStop = TRUE;
				bSendToLift = TRUE;
				if (pHMICmd->mode != eStop) {
					lDebug(Error, "error Condicionamiento STOP LIFT COMMAND");
				} /* Deberia corresponder solo al modo Lift */

			}

			break;

		case 0x01: /*	--	START FREE RUN COMMAND --	*/

			lDebug(Info, "START FR MODE");
			bTypeFreeRunStart = TRUE;
			if (pHMICmd->freeRunAxis == eArm) {
				bSendToArm = TRUE;
			} else {
				bSendToPole = TRUE;
			}
			if (pHMICmd->mode != eFree_run) {
				lDebug(Error, "error Condicionamiento START FREE RUN COMMAND");
			} /* Deberia corresponder solo al modo FreeRun */

			if (ucMode_EventBits != 0x01) {
				lDebug(Warn, "TaskTriggerMsg:ucMode_ActualBits case 0x01");
			}

			break;

		case 0x02: /*	--	START AUTO COMMAND --	*/
			bSendToArm = TRUE;
			bSendToPole = TRUE;
			bTypeAutoStart = TRUE;
			lDebug(Info, " START AUTO MODE");

			if (pHMICmd->mode != eAuto) {
				lDebug(Error, "error Condicionamiento START AUTO MODE COMMAND");
			} /* Deberia corresponder solo al modo Automatico */

			if (ucMode_EventBits != 0x02 && !(ucEventFlagByte & 0x40)) {
				lDebug(Warn, "TaskTriggerMsg:ucMode_ActualBits case 0x02");
			}

			break;

		case 0x03:	/*	--	START LIFT COMMAND --	*/
			bSendToLift = TRUE;
			if (pHMICmd->liftDir == eUp) {
				bTypeLiftUp = TRUE;
			} else {
				bTypeLiftDown = TRUE;
			}

			lDebug(Info, " START LIFT");

			if (pHMICmd->mode != eLift) {
				lDebug(Error, "error Condicionamiento START LIFT COMMAND");
			}/* Deberia corresponder solo al modo Lift */

			if (ucMode_EventBits != 0x03) {
				lDebug(Warn, "TaskTriggerMsg:ucMode_ActualBits case 0x03");
			}

			break;

		}

		/*	-- CtrlEn TaskTriggerMsg --	*/
		/*	Evalia solo la deshabiliataciin del bit -Actual- correspondiente a Control Enable, ya que una vez en -eDisabled-
		 no se procesar� la l�gica desde -CONDICIONAMIENTOS-	*/
		if (bControlEnable_EventBit) {
			if (pHMICmd->ctrlEn == eDisable) {
				bTypeStop = TRUE;
				bSendToArm = TRUE;
				bSendToPole = TRUE;
				bSendToLift = TRUE;
				relay_main_pwr(false);
				lDebug(Info, " CONTROL DISABLE! STOP ALL!");
				if (pHMICmd->ctrlEn != eDisable) {
					lDebug(Error, "error CONTROL ENABLE BIT");
				}
			} else {
				lDebug(Info, " Se activa el control -CONTROL ENABLE-!");
				relay_main_pwr(true);
			}
		}

		if (bSendToArm) {
			pArmMsg = (struct mot_pap_msg*) pvPortMalloc(
					sizeof(struct mot_pap_msg));
			if (bTypeStop) {
				pArmMsg->type = MOT_PAP_TYPE_STOP;
			} else if (bTypeFreeRunStart) {
				pArmMsg->type = MOT_PAP_TYPE_FREE_RUNNING;
			} else if (bTypeAutoStart) {
				pArmMsg->type = MOT_PAP_TYPE_CLOSED_LOOP;
			} else {
				lDebug(Info, " Info bSendToArm");
			}
			pArmMsg->free_run_direction = pHMICmd->freeRunDir;
			pArmMsg->free_run_speed = pHMICmd->velCmdArm;
			pArmMsg->closed_loop_setpoint = pHMICmd->posCmdArm;
			if (xQueueSend(arm_queue, &pArmMsg, portMAX_DELAY) == pdPASS) {
				lDebug(Debug, " Comando enviado a arm.c exitoso!");
			} else {
				lDebug(Debug, "Comando NO PUDO ser enviado a arm.c");
			}
		}
		if (bSendToPole) {
			pPoleMsg = (struct mot_pap_msg*) pvPortMalloc(
					sizeof(struct mot_pap_msg));
			if (bTypeStop) {
				pPoleMsg->type = MOT_PAP_TYPE_STOP;
			} else if (bTypeFreeRunStart) {
				pPoleMsg->type = MOT_PAP_TYPE_FREE_RUNNING;
			} else if (bTypeAutoStart) {
				pPoleMsg->type = MOT_PAP_TYPE_CLOSED_LOOP;
			} else {
				lDebug(Info, "Info bSendToPole");
			}
			pPoleMsg->free_run_direction = pHMICmd->freeRunDir;
			pPoleMsg->free_run_speed = pHMICmd->velCmdPole;
			pPoleMsg->closed_loop_setpoint = pHMICmd->posCmdPole;
			if (xQueueSend(pole_queue, &pPoleMsg, portMAX_DELAY) == pdPASS) {
				lDebug(Debug, "Comando enviado a pole.c exitoso!");
			} else {
				lDebug(Debug, "Comando NO PUDO ser enviado a pole.c");
			}
		}
		if (bSendToLift) {
			pLiftMsg = (struct lift_msg*) pvPortMalloc(sizeof(struct lift_msg));
			if (bTypeStop) {
				pLiftMsg->type = LIFT_TYPE_STOP;
			} else if (bTypeLiftUp) {
				pLiftMsg->type = LIFT_TYPE_UP;
			} else if (bTypeLiftDown) {
				pLiftMsg->type = LIFT_TYPE_DOWN;
			} else {
				lDebug(Info, "Info bSendToLift");
			}
			if (xQueueSend(lift_queue, &pLiftMsg, portMAX_DELAY) == pdPASS) {
				lDebug(Debug, "Comando enviado a lift.c exitoso!");
			} else {
				lDebug(Debug, "Comando NO PUDO ser enviado a lift.c");
			}
		}

	}

	ucPreviousFlagByte = ucActualFlagByte;
	uiPreviousPosCmdArm = pHMICmd->posCmdArm;
	uiPreviousPosCmdPole = pHMICmd->posCmdPole;

	return;
}
/*-----------------------------------------------------------*/
