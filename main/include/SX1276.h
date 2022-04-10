/*********************************************************************************************
PROJECT : LoRaWAN ESP32 Gateway V1.x
 
FILE    : SX1276.h
 
AUTHOR  : F.Fargon 
 
PURPOSE : This file contains program definitions for Semtech SX1276 low level driver. 

COMMENTS: This program is designed for execution on ESP32 Module (Dev.C kit). 
          The LoRa radio is implemented with Semtech SX1276 chip (Modtronix inAir9 module)  
          The implementation uses the Espressif IDF V3.0 framework (with RTOS)
*********************************************************************************************/

#ifndef SX1276_H_
#define SX1276_H_

/*********************************************************************************************
  Includes
*********************************************************************************************/

#include "driver/spi_master.h"


/********************************************************************************************* 
  Definitions for debug traces
  The debug level is specified with 'SX1276_DEBUG_LEVEL' in Definitions.h file
*********************************************************************************************/

#define SX1276_DEBUG_LEVEL0 ((SX1276_DEBUG_LEVEL & 0x01) > 0)
#define SX1276_DEBUG_LEVEL1 ((SX1276_DEBUG_LEVEL & 0x02) > 0)
#define SX1276_DEBUG_LEVEL2 ((SX1276_DEBUG_LEVEL & 0x04) > 0)


/********************************************************************************************* 
  Definitions for ESP32 PIN used for SPI with Semtech SX1276 chip
*********************************************************************************************/

#define PIN_NUM_MISO       25
#define PIN_NUM_MOSI       23
#define PIN_NUM_CLK        19
#define PIN_NUM_CS         22       // TO DO : Single SX1276 
#define PIN_NUM_RX_TX_IRQ  02       // Receive and sent IRQ on same PIN (i.e. software configuration
                                    // of DIO mapping on SX1276)
                                    // TO DO : Single SX1276 


/********************************************************************************************* 
  Definitions for Semtech SX1276 chip and LoRa specification
*********************************************************************************************/

// SX1276 Registers

#define REG_FIFO                    0x00
#define REG_OP_MODE                 0x01
#define REG_BITRATE_MSB             0x02
#define REG_BITRATE_LSB             0x03
#define REG_FDEV_MSB                0x04
#define REG_FDEV_LSB                0x05
#define REG_FRF_MSB                 0x06
#define REG_FRF_MID                 0x07
#define REG_FRF_LSB                 0x08
#define REG_PA_CONFIG               0x09
#define REG_PA_RAMP                 0x0A
#define REG_OCP                     0x0B
#define REG_LNA                     0x0C
#define REG_RX_CONFIG               0x0D
#define REG_FIFO_ADDR_PTR           0x0D
#define REG_RSSI_CONFIG             0x0E
#define REG_FIFO_TX_BASE_ADDR       0x0E
#define REG_RSSI_COLLISION          0x0F
#define REG_FIFO_RX_BASE_ADDR       0x0F
#define REG_RSSI_THRESH             0x10
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_RSSI_VALUE_FSK          0x11
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_RX_BW                   0x12
#define REG_IRQ_FLAGS               0x12
#define REG_AFC_BW                  0x13
#define REG_RX_NB_BYTES             0x13
#define REG_OOK_PEAK                0x14
#define REG_RX_HEADER_CNT_VALUE_MSB 0x14
#define REG_OOK_FIX                 0x15
#define REG_RX_HEADER_CNT_VALUE_LSB 0x15
#define REG_OOK_AVG                 0x16
#define REG_RX_PACKET_CNT_VALUE_MSB 0x16
#define REG_RX_PACKET_CNT_VALUE_LSB 0x17
#define REG_MODEM_STAT              0x18
#define REG_PKT_SNR_VALUE           0x19
#define REG_AFC_FEI                 0x1A
#define REG_PKT_RSSI_VALUE          0x1A
#define REG_AFC_MSB                 0x1B
#define REG_RSSI_VALUE_LORA         0x1B
#define REG_AFC_LSB                 0x1C
#define REG_HOP_CHANNEL             0x1C
#define REG_FEI_MSB                 0x1D
#define REG_MODEM_CONFIG1           0x1D
#define REG_FEI_LSB                 0x1E
#define REG_MODEM_CONFIG2           0x1E
#define REG_PREAMBLE_DETECT         0x1F
#define REG_SYMB_TIMEOUT_LSB        0x1F
#define REG_RX_TIMEOUT1             0x20
#define REG_PREAMBLE_MSB_LORA       0x20
#define REG_RX_TIMEOUT2             0x21
#define REG_PREAMBLE_LSB_LORA       0x21
#define REG_RX_TIMEOUT3             0x22
#define REG_PAYLOAD_LENGTH_LORA     0x22
#define REG_RX_DELAY                0x23
#define REG_MAX_PAYLOAD_LENGTH      0x23
#define REG_OSC                     0x24
#define REG_HOP_PERIOD              0x24
#define REG_PREAMBLE_MSB_FSK        0x25
#define REG_FIFO_RX_BYTE_ADDR       0x25
#define REG_PREAMBLE_LSB_FSK        0x26
#define REG_MODEM_CONFIG3           0x26
#define REG_SYNC_CONFIG             0x27
#define REG_SYNC_VALUE1             0x28
#define REG_SYNC_VALUE2             0x29
#define REG_SYNC_VALUE3             0x2A
#define REG_SYNC_VALUE4             0x2B
#define REG_SYNC_VALUE5             0x2C
#define REG_SYNC_VALUE6             0x2D
#define REG_SYNC_VALUE7             0x2E
#define REG_SYNC_VALUE8             0x2F
#define REG_PACKET_CONFIG1          0x30
#define REG_PACKET_CONFIG2          0x31
#define REG_DETECT_OPTIMIZE         0x31
#define REG_PAYLOAD_LENGTH_FSK      0x32
#define REG_NODE_ADRS               0x33
#define REG_BROADCAST_ADRS          0x34
#define REG_FIFO_THRESH             0x35
#define REG_SEQ_CONFIG1             0x36
#define REG_SEQ_CONFIG2             0x37
#define REG_DETECTION_THRESHOLD     0x37
#define REG_TIMER_RESOL             0x38
#define REG_TIMER1_COEF             0x39
#define REG_SYNC_WORD               0x39
#define REG_TIMER2_COEF             0x3A
#define REG_IMAGE_CAL               0x3B
#define REG_TEMP                    0x3C
#define REG_LOW_BAT                 0x3D
#define REG_IRQ_FLAGS1              0x3E
#define REG_IRQ_FLAGS2              0x3F
#define REG_DIO_MAPPING1            0x40
#define REG_DIO_MAPPING2            0x41
#define REG_VERSION                 0x42
#define REG_AGC_REF                 0x43
#define REG_AGC_THRESH1             0x44
#define REG_AGC_THRESH2             0x45
#define REG_AGC_THRESH3             0x46
#define REG_PLL_HOP                 0x4B
#define REG_TCXO                    0x58
#define REG_PA_DAC                  0x5A
#define REG_PLL                     0x5C
#define REG_PLL_LOW_PN              0x5E
#define REG_FORMER_TEMP             0x6C
#define REG_BIT_RATE_FRAC           0x70

// SX1276 LoRa Modes
#define LORA_SLEEP_MODE             0x80
#define LORA_STANDBY_MODE           0x81
#define LORA_TX_MODE                0x83
#define LORA_RX_MODE                0x85
#define LORA_STANDBY_FSK_REGS_MODE  0xC1

#define FSK_SLEEP_MODE              0x00        // Sleep mode for FSK modulation


// Register values for LoRa Frequency Channels
// Note: MUST be adjusted accordingly to 'LORATRANSCEIVERITF_FREQUENCY_CHANNEL_xx' in 'LoraTransceiverIth.h'
// Note: Formula, cf. p36 LoRaWAN specification
//        - Fxosc = 32000000
//        - Freq Step = 61.03515625
//        - Register Value = Freq TX Lora / Freq Step
#define SX1276_REG_CH_10_868      0xD84CCC   // channel 10, central freq = 865.20MHz
#define SX1276_REG_CH_11_868      0xD86000   // channel 11, central freq = 865.50MHz
#define SX1276_REG_CH_12_868      0xD87333   // channel 12, central freq = 865.80MHz
#define SX1276_REG_CH_13_868      0xD88666   // channel 13, central freq = 866.10MHz
#define SX1276_REG_CH_14_868      0xD89999   // channel 14, central freq = 866.40MHz
#define SX1276_REG_CH_15_868      0xD8ACCC   // channel 15, central freq = 866.70MHz
#define SX1276_REG_CH_16_868      0xD8C000   // channel 16, central freq = 867.00MHz
#define SX1276_REG_CH_17_868      0xD90000   // channel 16, central freq = 868.00MHz
#define SX1276_REG_CH_18_868      0xD90666   // 868.1MHz for LoRaWAN test
                                            

// EU868 Standard 6 channel frequency plan
#define SX1276_REG_CH_00_868      0xD90666   // channel 0, central freq = 868.100MHz (DR0-5) (SF7) (125 kHz) (default LoRaWAN EU channel)
#define SX1276_REG_CH_01_868      0xD91333   // channel 1, central freq = 868.300MHz (DR0-5) (SF7) (125 kHz) (default LoRaWAN EU channel)
#define SX1276_REG_CH_02_868      0xD92000   // channel 2, central freq = 868.500MHz (DR0-5) (SF7) (125 kHz) (default LoRaWAN EU channel)
#define SX1276_REG_CH_03_868      0xD93666   // channel 3, central freq = 868.850MHz (DR0-5) (SF7) (125 kHz)
#define SX1276_REG_CH_04_868      0xD94333   // channel 4, central freq = 869.050MHz (DR0-5) (SF7) (125 kHz)
#define SX1276_REG_CH_05_868      0xD9619A   // channel 5, central freq = 869.525MHz (DR0-5) (SF7) (125 kHz)

#define SX1276_REG_CH_RX2_868     0xD9619A   // Downlink RX2, central freq = 869.525MHz (DR0) (SF12) (125 kHz) (default LoRaWAN EU RX2)


#define SX1276_REG_CH_UNDEFINED   0x000000   // Register value undefined

#define SX1276_FREQ_CH_UNDEFINED  0x00       // Frequency channel not defined (i.e. predefined channel No.)
                                             // See 'LoraTransceiverItf.h' for other values

// Frequency in text format for predefined channel No. (see 'LoraTransceiverItf.h')
// MUST be adjusted when 'SX1276_REG_CH_xx' is changed 
#define SX1276_FREQ_TEXT_CH_00    "868.100"
#define SX1276_FREQ_TEXT_CH_01    "868.300"
#define SX1276_FREQ_TEXT_CH_02    "868.500"
#define SX1276_FREQ_TEXT_CH_03    "868.850"
#define SX1276_FREQ_TEXT_CH_04    "869.050"
#define SX1276_FREQ_TEXT_CH_05    "869.525"
                                 
#define SX1276_FREQ_TEXT_CH_10    "865.200"
#define SX1276_FREQ_TEXT_CH_11    "865.500"
#define SX1276_FREQ_TEXT_CH_12    "865.800"
#define SX1276_FREQ_TEXT_CH_13    "866.100"
#define SX1276_FREQ_TEXT_CH_14    "866.400"
#define SX1276_FREQ_TEXT_CH_15    "866.700"
#define SX1276_FREQ_TEXT_CH_16    "867.000"
#define SX1276_FREQ_TEXT_CH_17    "868.000"
#define SX1276_FREQ_TEXT_CH_18    "868.100"


// LoRa Bandwidth
#define SX1276_BW_UNDEFINED  0xFF            // Not a defined bandwidth
                                             // See 'LoraTransceiverItf.h' for other values


// LoRa Coding Rate
#define SX1276_CR_UNDEFINED  0x00            // Not a defined coding rate
                                             // See 'LoraTransceiverItf.h' for other values

// Text values for Coding Rate (used by 'CSX1276_getCRTextValue')
#define SX1276_CR_TEXT_CR5    "4/5"
#define SX1276_CR_TEXT_CR6    "4/6"
#define SX1276_CR_TEXT_CR7    "4/7"
#define SX1276_CR_TEXT_CR8    "4/8"


// LoRa Spreading Factor
#define SX1276_SF_UNDEFINED  0x00            // Not a defined spreading factor
                                             // See 'LoraTransceiverItf.h' for other values

#define SX1276_LORAMODE_UNDEFINED  0x00      // Predefined LoRa Mode undefined
                                             // See 'LoraTransceiverItf.h' for other values

#define SX1276_HEADER_UNDEFINED    0xFF
#define SX1276_HEADER_OFF          0x00
#define SX1276_HEADER_ON           0x01
 
#define SX1276_CRC_UNDEFINED       0xFF
#define SX1276_CRC_OFF             0x00
#define SX1276_CRC_ON              0x01

#define SX1276_POWER_LEVEL_UNDEFINED  0xFF

#define SX1276_POWER_MODE_UNDEFINED   0xFF      // See 'LoraTransceiverItf.h' for other values

#define SX1276_POWER_MODE_CUSTOM      0xFE      // Not a predefined mode, custom level applied

#define SX1276_OCP_UNDEFINED     0xFF

#define SX1276_PREAMBLE_LENGTH_UNDEFINED   0xFF

#define SX1276_SYNCWORD_UNDEFINED          0x00

// Copied from LoRaMAC-Node
// RegImageCal
#define RF_IMAGECAL_AUTOIMAGECAL_MASK               0x7F
#define RF_IMAGECAL_AUTOIMAGECAL_ON                 0x80
#define RF_IMAGECAL_AUTOIMAGECAL_OFF                0x00  // Default

#define RF_IMAGECAL_IMAGECAL_MASK                   0xBF
#define RF_IMAGECAL_IMAGECAL_START                  0x40

#define RF_IMAGECAL_IMAGECAL_RUNNING                0x20
#define RF_IMAGECAL_IMAGECAL_DONE                   0x00  // Default

#define RF_IMAGECAL_TEMPCHANGE_HIGHER               0x08
#define RF_IMAGECAL_TEMPCHANGE_LOWER                0x00

#define RF_IMAGECAL_TEMPTHRESHOLD_MASK              0xF9
#define RF_IMAGECAL_TEMPTHRESHOLD_05                0x00
#define RF_IMAGECAL_TEMPTHRESHOLD_10                0x02  // Default
#define RF_IMAGECAL_TEMPTHRESHOLD_15                0x04
#define RF_IMAGECAL_TEMPTHRESHOLD_20                0x06

#define RF_IMAGECAL_TEMPMONITOR_MASK                0xFE
#define RF_IMAGECAL_TEMPMONITOR_ON                  0x00 // Default
#define RF_IMAGECAL_TEMPMONITOR_OFF                 0x01


// Configuration and implementation constants
#define MODEM_MODE_LORA       1
#define MODEM_MODE_UNKNOWN    0

#define OFFSET_RSSI           137
#define NOISE_FIGURE          6.0
#define NOISE_ABSOLUTE_ZERO   174.0

#define CORRECT_PACKET        0
#define INCORRECT_PACKET      1


/********************************************************************************************* 
  Structures 
*********************************************************************************************/


/********************************************************************************************* 
 LoRa packet format
*********************************************************************************************/

// This structure MUST be identical to 'CLoraTransceiverItf_LoraPacketOb'.
// The only difference is that 'm_usData' is a fixed length buffer for 'CLoraPacket' with
// 'MAX_PAYLOAD' bytes (i.e. to be able to receive LoRa packet of any length).
// Typically, the 'm_usData' buffer have 'm_dwDataSize' for 'CLoraTransceiverItf_LoraPacketOb'
// structure (i.e. storage optimization when owner object copies received packet) 
typedef struct _LoraPacket
{
  // System tick count for LoRaWAN protocol time rules (i.e. uplink RX windows)
  // Received packet = when packet is received
  // Packet to send  = when sending packet bytes begins
  DWORD m_dwTimestamp;

  // Packet payload size
  // Note: This member variable is used as synchronization flag for packet transmission to other
  //       object (i.e. the CSX1276 cannot update 'CloraPacket' if 'm_dwDataSize' is not 0
  //       and cannot read 'CloraPacket' if 'm_dwDataSize' is 0).
  //       Due to this concurrent access to 'm_dwDataSize', the type of this variable MUST be
  //       DWORD (i.e. size for atomic access by ESP32 = no MUTEX required)
  DWORD m_dwDataSize;

  // Packet payload
  // Note: This member variable must be at the end of structure
  uint8_t m_usData[LORA_MAX_PAYLOAD_LENGTH];
} CLoraPacket;


/********************************************************************************************* 
 LoRa packet info format
*********************************************************************************************/

// Meta data associated to last received LoraPacket in text format
// 
// IMPORTANT: This structure MUST be identical to 'CLoraTransceiverItf_ReceivedLoraPacketInfoOb'
//            (i.e. direct memcpy to client buffer for optimization)
typedef struct _ReceivedLoraPacketInfo
{
  // RX timestamp (RTC value in UTC)
  DWORD m_dwUTCSec;
  DWORD m_dwUTCMicroSec;

  // RX central frequency in MHz (unsigned float, Hz precision)
  BYTE m_szFrequency[8];

  // LoRa datarate identifier (eg. SF12BW500) 
  BYTE m_szDataRate[10];

  // LoRa coding rate identifier (eg. 4/5) 
  BYTE m_szCodingRate[4];

  // Lora SNR ratio in dB (signed float, 0.1 dB precision)
  BYTE m_szSNR[7];

  // RSSI in dBm (signed integer, 1 dB precision)
  BYTE m_szRSSI[5];
} CReceivedLoraPacketInfo;


/********************************************************************************************* 
 SX1276 Class
*********************************************************************************************/

// Class data
typedef struct _CSX1276
{
  // Interface
  ILoraTransceiver m_pLoraTransceiverItf;

  uint32_t m_nRefCount;      // The 'CSX1276' object is reference counted (number of client 
                             // objects owning one of the public interfaces exposed by 'CSX1276)

  QueueHandle_t m_hEventNotifyQueue;         // Owner object's queue to use for event notification
                                             // Queue items are 'CLoraTransceiverItf_EventOb'

  // Automaton
  DWORD m_dwCurrentState;
  TaskHandle_t m_hAutomatonTask;

  SemaphoreHandle_t m_hCommandMutex;
  SemaphoreHandle_t m_hCommandDone;
  DWORD m_dwCommand;
  void *m_pCommandParams;


  // Private attributes (implementation)

  uint8_t m_usBandwidth;                // Bandwidth configured in LoRa mode
                                        // This variable contains bit value in SX1276 register (REG_MODEM_CONFIG1)
                                        //   bandwidth = 00  --> BW = 125KHz
                                        //   bandwidth = 01  --> BW = 250KHz
                                        //   bandwidth = 10  --> BW = 500KHz
  
  uint8_t m_usCodingRate;               // Coding rate configured in LoRa mode
                                        // This variable contains bit value in SX1276 register (REG_MODEM_CONFIG1)
                                        //   codingRate = 001  --> CR = 4/5
                                        //   codingRate = 010  --> CR = 4/6
                                        //   codingRate = 011  --> CR = 4/7
                                        //   codingRate = 100  --> CR = 4/8

  uint8_t m_usSpreadingFactor;          // Spreading factor configured in LoRa mode.
                                        // This variable contains bit value in SX1276 register (REG_MODEM_CONFIG2)
                                        //   spreadingFactor = 6   --> SF = 6, 64 chips/symbol
                                        //   spreadingFactor = 7   --> SF = 7, 128 chips/symbol
                                        //   spreadingFactor = 8   --> SF = 8, 256 chips/symbol
                                        //   spreadingFactor = 9   --> SF = 9, 512 chips/symbol
                                        //   spreadingFactor = 10  --> SF = 10, 1024 chips/symbol
                                        //   spreadingFactor = 11  --> SF = 11, 2048 chips/symbol
                                        //   spreadingFactor = 12  --> SF = 12, 4096 chips/symbol

  uint8_t m_usFreqChannel;              // Predefined channel No. (i.e. predefined frequency)
                          
  uint32_t m_dwRegFreqChannel;          // Channel frequency configured               
                                        // This variable contains the SX1276 registers values
                                        //   channel = 0xD84CCC  --> CH = 10_868, 865.20MHz
                                        //   channel = 0xD86000  --> CH = 11_868, 865.50MHz
                                        //   channel = 0xD87333  --> CH = 12_868, 865.80MHz
                                        //   channel = 0xD88666  --> CH = 13_868, 866.10MHz
                                        //   channel = 0xD89999  --> CH = 14_868, 866.40MHz
                                        //   channel = 0xD8ACCC  --> CH = 15_868, 866.70MHz
                                        //   channel = 0xD8C000  --> CH = 16_868, 867.00MHz
                                        //   channel = 0xE1C51E  --> CH = 00_900, 903.08MHz
                                        //   channel = 0xE24F5C  --> CH = 01_900, 905.24MHz
                                        //   channel = 0xE2D999  --> CH = 02_900, 907.40MHz
                                        //   channel = 0xE363D7  --> CH = 03_900, 909.56MHz
                                        //   channel = 0xE3EE14  --> CH = 04_900, 911.72MHz
                                        //   channel = 0xE47851  --> CH = 05_900, 913.88MHz
                                        //   channel = 0xE5028F  --> CH = 06_900, 916.04MHz
                                        //   channel = 0xE58CCC  --> CH = 07_900, 918.20MHz
                                        //   channel = 0xE6170A  --> CH = 08_900, 920.36MHz
                                        //   channel = 0xE6A147  --> CH = 09_900, 922.52MHz
                                        //   channel = 0xE72B85  --> CH = 10_900, 924.68MHz
                                        //   channel = 0xE7B5C2  --> CH = 11_900, 926.84MHz
  

  // Predefined LoRa configuration
  uint8_t m_usLoraMode;                 // Mode 1 --> CR = 4/5, SF = 12, BW = 125 KHz   
                                        // Mode 2 --> CR = 4/5, SF = 12, BW = 250 KHz   
                                        // Mode 3 --> CR = 4/5, SF = 10, BW = 11250 KHz 
                                        // Mode 4 --> CR = 4/5, SF = 12, BW = 500 KHz   
                                        // Mode 5 --> CR = 4/5, SF = 10, BW = 250 KHz   
                                        // Mode 6 --> CR = 4/5, SF = 11, BW = 500 KHz   
                                        // Mode 7 --> CR = 4/5, SF = 9,  BW = 250 KHz   
                                        // Mode 8 --> CR = 4/5, SF = 9,  BW = 500 KHz   
                                        // Mode 9 --> CR = 4/5, SF = 8,  BW = 500 KHz   
                                        // Mode 10 --> CR = 4/5, SF = 7,  BW = 500 KHz   
                                        // Mode 11 --> CR = 4/5, SF = 12, BW = 125 KHz   

  // Output power (level in dBm).
  uint8_t m_usPowerLevel;

  // Predefined output power modes
  uint8_t m_usPowerMode;

  // Over Current Protection (max. current).
  uint8_t m_usOcpRate;

  // RSSI current value.
  int8_t m_nRSSI;

  // SNR from the last packet received in LoRa mode.
  int8_t m_nSNRPacket;

  // RSSI from the last packet received in LoRa mode.
  int16_t m_nRSSIPacket;

  // LoRa MAC preamble length
  uint16_t m_wPreambleLength;

  // LoRa MAC synchronization word
  uint8_t m_usSyncWord;

  // Implicit or explicit header in LoRa mode.
  uint8_t m_usHeader;

  // Presence or absence of CRC calculation.
  uint8_t m_usCRC;

  // Number of current retry.
  uint8_t m_usRetries;

  // Maximum number of retries.
  uint8_t m_usMaxRetries;

  // Maximum current supply.
  uint8_t m_usMaxCurrent;

  // Indicates FSK or LoRa modem.
  uint8_t m_usModemMode;

  // Number of received packets successfully transmited to owner
  DWORD m_dwPacketReceivedNumber;

  // Number of received packets not read
  DWORD m_dwMissedPacketReceivedNumber;

  // Number of sent packets successfully notified to owner
  DWORD m_dwPacketSentNumber;

  // Packet currently sent in 'SENDING' state
  // Note: This object is owned by the object which have launched the send operation
  //       (i.e. 'Send' method on ILoraTransceiver' interface) 
  CLoraTransceiverItf_LoraPacket m_pPacketToSend;

  // Data block containing the last received received packet
  CLoraPacket *m_pPacketReceived;

  // Additional information associated to last received packet:
  //  - The radio setting information is updated when settings are changed
  //  - The information about packet reception are recorded when packet is received
  // Values in this structure are garanteed until the CLoraPacket is retrieved by client object
  // (i.e. see synchronization mechanism for 'm_pPacketReceived' access)
  CReceivedLoraPacketInfo m_ReceivedPacketInfo;

  // Module temperature.
  int m_nTemp;

  // ID of SX1276 on SPI bus
  BYTE m_usSpiSlaveID;
  spi_device_handle_t m_SpiDeviceHandle;

  // Interrupt ESP objects
  intr_handle_t m_hPacketReceivedIntOb;

} CSX1276;



// Methods for 'ILoraTransceiver' interface implementation on CSX1276 object
// The 'CLoraTransceiverItfImpl' structure provided by CSX1276 object contains pointers to these methods 
uint32_t CSX1276_AddRef(void *this);
uint32_t CSX1276_ReleaseItf(void *this);

bool CSX1276_Initialize(void *this, void *pParams);
bool CSX1276_SetLoraMAC(void *this, void *pParams);
bool CSX1276_SetLoraMode(void *this, void *pParams);
bool CSX1276_SetPowerMode(void *this, void *pParams);
bool CSX1276_SetFreqChannel(void *this, void *pParams);

bool CSX1276_StandBy(void *this, void *pParams);
bool CSX1276_Receive(void *this, void *pParams);
bool CSX1276_Send(void *this, void *pParams);

bool CSX1276_GetReceivedPacketInfo(void *this, void *pParams);


// Construction
CSX1276 * CSX1276_New();
void CSX1276_Delete(CSX1276 *this);


// Automaton
#define SX1276_AUTOMATON_STATE_CREATED         0
#define SX1276_AUTOMATON_STATE_INITIALIZED     1
#define SX1276_AUTOMATON_STATE_STANDBY         2
#define SX1276_AUTOMATON_STATE_RECEIVING       3
#define SX1276_AUTOMATON_STATE_SENDING         4
#define SX1276_AUTOMATON_STATE_TERMINATED      5
#define SX1276_AUTOMATON_STATE_ERROR           6


#define SX1276_AUTOMATON_NOTIFY_NONE              0x00000000
#define SX1276_AUTOMATON_NOTIFY_COMMAND           0x00000001
#define SX1276_AUTOMATON_NOTIFY_PACKET_RECEIVED   0x00000002
#define SX1276_AUTOMATON_NOTIFY_PACKET_SENT       0x00000004

#define SX1276_AUTOMATON_MAX_CMD_DURATION         2000

#define SX1276_AUTOMATON_CMD_NONE                 0x00000000
#define SX1276_AUTOMATON_CMD_INITIALIZE           0x00000001
#define SX1276_AUTOMATON_CMD_SETLORAMAC           0x00000002
#define SX1276_AUTOMATON_CMD_SETLORAMODE          0x00000003
#define SX1276_AUTOMATON_CMD_SETPOWERMODE         0x00000004
#define SX1276_AUTOMATON_CMD_SETFREQCHANNEL       0x00000005
#define SX1276_AUTOMATON_CMD_STANDBY              0x00000006
#define SX1276_AUTOMATON_CMD_RECEIVE              0x00000007
#define SX1276_AUTOMATON_CMD_SEND                 0x00000008


void CSX1276_PacketRxTxIntHandler(CSX1276 *this);


bool CSX1276_NotifyAndProcessCommand(CSX1276 *this, DWORD dwCommand, void *pCmdParams);

bool CSX1276_ProcessAutomatonNotifyCommand(CSX1276 *this);
bool CSX1276_ProcessAutomatonNotifyPacketReceived(CSX1276 *this);
bool CSX1276_ProcessAutomatonNotifyPacketSent(CSX1276 *this);


bool CSX1276_ProcessInitialize(CSX1276 *this, CLoraTransceiverItf_InitializeParams pParams);
bool CSX1276_ProcessSetLoraMAC(CSX1276 *this, CLoraTransceiverItf_SetLoraMACParams pParams);
bool CSX1276_ProcessSetLoraMode(CSX1276 *this, CLoraTransceiverItf_SetLoraModeParams pParams);
bool CSX1276_ProcessSetPowerMode(CSX1276 *this, CLoraTransceiverItf_SetPowerModeParams pParams);
bool CSX1276_ProcessSetFreqChannel(CSX1276 *this, CLoraTransceiverItf_SetFreqChannelParams pParams);
bool CSX1276_ProcessStandBy(CSX1276 *this, CLoraTransceiverItf_StandByParams pParams);
bool CSX1276_ProcessReceive(CSX1276 *this, CLoraTransceiverItf_ReceiveParams pParams);
bool CSX1276_ProcessSend(CSX1276 *this, CLoraTransceiverItf_SendParams pParams);

// Private methods (implementation)

uint8_t CSX1276_InitializeDevice(CSX1276 *this, BYTE SPISlaveID, CLoraTransceiverItf_InitializeParams pParams);

void CSX1276_clearFlags(CSX1276 *this);

uint8_t CSX1276_setLoRa(CSX1276 *this);
uint8_t CSX1276_setLoRaMode(CSX1276 *this, uint8_t LoraMode);

uint8_t CSX1276_setSyncWord(CSX1276 *this, uint8_t SyncWord);

uint8_t CSX1276_setHeaderON(CSX1276 *this);
uint8_t CSX1276_setHeaderOFF(CSX1276 *this);

uint8_t CSX1276_setCRC_ON(CSX1276 *this);
uint8_t CSX1276_setCRC_OFF(CSX1276 *this);

uint8_t CSX1276_setSF(CSX1276 *this, uint8_t SpreadingFactor);

uint8_t CSX1276_setBW(CSX1276 *this, uint16_t BandWidth);

uint8_t CSX1276_setCR(CSX1276 *this, uint8_t CodingRate);

uint8_t CSX1276_setChannel(CSX1276 *this, uint8_t FreqChannel);

uint8_t CSX1276_setPowerMode(CSX1276 *this, uint8_t PowerMode);
uint8_t CSX1276_setPowerLevel(CSX1276 *this, uint8_t PowerLevel);

uint8_t CSX1276_setPreambleLength(CSX1276 *this, uint16_t PreambleLength);

uint8_t CSX1276_getSNR(CSX1276 *this);
uint8_t CSX1276_getRSSI(CSX1276 *this);
uint8_t CSX1276_getRSSIpacket(CSX1276 *this);
uint8_t CSX1276_getSF(CSX1276 *this);
uint8_t CSX1276_getBW(CSX1276 *this);

uint8_t CSX1276_setRetries(CSX1276 *this, uint8_t RetryNumber);

uint8_t CSX1276_setMaxCurrent(CSX1276 *this, uint8_t OcpRate);

uint8_t CSX1276_setPacketLength(CSX1276 *this, uint8_t PacketLength);

uint8_t CSX1276_getRegs(CSX1276 *this);

uint8_t CSX1276_startStandBy(CSX1276 *this);
uint8_t CSX1276_startReceive(CSX1276 *this);
uint8_t CSX1276_getPacket(CSX1276 *this);
uint8_t CSX1276_startSend(CSX1276 *this, CLoraTransceiverItf_LoraPacket pLoraPacket);

uint8_t CSX1276_getTemp(CSX1276 *this);

void CSX1276_RxChainCalibration(CSX1276 *this);

bool CSX1276_IsDeviceConfigured(CSX1276 *this);

// Private methods static (implementation)

BYTE CSX1276_readRegister(spi_device_handle_t SPIDeviceHandle, BYTE address);
void CSX1276_writeRegister(spi_device_handle_t SPIDeviceHandle, BYTE address, BYTE data);

bool CSX1276_isSF(uint8_t SpreadingFactor);
bool CSX1276_isBW(uint16_t Bandwidth);
bool CSX1276_isCR(uint8_t CodingRate);
bool CSX1276_isChannel(uint8_t FreqChannel);
uint32_t CSX1276_getFreqRegValue(uint8_t FreqChannel);
char * CSX1276_getFreqTextValue(uint8_t FreqChannel);
char * CSX1276_getCRTextValue(uint8_t CodingRate);


#endif 
