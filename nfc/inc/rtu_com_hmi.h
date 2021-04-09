#ifndef RTU_COM_HMI_H
#define RTU_COM_HMI_H

/* SM13 Remote Terminal Unit -RTU- Prototype TCP Server
  Automatically connects to HMI running on remote client */

  /* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* LWIP includes */
#include "lwip/err.h"
/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


/**
 * @def Puerto de conexion del RTU - socket
 */
#define PORT_NUMBER		5020
/**
 * @def Timeout -ms- para intervalo entre ciclos de recepcion y transmision.
 * @brief Espera de nueva trama desde HMI dentro de este intrvalo de tiempo.
 * @note Al generarse timeout, se produce la desconexion por parte de la RTU.
 */
#define	RCV_TIMEO		1000

#define RCV_TRAMA_LARGO	51

/**
* @def ERROR_SM13
* @brief Variable iServerStatus error identifiers
* @note Handle general spec error events
*/
#define ERROR_TRAMA_VACIA 		0x81
#define ERROR_TRAMA_LARGO 		0x82
#define ERROR_TRAMA_DATO 		0x83
#define ERROR_TRAMA_CLIENTE 	0x84
#define ERROR_NETCONN 			0x85




void stackIp_ThreadInit(void);
													 


/*	----------------------------------------------------------------------------------------- */
/*	----------------------------	-	From HMI	-	------------------------------------- */
/*	----------------------------------------------------------------------------------------- */
/**
* @enum	Tamaño de variables de red -NETVAR- recibidos en tramas desde la aplicacion HMI.
* @brief 	Corresponde a datos utilizados en tramas ethernet recibidas desde la aplicacion HMI.
*/
enum { HMI_NETVAR_SIZE = 5 };

/**
 * @struct 	HMIData
 * @brief	Descriptor del bufer de recepcion.
																  
 */
typedef struct HMIDATA
{
	uint16_t posCmdArm;
	uint16_t posCmdPole;
	uint8_t velCmdArm;
	uint8_t velCmdPole;
	char mode[HMI_NETVAR_SIZE];			/*	-- mode --			STOP; FRUN; AUTO; LIFT; */
	char freeRunAxis[HMI_NETVAR_SIZE];	/*	-- freeRunAxis	--	POLE; ARM_;				*/
	char freeRunDir[HMI_NETVAR_SIZE];	/*	-- freeRunDir  --	CW__; CCW_;				*/
	char ctrlEn[HMI_NETVAR_SIZE];		/*	-- ctrlEn --		CTLE; DCTL;				*/
	char stallEn[HMI_NETVAR_SIZE];		/*	-- stallEn --		STLE; DSTL;				*/
	char liftDir[HMI_NETVAR_SIZE];		/*	-- liftDir --		LFUP; LFDW;				*/
	char setCal[HMI_NETVAR_SIZE];		/*	-- setCal --		NOP_; CAL_; 			*/
	char clientId[HMI_NETVAR_SIZE];		/*	-- clientId --		SM13;					*/
} HMIData_t;

/**
 * @enum HMICmd
 * @brief Definicion enumerada de los modos de funcionamiento comandados por el HMI.
 */
typedef enum
{
	eStop = 0,
	eFree_run = 1,
	eAuto = 2,
	eLift = 3,

}mode_t;
typedef enum { eArm, ePole } freeRunAxis_t;
typedef enum { eCW, eCCW } freeRunDir_t;
typedef enum { eDisable, eEnable } enable_t;
typedef enum { eDown, eUp } liftDir_t;
typedef enum { eNop, eCal } setCal_t;
typedef enum { eUnsigned, eSigned } sign_t;

/**
 * @struct 	HMICmd
 * @brief	Comandos HMI. Obtenidos y traducidos desde HMIData
 */		  
typedef struct
{
	uint16_t posCmdArm;
	uint16_t posCmdPole;
	uint8_t velCmdArm;
	uint8_t velCmdPole;
	mode_t mode;		
	freeRunAxis_t freeRunAxis;
	freeRunDir_t freeRunDir;
	enable_t ctrlEn;
	enable_t stallEn;
	liftDir_t liftDir;
	setCal_t setCal;
	sign_t clientID;

} HMICmd_t;

int16_t NetValuesReceivedFromHMI(HMIData_t *HMIData, HMICmd_t *HMICmd, uint16_t uLenDataRecv);



/*	----------------------------------------------------------------------------------------- */
/*	----------------------------	-	From RTU	-	------------------------------------- */
/*	----------------------------------------------------------------------------------------- */
/**
* @enum	Tamaño de variables de red -NETVAR- enviados en tramas desde la RTU.
* @brief 	Corresponde a datos utilizados en tramas ethernet enviadas hacia la aplicacion HMI.
*/
enum { RTU_NETVAR_SIZE = 9 };

/**
 * @struct 	RTUData
 * @brief	Estado actual del RTU y sus dispositivos asociados. Datos enviados en
 * 			tramas ethernet hacia aplicacion HMI.
 */
 typedef struct
{
	uint16_t posActArm;
	uint16_t posActPole;
	char stallAlm[RTU_NETVAR_SIZE];
	char onCondition[RTU_NETVAR_SIZE];
	uint8_t armrRdcStatus;
	uint8_t poleRdcStatus;
	uint8_t rtuStatus;
	char buffer[100];

} RTUData_t;



void NetValuesToSendFromRTU(int16_t iServerStatus,RTUData_t* pRTUDataTx);


/*	----------------------------------------------------------------------------------------- */
/*	----------------------------	-	TASK TRIGGER & STATUS HANDLER	-	----------------- */
/*	----------------------------------------------------------------------------------------- */

void TaskTriggerMsg(HMICmd_t* pHMICmd);

void vStackIpSetup(void *pvParameters);

void arm_init();
void pole_init();
void lift_init();

#endif /* RTU_COM_HMI_H */
