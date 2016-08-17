// copy https://developer.mbed.org/users/SandraK/code/BLE-bm019-spi/
#ifndef CR95HF_H
#define CR95HF_H

#include <stdint.h>
#include <stdbool.h>


// SPI HW-Konfiguration (MOSI/MISO/CLK PIN's usw.) wird in der nrf_drv_config.h vorgenommen!
// Hier nur die Auswahl der zu verwendeten SPI-Instanz:
#define SPI_INSTANCE  0 /**< SPI instance index. */

#define CR95HF_CONFIG_CS_PIN        18 /**< Chip-Select (SS) */
#define CR95HF_CONFIG_IRQ_IN_PIN    20 /**< CR95HF INT1 / IRQ_IN Pin */
#define CR95HF_CONFIG_IRQ_OUT_PIN   19 /**< CR95HF INT0 / IRQ_OUT Pin */


#define CR95HF_MAX_RX        255 // Datentyp ist uint8_t!
#define CR95HF_READY_TIMEOUT 50 /* 0 for endless waiting else  ms to wait */


typedef enum {
	CR95HF_PROTOCOL_ISO_15693_NO_CRC = 0x00,
	CR95HF_PROTOCOL_ISO_15693_CRC = 0x01,

	CR95HF_PROTOCOL_ISO_15693_SINGLE_SUBCARRIER = 0x00,
	CR95HF_PROTOCOL_ISO_15693_DUAL_SUBCARRIER = 0x02,

	CR95HF_PROTOCOL_ISO_15693_100_MODULATION = 0x00, // 100%
	CR95HF_PROTOCOL_ISO_15693_10_MODULATION = 0x04, // 10%

	CR95HF_PROTOCOL_ISO_15693_RESPECT_312U_DELAY = 0x00,
	CR95HF_PROTOCOL_ISO_15693_WAIT_FOR_SOF = 0x08,

	CR95HF_PROTOCOL_ISO_15693_26_KBPS = 0x00,
	CR95HF_PROTOCOL_ISO_15693_52_KBPS = 0x10,
	CR95HF_PROTOCOL_ISO_15693_6_KBPS = 0x20
} CR95HF_PROTOCOL_ISO_15693_PARAMETER;

typedef enum {
	EFrameRecvOK    = 0x80, // Frame correctly received (additionally see CRC/Parity information)
	EUserStop       = 0x85, // Stopped by user (used only in Card mode)
	ECommError      = 0x86, // Hardware communication error
	EFrameWaitTOut  = 0x87, // Frame wait time out (no valid reception)
	EInvalidSof     = 0x88, // Invalid SOF
	EBufOverflow    = 0x89, // Too many bytes received and data still arriving
	EFramingError   = 0x8A, // if start bit = 1 or stop bit = 0
	EEgtError       = 0x8B, // EGT time out
	EInvalidLen     = 0x8C, // Valid for ISO/IEC 18092, if Length <3
	ECrcError       = 0x8D, // CRC error, Valid only for ISO/IEC 18092
	ERecvLost       = 0x8E, // When reception is lost without EOF received (or subcarrier was lost)
	ENoField        = 0x8F, // When Listen command detects the absence of external field
	EUnintByte      = 0x90, // Residual bits in last byte. Useful for ACK/NAK reception of ISO/IEC 14443 Type A.
} CR95HF_CODES;

typedef enum {
	CR95HF_STATE_UNKNOWN   = 0, // initial condition
	CR95HF_STATE_SLEEPING  = 1, // CR95HF is in sleep mode (HYBERNATE). Wakeup required
	CR95HF_STATE_ANSWERING = 2, // if any communication has been successful
	CR95HF_STATE_PROTOCOL  = 3, // a protocol (other then off) has been set
	CR95HF_STATE_TAG_IN_RANGE = 4 // a tag is in range and is responding without error (e.g. to inventory command). Now or read/write possible

} CR95HF_STATES;

typedef struct
{
	char deviceID[13]; // Device ID in ASCII, example - 'NFC FS2JAST2'
	uint16_t romCRC;
} CR95HF_IDN;

typedef struct {
	uint8_t crc[2];
	uint8_t uid[8];
} CR95HF_TAG;

CR95HF_STATES getStateCR95HF(void);


// TODO:
uint8_t getErrorRate(void); // 100% success vs. error �ber die letzten x Anfragen??? Doppel FIFO notwendig 


bool initCR95HF(void);

bool resetCR95HF(void);
bool wakeCR95HF(const int timeout);
bool hybernateCR95HF(void);

bool identifyCR95HF(CR95HF_IDN* idn, const int timeout);

bool protocolOffCR95HF(void);
bool protocolISO15693_CR95HF(const uint8_t parameters); // see CR95HF_PROTOCOL_ISO_15693_PARAMETER

bool echoCR95HF(const int timeout, const bool log);

bool inventoryISO15693_CR95HF(CR95HF_TAG* tag, const int timeout);

int readSingleCR95HF(const uint8_t adr, uint8_t* buf, const int len, const int timeout, const unsigned int max_retry_cnt);
int readMultiCR95HF(const uint8_t adr, const int count, uint8_t* buf, const int len, const int timeout);

#endif /* CR95HF_H */
