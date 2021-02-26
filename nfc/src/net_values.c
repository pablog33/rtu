/* SM13 Remote Terminal Unit -RTU- Prototype TCP Server
  Automatically connects to HMI running on remote client */


  /* SM13 Remote Terminal Unit -RTU- Prototype TCP Server
	Automatically connects to HMI running on remote client */



	/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>


/*	RTUcomHMI	*/
#include "lwip/err.h"

/*	Motor Control	*/
#include "debug.h"
#include "lift.h"
#include "mot_pap.h"
#include "pole.h"
#include "arm.h"

#include "rtu_com_hmi.h"

static uint16_t prvFormatoTramaRecv(uint16_t uiLenDataRecv);

void NetValuesToSendFromRTU(int16_t iServerStatus, RTUData_t *pRTUDataTx)
{
	/*	Obtengo los valores desde el driver correspondiente a cada eje -Arm, Pole, LIFT-	*/
	struct mot_pap *pPoleStatus = pole_get_status();
	struct mot_pap *pArmStatus= arm_get_status();
	struct lift *pLiftStatus= lift_status_get();

	pRTUDataTx->posActArm = pArmStatus->posAct;
	pRTUDataTx->posActPole = pPoleStatus->posAct;
	pRTUDataTx->velActArm = pArmStatus->freq;
	pRTUDataTx->velActPole = pPoleStatus->freq;

	/*	-- cwLimitArm --	*/
	if (pArmStatus->cwLimitReached)	{	sprintf(pRTUDataTx->cwLimitArm, "%s", "ACW_LIM;");	}
	else {	sprintf(pRTUDataTx->cwLimitArm, "%s", "ACW_RUN;");	}

	/*	-- ccwLimitArm --	*/
	if (pArmStatus->ccwLimitReached)	{	sprintf(pRTUDataTx->ccwLimitArm, "%s", "ACC_LIM;");	}
	else {	sprintf(pRTUDataTx->ccwLimitArm, "%s", "ACC_RUN;");	}

	/*	-- cwLimitPole --	*/
	if (pPoleStatus->cwLimitReached)	{	sprintf(pRTUDataTx->cwLimitPole, "%s", "PCW_LIM;");	}
	else {	sprintf(pRTUDataTx->cwLimitPole, "%s", "PCW_RUN;");	}

	/*	-- ccwLimitPole --	*/
	if (pPoleStatus->ccwLimitReached)	{	sprintf(pRTUDataTx->ccwLimitPole, "%s", "PCC_LIM;");	}
	else {	sprintf(pRTUDataTx->ccwLimitPole, "%s", "PCC_RUN;");	}

	/*	-- limitUp --	*/
	if (pLiftStatus->upLimit)	{	sprintf(pRTUDataTx->limitUp, "%s", "LUP_LIM;");	}
	else {	sprintf(pRTUDataTx->limitUp, "%s", "LUP_RUN;");	}

	/*	-- limitDown --	*/
	if (pLiftStatus->downLimit)	{	sprintf(pRTUDataTx->limitDown, "%s", "LDW_LIM;");	}
	else {	sprintf(pRTUDataTx->limitDown, "%s", "LDW_RUN;");	}

	/*	-- stallAlm --	*/
	if (pArmStatus->stalled||pPoleStatus->stalled)	{	sprintf(pRTUDataTx->stallAlm, "%s", "STL_ALM;");	}
	else {	sprintf(pRTUDataTx->stallAlm, "%s", "STL_RUN;");	}

	/*	-- status --	*/
	if (iServerStatus) { pRTUDataTx->status = iServerStatus; }
	else { pRTUDataTx->status = 0x00; }


	snprintf(pRTUDataTx->buffer, 100, "%d %d %d %d %s %s %s %s %s %s %s %d ",
	pRTUDataTx->posActArm, pRTUDataTx->posActPole, pRTUDataTx->velActArm, pRTUDataTx->velActPole,
	pRTUDataTx->cwLimitArm, pRTUDataTx->ccwLimitArm, pRTUDataTx->cwLimitPole, pRTUDataTx->ccwLimitPole,
	pRTUDataTx->limitUp, pRTUDataTx->limitDown, pRTUDataTx->stallAlm, pRTUDataTx->status);

	return;
}
/*-----------------------------------------------------------*/
int16_t NetValuesReceivedFromHMI(HMIData_t *HMIData, HMICmd_t *HMICmd, uint16_t uiLenDataRecv)
{	/*	HMI_NETVAR_SIZE + 1: Para tener en cuenta el fin de cadena en caracteres '\0' 	*/

	uint16_t iServerStatus = ERR_OK;

	if( (iServerStatus = prvFormatoTramaRecv(uiLenDataRecv)) == ERR_OK )
	{
		/*	-- clientID --*/
			if (!strncmp(HMIData->clientId, "SM13;", HMI_NETVAR_SIZE)) { HMICmd->clientID = eSigned; }
			else { lDebug(Error,"error- HMICmd->ClientID", HMI_NETVAR_SIZE); HMICmd->clientID = eUnsigned; iServerStatus = ERROR_TRAMA_CLIENTE; }

			if(!iServerStatus)
			{
			/*	Se asignan en forma directa los valores enteros correspondientes a objetivos para los resolvers	*/
				HMICmd->posCmdArm = HMIData->posCmdArm;
				HMICmd->velCmdArm = HMIData->velCmdArm;
				HMICmd->posCmdPole = HMIData->posCmdPole;
				HMICmd->velCmdPole = HMIData->velCmdPole;

				if (HMICmd->velCmdArm > 8 || HMICmd->velCmdArm < 0 ) {	lDebug(Error,"error- HMICmd->velCmdArm"); iServerStatus = ERROR_TRAMA_DATO;	} /* 0x82 */
				if (HMICmd->velCmdPole > 8 || HMICmd->velCmdPole < 0 ) { lDebug(Error,"error- HMICmd->velCmdPole"); iServerStatus = ERROR_TRAMA_DATO;	} /* 0x82 */

				/*		-- HMICmd Frame Parsing --	*/
				/*	A partir de los descriptores -*HMIData- se obtienen los valores para HMICmd contenidos en la trama recibida.
				*/

				/*	-- mode --	*/
				if(!strncmp(HMIData->mode, "STOP;", HMI_NETVAR_SIZE))	{	HMICmd->mode = eStop;	}
				else if (!strncmp(HMIData->mode, "FRUN;", HMI_NETVAR_SIZE)) {	HMICmd->mode = eFree_run;	}
				else if (!strncmp(HMIData->mode, "AUTO;", HMI_NETVAR_SIZE)) {	HMICmd->mode = eAuto; }
				else if (!strncmp(HMIData->mode, "LIFT;", HMI_NETVAR_SIZE)) { HMICmd->mode = eLift; }
				else { lDebug(Error,"error- HMICmd->mode"); iServerStatus = ERROR_TRAMA_DATO; }

				/*	--	freeRunAxis --	*/
				if (!strncmp(HMIData->freeRunAxis, "ARM_;", HMI_NETVAR_SIZE)) { HMICmd->freeRunAxis = eArm; }
				else if (!strncmp(HMIData->freeRunAxis, "POLE;", HMI_NETVAR_SIZE)) { HMICmd->freeRunAxis = ePole; }
				else { lDebug(Error,"error- HMICmd->freeRunAxis"); iServerStatus = ERROR_TRAMA_DATO; }

				/*	--	freeRunDir --	*/
				if (!strncmp(HMIData->freeRunDir, "CW__;", HMI_NETVAR_SIZE)) { HMICmd->freeRunDir = eCW; }
				else if (!strncmp(HMIData->freeRunDir, "CCW_;", HMI_NETVAR_SIZE)) { HMICmd->freeRunDir = eCCW; }
				else { lDebug(Error,"error- HMICmd->freeRunDir"); iServerStatus = ERROR_TRAMA_DATO; }

				/*	-- ctrlEn --	*/
				if (!strncmp(HMIData->ctrlEn, "CTLE;", HMI_NETVAR_SIZE)) { HMICmd->ctrlEn = eEnable; }
				else if (!strncmp(HMIData->ctrlEn, "DCTL;", HMI_NETVAR_SIZE)) { HMICmd->ctrlEn = eDesable; }
				else { lDebug(Error,"error- HMICmd->ctrlEn"); iServerStatus = ERROR_TRAMA_DATO; }

				/*	-- stallEn --	*/
				if (!strncmp(HMIData->stallEn, "STLE;", HMI_NETVAR_SIZE)) { HMICmd->stallEn = eEnable; }
				else if (!strncmp(HMIData->stallEn, "DSTL;", HMI_NETVAR_SIZE)) { HMICmd->stallEn = eDesable; }
				else { lDebug(Error,"error- HMICmd->stallEn"); iServerStatus = ERROR_TRAMA_DATO; }

				/*	-- liftDir --	*/
				if (!strncmp(HMIData->liftDir, "LFUP;", HMI_NETVAR_SIZE)) { HMICmd->liftDir = eUp; }
				else if (!strncmp(HMIData->liftDir, "LFDW;", HMI_NETVAR_SIZE)) { HMICmd->liftDir = eDown; }
				else { lDebug(Error,"error- HMICmd->liftDir"); iServerStatus = ERROR_TRAMA_DATO; }

			} /* if -ClienteID */

	} /* if-formato_trama */

	return iServerStatus;
}

static uint16_t prvFormatoTramaRecv(uint16_t uiLenDataRecv)
{
	uint16_t iStatus = ERR_OK;

	if( uiLenDataRecv <= 0 )
	{
		iStatus = ERROR_TRAMA_VACIA; /* 0x81 */
		lDebug( Error, " - Se recibio trama vacia - ");
	}
	else if( uiLenDataRecv != RCV_TRAMA_LARGO )
	{
		iStatus = ERROR_TRAMA_LARGO;	/* 0x82	Error: Formato de trama incorrecto!!	*/
		lDebug(Error, "Error - Formato de trama incorrecto!!");
	}

	return iStatus;
}
