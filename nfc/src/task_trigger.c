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


void TaskTriggerMsg(HMICmd_t* pHMICmd)
{
	static unsigned char ucPreviousFlagByte;
	unsigned char ucActualFlagByte, ucEventFlagByte, ucMode_ActualBits, ucMode_EventBits;
	bool bSendToArm, bSendToPole, bSendToLift, bControlEnable_EventBit,bTypeStop, bTypeFreeRunStart,
	bTypeAutoStart, bTypeLiftUp, bTypeLiftDown;

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
	
	/*	-- ucActualFlagByte -- Se consituye un byte donde 3 de sus bits -b0 a b2- representan 
		b0b1: mode. 00: STOP, 01: FREE RUN, 10: AUTO, 11: LIFT
		b2: ctrlEn. 0: Desabled, 1: Enabled.
	*/
	/*	-- mode --	*/
	if (pHMICmd->mode == eStop) { ; }
	else if (pHMICmd->mode == eFree_run) { BitSet(ucActualFlagByte, bit0); }
	else if (pHMICmd->mode == eAuto)	 { BitSet(ucActualFlagByte, bit1); }
	else if (pHMICmd->mode == eLift)	 { BitSet(ucActualFlagByte, bit0); BitSet(ucActualFlagByte, bit1); }
	else { lDebug(Info, "Info - prvTrigger: pHMICmd->mode"); }
	/*	-- ctrlEn --	*/
	if (pHMICmd->ctrlEn == eEnable) { BitSet(ucActualFlagByte, bit2);  }

	ucEventFlagByte = ucActualFlagByte ^ ucPreviousFlagByte;

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
	if(ucEventFlagByte!=0x00 && (pHMICmd->ctrlEn || BitStatus(ucPreviousFlagByte, bit2)))
	{
		/*	-- Mode Trigger --	*/
		
		switch (ucMode_EventBits)
		{
		case 0x00:
			break;	/*	No se registraron eventos para Mode */
		
		case 0x01:
			if (ucMode_ActualBits == 0x00)	/*	-- STOP FREE RUN COMMAND --		*/
			{
				lDebug(Info, "STOP FR MODE");
				bTypeStop = TRUE;
				if (pHMICmd->freeRunAxis == eArm) { bSendToArm = TRUE; }
				else { bSendToPole = TRUE; }
				configASSERT(pHMICmd->mode == eStop); /* Deber�a corresponder solo al modo STOP */
			}
			else if (ucMode_ActualBits == 0x01)	/*	--	START FREE RUN COMMAND --	*/
			{
				lDebug(Info,  "START FR MODE");
				bTypeFreeRunStart = TRUE;
				if (pHMICmd->freeRunAxis == eArm) { bSendToArm = TRUE; }
				else { bSendToPole = TRUE; }
				configASSERT(pHMICmd->mode == eFree_run); /* Deber�a corresponder solo al modo FreeRun */
			}
			else { lDebug(Warn, "RTUcomHMI.c", " Info - prvTaskTriggerMsg:ucMode_EventBits case 0x01"); }

			break;

		case 0x02:
			bSendToArm = TRUE; 
			bSendToPole = TRUE;
			if (ucMode_ActualBits == 0x00)	/*	-- STOP AUTO COMMAND --		*/
			{
				lDebug(Info, " STOP AUTO MODE");
				bTypeStop = TRUE;
				configASSERT(pHMICmd->mode == eStop); /* Deber�a corresponder solo al modo Automatico */
			}
			else if (ucMode_ActualBits == 0x02)	/*	--	START AUTO COMMAND --	*/
			{
				lDebug(Info, " START AUTO MODE");
				bTypeAutoStart = TRUE;
				configASSERT(pHMICmd->mode == eAuto); /* Deber�a corresponder solo al modo Automatico */
			}
			else { lDebug(Warn, "RTUcomHMI.c", " Info - prvTaskTriggerMsg:ucMode_EventBits case 0x02"); }

			break;

		case 0x03:
			bSendToLift = TRUE;
			if (ucMode_ActualBits == 0x00)	/*	-- STOP LIFT COMMAND --		*/
			{
				lDebug(Info, " STOP LIFT");
				bTypeStop = TRUE;
				configASSERT(pHMICmd->mode == eStop); /* Deber�a corresponder solo al modo Lift */
			}
			else if (ucMode_ActualBits == 0x03)	/*	--	START LIFT COMMAND --	*/
			{
				lDebug(Info, " START LIFT");
				if (pHMICmd->liftDir == eUp) { bTypeLiftUp = TRUE; }
				else { bTypeLiftDown = TRUE; }
				configASSERT(pHMICmd->mode == eLift); /* Deber�a corresponder solo al modo Lift */
			}
			else { lDebug(Warn, " Info - prvTaskTriggerMsg:ucMode_EventBits case 0x03"); }
			
			break;
		}

		/*	-- CtrlEn TaskTriggerMsg --	*/
		/*	Eval�a solo la deshabiliataci�n del bit -Actual- correspondiente a Control Enable, ya que una vez en -eDesabled- 
			no se procesar� la l�gica desde -CONDICIONAMIENTOS-	*/
		if (bControlEnable_EventBit)
		{
			if (pHMICmd->ctrlEn == eDesable)
			{
				bTypeStop = TRUE;
				bSendToArm = TRUE;
				bSendToPole = TRUE;
				bSendToLift = TRUE;
				lDebug(Info, " CONTROL DISABLE! STOP ALL!");
				configASSERT(pHMICmd->ctrlEn == eDesable); /* Deber�a corresponder al HMICmd.CtrlEn = eDesable */
			}
			else { lDebug(Info, " Se activa el control -CONTROL ENABLE-!"); }
		}
		
		if(bSendToArm)
		{	pArmMsg = (struct mot_pap_msg*)pvPortMalloc(sizeof(struct mot_pap_msg));
			if (bTypeStop) { pArmMsg->type = MOT_PAP_TYPE_STOP; }
			else if (bTypeFreeRunStart) { pArmMsg->type = MOT_PAP_TYPE_FREE_RUNNING; }
			else if (bTypeAutoStart) { pArmMsg->type = MOT_PAP_TYPE_CLOSED_LOOP; }
			else { lDebug(Info, " Info bSendToArm"); }
			pArmMsg->free_run_direction = pHMICmd->freeRunDir;
			pArmMsg->free_run_speed = pHMICmd->velCmdArm;
			pArmMsg->closed_loop_setpoint = pHMICmd->posCmdArm;
			if (xQueueSend(arm_queue, &pArmMsg, portMAX_DELAY) == pdPASS) { lDebug(Debug, " Comando enviado a arm.c exitoso!"); }
			else { lDebug(Debug, "Comando NO PUDO ser enviado a arm.c"); }
		}
		if (bSendToPole)
		{
			pPoleMsg = (struct mot_pap_msg*)pvPortMalloc(sizeof(struct mot_pap_msg));
			if (bTypeStop) { pPoleMsg->type = MOT_PAP_TYPE_STOP; }
			else if (bTypeFreeRunStart) { pPoleMsg->type = MOT_PAP_TYPE_FREE_RUNNING; }
			else if (bTypeAutoStart) { pPoleMsg->type = MOT_PAP_TYPE_CLOSED_LOOP; }
			else { lDebug(Info, "Info bSendToPole"); }
			pPoleMsg->free_run_direction = pHMICmd->freeRunDir;
			pPoleMsg->free_run_speed = pHMICmd->velCmdPole;
			pPoleMsg->closed_loop_setpoint = pHMICmd->posCmdPole;
			if (xQueueSend(pole_queue, &pPoleMsg, portMAX_DELAY) == pdPASS) { lDebug(Debug, "Comando enviado a pole.c exitoso!"); }
			else { lDebug(Debug, "Comando NO PUDO ser enviado a pole.c"); }
		}
		if(bSendToLift)
		{
			pLiftMsg = (struct lift_msg*)pvPortMalloc(sizeof(struct lift_msg));
			if (bTypeStop) { pLiftMsg->type = LIFT_TYPE_STOP; }
			else if (bTypeLiftUp) { pLiftMsg->type = LIFT_TYPE_UP; }
			else if (bTypeLiftDown) { pLiftMsg->type = LIFT_TYPE_DOWN; }
			else { lDebug(Info, "Info bSendToLift"); }
			if (xQueueSend(lift_queue, &pLiftMsg, portMAX_DELAY) == pdPASS) { lDebug(Debug, "Comando enviado a lift.c exitoso!"); }
			else { lDebug(Debug, "Comando NO PUDO ser enviado a lift.c"); }
		}
		

	}
	
	ucPreviousFlagByte = ucActualFlagByte;

	return;
}
/*-----------------------------------------------------------*/
