/*********************************************************************************************
PROJECT  : LoRaWAN ESP32 Gateway V1.x 
 
FILE     : lorawan_esp32_gw.c
 
AUTHOR   : F.Fargon 
 
PURPOSE  : Main program (entry point). 
           Initializes the interrupt driven event loop.
 
FEATURES : 
                  
COMMENTS : This program is designed for execution on ESP32 Module (Dev.C kit). 
           The LoRa radio is implemented with Semtech SX1276 chip (Modtronix inAir9 module)  
           The implementation uses the Espressif IDF V3.0 framework (with RTOS)
*********************************************************************************************/


/*********************************************************************************************
  Espressif framework includes
*********************************************************************************************/

#include <Common.h>
#include <ServerManagerItf.h>


/*********************************************************************************************
  Include for module implementation
*********************************************************************************************/

#include "Version.h"
#include "esp_spi_flash.h"
//#include "SX1276Itf.h"
#include "LoraNodeManagerItf.h"
#include "LoraServerManagerItf.h"

/****************************************************************************** 
  Forward declaration
*******************************************************************************/


/****************************************************************************** 
  Implementation 
*******************************************************************************/


// Debug : global variables

ITransceiverManager g_pTransceiverManagerItf = NULL;
IServerManager g_pServerManagerItf = NULL;

TaskHandle_t g_PacketForwarderTask = NULL;

//ILoraTransceiver g_pLoraTransceiverItf = NULL;
//QueueHandle_t g_hEventQueue = NULL;
//CLoraTransceiverItf_EventOb g_Event;




// Test task for LoraNodeManager interface debug
// This task simulates the PacketForwarder task receiving uplink packets
void test_task(void *pvParameter)
{
  CServerManagerItf_LoraSessionPacket pLoraSessionPacket;
  CTransceiverManagerItf_SessionEventOb SessionEvent;



  // Attach the PacketForwarder
//printf("Calling ITransceiverManager_Attach\n");
//
//CTransceiverManagerItf_AttachParamsOb AttachParams;
//AttachParams.m_hPacketForwarderTask = xTaskGetCurrentTaskHandle();
//
//ITransceiverManager_Attach(g_pTransceiverManagerItf, &AttachParams);
//
//printf("Return from ITransceiverManager_Attach\n");


  // Start the LoraNodeManager (i.e. receive packets from Nodes)
  printf("Calling ITransceiverManager_Start\n");

  CTransceiverManagerItf_StartParamsOb StartParams;
  StartParams.m_bForce = false;

  ITransceiverManager_Start(g_pTransceiverManagerItf, &StartParams);

  printf("Return from ITransceiverManager_Start\n");


  // Start the LoraServerManager (i.e. transmit packets to network server)
  printf("Calling ITransceiverManager_Start\n");

  CServerManagerItf_StartParamsOb ServerStartParams;
  StartParams.m_bForce = false;

  IServerManager_Start(g_pServerManagerItf, &ServerStartParams);

  printf("Return from IServerManager_Start\n");


  while(1) 
  {
    // Wait for notification from LoraNodeManager (packet received)
    if (xTaskNotifyWait(0, 0xFFFFFFFF, &pLoraSessionPacket, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      printf("Test Task : Packet received\n");

      // Notify LoraNodeManager that packet is forwarded
//    SessionEvent.m_wEventType = TRANSCEIVERMANAGER_SESSIONEVENT_UPLINK_SENT;
//    SessionEvent.m_pSession = pLoraSessionPacket->m_pSession;
//    SessionEvent.m_dwSessionId = pLoraSessionPacket->m_dwSessionId;
//    ITransceiverManager_SessionEvent(g_pTransceiverManagerItf, &SessionEvent);

    }
  }
  

/*

  // Set continuous receive mode and wait for packets
  CLoraTransceiverItf_ReceiveParamsOb ReceiveParams;
  ReceiveParams.m_bForce = false;
  ILoraTransceiver_Receive(g_pLoraTransceiverItf, &ReceiveParams);

  // Back to standby
//CLoraTransceiverItf_StandByParamsOb StandByParams;
//StandByParams.m_bForce = false;
//ILoraTransceiver_StandBy(g_pLoraTransceiverItf, &StandByParams);


  CLoraTransceiverItf_LoraPacket pPacketToSend = NULL;
  CLoraTransceiverItf_SendParamsOb SendParams;

  while(1) 
  {
    if (xQueueReceive(g_hEventQueue, &g_Event, pdMS_TO_TICKS(50)) == pdPASS)
    {
      // Process event
      if (g_Event.m_wEventType == LORATRANSCEIVERITF_EVENT_PACKETRECEIVED)
      {
        printf("Test Task : Packet received, length: %d\n", ((CLoraTransceiverItf_LoraPacket) (g_Event.m_pEventData))->m_dwDataSize);

        pPacketToSend = pvPortMalloc(sizeof(CLoraTransceiverItf_LoraPacketOb) + ((CLoraTransceiverItf_LoraPacket) (g_Event.m_pEventData))->m_dwDataSize);
        pPacketToSend->m_dwTimestamp = 0;
        pPacketToSend->m_dwDataSize = ((CLoraTransceiverItf_LoraPacket) (g_Event.m_pEventData))->m_dwDataSize;
        for (int i = 0; i < pPacketToSend->m_dwDataSize; i++)
        {
          pPacketToSend->m_usData[i] = ((CLoraTransceiverItf_LoraPacket) (g_Event.m_pEventData))->m_usData[i];
        }

        // Set packet read semaphore
        ((CLoraTransceiverItf_LoraPacket) (g_Event.m_pEventData))->m_dwDataSize = 0;

        printf("Test Task : Sending packet\n");

        SendParams.m_pPacketToSend = pPacketToSend;
        ILoraTransceiver_Send(g_pLoraTransceiverItf, &SendParams);
      }
      else if (g_Event.m_wEventType == LORATRANSCEIVERITF_EVENT_PACKETSENT)
      {
        printf("Test Task : Notified Packet sent\n");
        vPortFree(pPacketToSend);

        printf("Test Task : Activating receive mode\n");
        ILoraTransceiver_Receive(g_pLoraTransceiverItf, &ReceiveParams);
      }

    }
    printf("Test Task excuting\n");
  }

*/


/*
  CSX1276 * pSX1276 = CSX1276_New(0);
  
  // Check SPI interface
  CSX1276_ON(pSX1276);

  // Check Receive Packet Lora
  CSX1276_setCR(pSX1276, CR_5);
  CSX1276_setSF(pSX1276, SF_7);
  CSX1276_setBW(pSX1276, BW_125);
  CSX1276_setChannel(pSX1276, CH_18_868);
  CSX1276_setPreambleLength(pSX1276, 8);
//  CSX1276_setCRC_OFF(pSX1276);
//  CSX1276_setHeaderON(pSX1276);

//  CSX1276_setSyncWord(pSX1276, 0x34);
//  CSX1276_getSyncWord(pSX1276);

  CSX1276_getMode(pSX1276);

//CSX1276_cadDetected(pSX1276);

//CSX1276_availableData2(pSX1276, 180000);

CSX1276_receiveAll2(pSX1276, 180000);

  while(1) 
  {
    // Idle loop = debug tests called before entering task main loop
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
 
*/
   
}



/*********************************************************************************************
FUNCTION  : void app_main(void)

ARGUMENTS : None.

RETURN    : None.

PURPOSE   : User application entry point. 
            The function starts the application tasks and enters the event driven loop.
 
COMMENTS  : None. 
*********************************************************************************************/
void app_main()
{
  printf("LoRaWAN Gateway version:%s\n", g_szVersionString);

  /* Print chip information */
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
          chip_info.cores,
          (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
          (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

  printf("silicon revision %d, ", chip_info.revision);

  printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
          (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");



  // configMINIMAL_STACK_SIZE = 768 -> stack overflow !!!!
  //xTaskCreate(&test_task, "test_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);


  // Create the LoraNodeManager

  printf("Calling CLoraNodeManager_CreateInstance\n");

  g_pTransceiverManagerItf = CLoraNodeManager_CreateInstance(1);

  printf("Return from CLoraNodeManager_CreateInstance\n");

  // Create the LoraServerManager

  printf("Calling CLoraServerManager_CreateInstance\n");

  g_pServerManagerItf = CLoraServerManager_CreateInstance(1, 0, SERVERMANAGER_PROTOCOL_SEMTECH);

  printf("Return from CLoraServerManager_CreateInstance\n");


  // Initialize the LoraNodeManager

  printf("Calling ITransceiverManager_Initialize\n");

  CTransceiverManagerItf_InitializeParamsOb InitializeParams;
  InitializeParams.m_pServerManagerItf = g_pServerManagerItf;
  InitializeParams.m_bUseBuiltinSettings = true;

  ITransceiverManager_Initialize(g_pTransceiverManagerItf, &InitializeParams);

  printf("Return from ITransceiverManager_Initialize\n");


  // Initialize the LoraServerManager

  printf("Calling IServerManager_Initialize\n");

//CServerManagerItf_LoraServerSettingsOb LoraServerSettings;
//strcpy(LoraServerSettings.ConnectorSettings[0].m_szNetworkName, "");
//strcpy(LoraServerSettings.ConnectorSettings[0].m_szNetworkPassword, "");
//strcpy(LoraServerSettings.ConnectorSettings[0].m_szNetworkUser, "");
//
//LoraServerSettings.m_wNetworkServerProtocol = SERVERMANAGER_PROTOCOL_SEMTECH;
//strcpy(LoraServerSettings.m_szNetworkServerUrl, "");
//strcpy(LoraServerSettings.m_szNetworkServerUser, "");
//strcpy(LoraServerSettings.m_szNetworkServerPassword, "");

  CServerManagerItf_InitializeParamsOb InitializeServerParams;
  InitializeServerParams.m_bUseBuiltinSettings = true;
//InitializeServerParams.pLoraServerSettings = &LoraServerSettings;
  InitializeServerParams.pTransceiverManagerItf = g_pTransceiverManagerItf;

  IServerManager_Initialize(g_pServerManagerItf, &InitializeServerParams);

  printf("Return from IServerManager_Initialize\n");



//// Attach the PacketForwarder
//printf("Calling ITransceiverManager_Attach\n");
//
//CTransceiverManagerItf_AttachParamsOb AttachParams;
//AttachParams.m_hPacketForwarderTask = NULL;
//
//ITransceiverManager_Attach(g_pTransceiverManagerItf, &AttachParams);
//
//printf("Return from ITransceiverManager_Attach\n");
//
//
//// Start the LoraNodeManager (i.e. receive packets from Nodes)
//printf("Calling ITransceiverManager_Start\n");
//
//CTransceiverManagerItf_StartParamsOb StartParams;
//StartParams.m_bForce = false;
//
//ITransceiverManager_Start(g_pTransceiverManagerItf, &StartParams);
//
//printf("Return from ITransceiverManager_Start\n");




/* Test_APP_1 = direct use of CSX1276 

  // Initialize CSX1276

  printf("Calling CSX1276_CreateInstance");

  g_pLoraTransceiverItf = CSX1276_CreateInstance();

  printf("Return from CSX1276_CreateCSX1276Instance");

  g_hEventQueue = xQueueCreate(5, sizeof(CLoraTransceiverItf_EventOb));

  CLoraTransceiverItf_InitializeParamsOb InitializeParams;

  InitializeParams.m_hEventNotifyQueue = g_hEventQueue;
  InitializeParams.pLoraMAC = NULL;
  InitializeParams.pLoraMode = NULL;
  InitializeParams.pPowerMode = NULL;
  InitializeParams.pFreqChannel = NULL;

  printf("Calling ILoraTransceiver_Initialize");

  ILoraTransceiver_Initialize(g_pLoraTransceiverItf, &InitializeParams);

  printf("Return from ILoraTransceiver_Initialize");

  // TO DO -> full configuration
  CLoraTransceiverItf_SetLoraModeParamsOb SetLoraModeParams;
  SetLoraModeParams.m_bForce = false;
  SetLoraModeParams.m_usBandwidth = LORATRANSCEIVERITF_BANDWIDTH_125;
  SetLoraModeParams.m_usCodingRate = LORATRANSCEIVERITF_CR_5;
  SetLoraModeParams.m_usSpreadingFactor = LORATRANSCEIVERITF_SF_7;
  SetLoraModeParams.m_usLoraMode = LORATRANSCEIVERITF_LORAMODE_NONE;

  ILoraTransceiver_SetLoraMode(g_pLoraTransceiverItf, &SetLoraModeParams);

  CLoraTransceiverItf_SetFreqChannelParamsOb SetFreqChannelParams;
  SetFreqChannelParams.m_bForce = false;
  SetFreqChannelParams.m_usFreqChannel = LORATRANSCEIVERITF_FREQUENCY_CHANNEL_18;

  ILoraTransceiver_SetFreqChannel(g_pLoraTransceiverItf, &SetFreqChannelParams);

  CLoraTransceiverItf_SetPowerModeParamsOb SetPowerModeParams;
  SetPowerModeParams.m_bForce = false;
  SetPowerModeParams.m_usPowerMode = LORATRANSCEIVERITF_POWER_MODE_LOW;
  SetPowerModeParams.m_usPowerLevel = LORATRANSCEIVERITF_POWER_LEVEL_NONE;
  SetPowerModeParams.m_usOcpRate = LORATRANSCEIVERITF_OCP_NONE;

  ILoraTransceiver_SetPowerMode(g_pLoraTransceiverItf, &SetPowerModeParams);

*/

  // Start task to process events
  xTaskCreate(&test_task, "test_task", 3072, NULL, 5, &g_PacketForwarderTask);

}
