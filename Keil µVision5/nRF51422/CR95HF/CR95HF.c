#include "CR95HF.h"
#include "nrf_drv_spi.h"
#include "app_util_platform.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "app_error.h"
#include <string.h>

#define CR95HF_DEBUG      NRF_LOG_PRINTF
// TODO: oder NRF_LOG_DEBUG

#define CR95HF_SPI_USE_NON_BLOCKING_MODE 0 // mit SDK v10 funktioniert dies leider nicht wie mit v11 :(

/* Low Power SPI master
 * The SPI is high current when enabled. To enable low current consumption, you should therefore
 * only enable the SPI when there is data to be transmitted.
 * @see: https://devzone.nordicsemi.com/question/5186/how-to-minimize-current-consumption-for-ble-application-on-nrf51822/
*/
#define CR95HF_SPI_POWER_REDUCE          1


// Lokale Funktionsprototypen:
int spiHelperTransferSingleByte(const uint8_t data_in, uint8_t* data_out);
int spiHelperTransferBuffer(const uint8_t* tx_buf, const uint8_t txrx_len, uint8_t* rx_buf);

static int cr95hfSendCommand(const uint8_t* tx_buf, const uint8_t tx_len);
static int cr95hfPollForResponse(const int timeout);
static int cr95hfReadResponse(void);
static int cr95hfSendCmdPollRead(const uint8_t *tx_buf, const uint8_t tx_len, const int timeout); // write/send, poll, read


//SPI spi(CR95HF_MOSI, CR95HF_MISO, CR95HF_CLK);
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
#if (CR95HF_SPI_USE_NON_BLOCKING_MODE == 1)
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */
#endif

//static uint8_t m_tx_buf[CR95HF_MAX_RX];    /**< TX buffer. */
static uint8_t rxBuffer[CR95HF_MAX_RX];    /**< RX buffer. */
//static uint8_t m_rx_buf[CR95HF_MAX_RX];

// TODO: static uint32_t su32SuccessErrorRate; // Tracks NFC and SPI communication quality
// TODO: static void serAddCode(CR95HF_CODES code)

int spiHelperTransferSingleByte(const uint8_t data, uint8_t* data_out)
{
  int ret_val;

  ret_val = spiHelperTransferBuffer(&data, sizeof(data), data_out);
  if (sizeof(uint8_t) != ret_val)
  {
    CR95HF_DEBUG("ERROR: SPI rx too much/less!\r\n");
    *data_out = 0; // Nichts empfangen (oder zuviel)
  }

  return ret_val;
}

int spiHelperTransferBuffer(const uint8_t* tx_buf, const uint8_t txrx_len, uint8_t* rx_buf)
{
  if (txrx_len > CR95HF_MAX_RX)
  {
    CR95HF_DEBUG("ERROR: SPI rx/tx buffer too small!\r\n");
    return -1;
  }

  // Reset transfer done flag
#if (CR95HF_SPI_USE_NON_BLOCKING_MODE == 1)
  spi_xfer_done = false;
#endif

  APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, tx_buf, txrx_len, rx_buf, txrx_len));

#if (CR95HF_SPI_USE_NON_BLOCKING_MODE == 1)
  while (!spi_xfer_done)
  {
    __WFE();
  }
#endif

  //memcpy(rxBuffer, m_rx_buf, tx_len);

  return txrx_len;
}

#if (CR95HF_SPI_USE_NON_BLOCKING_MODE == 1)
/**
 * @brief SPI user event handler.
 * @param event
 */
void spi_event_handler(nrf_drv_spi_event_t event)
{
  if (NRF_DRV_SPI_EVENT_DONE == event) // Transfer done
  {
    // SPI transfer completed
    spi_xfer_done = true;
  }
  else
  {
    CR95HF_DEBUG("CR95HF: ERROR in spi_event_handler!\r\n");
  }
}
#endif


const uint8_t CR95HF_CONTROL_BYTE_CMD = 0x00;
const uint8_t CR95HF_CONTROL_BYTE_POLL = 0x03;
const uint8_t CR95HF_CONTROL_BYTE_READ = 0x02;
const uint8_t CR95HF_CONTROL_BYTE_RESET = 0x01;

const uint8_t CR95HF_CMD_BF_IDN[] = {CR95HF_CONTROL_BYTE_CMD,0x01,0x00};

const uint8_t CR95HF_CMD_BF_ECHO[] = {CR95HF_CONTROL_BYTE_CMD,0x55};

const uint8_t CR95HF_CMD_BF_PROTOCOL_ISO_15693[] = {CR95HF_CONTROL_BYTE_CMD,0x02,0x02,0x01,0x00};
const uint8_t CR95HF_CMD_BF_PROTOCOL_OFF[] = {CR95HF_CONTROL_BYTE_CMD,0x02,0x02,0x00,0x00};

const uint8_t CR95HF_CMD_BF_HYBERNATE[] = {CR95HF_CONTROL_BYTE_CMD,
                                           0x07, //idle
                                           0x0E, //14 bytes
                                           0x08, //WU source low pulse irq
                                           0x04, 0x00, //Enter Control hybernate
                                           0x04, 0x00, //WU Control hybernate
                                           0x18, 0x00, //Leave Control hybernate
                                           0x00, //WU Period 0
                                           0x00, //Osc Start
                                           0x00, //DAC Start
                                           0x00, 0x00, //DAC Data
                                           0x00, //Swing Count
                                           0x00  //Max Sleep
                                          };

const uint8_t CR95HF_CMD_BF_ISO_15693_INVENTORY[] = {CR95HF_CONTROL_BYTE_CMD,
                                                      0x04, //send receive
                                                     0x03, //length
                                                     0x26, //options:
                                                            /*
                                                             0 -> bit 0: Subcarrier 0 = ask, 1 =fsk
                                                             1 -> bit 1: uplink data rate 0 = low, 1 = high
                                                             1 -> bit 2: inventory flags 0 -> a), 1 -> b)
                                                             0 -> bit 3: proto extension = 0
                                                             a)
                                                             bit 4: select flag 0 = if(bit 5 = 1 address mode)
                                                             bit 5: address flag 1 = use address
                                                             bit 6: for write = 1, else 0
                                                             bit 7: future usage, always 0
                                                             b)
                                                             0 -> bit 4: afi flag 0 = no afi, 1 = afi (Application family identification)
                                                             1 -> bit 5: slot flag, 0 -> 16 slots, 1 -> 1 slot
                                                             0 -> bit 6: for write = 1, else 0
                                                             0 -> bit 7: future usage, always 0
                                                             */
                                                     0x01, // inventory command
                                                     0x00 // do not know why i sent it, maybe useless?
                                                    };

const uint8_t CR95HF_CMD_BF_READ[] = {CR95HF_CONTROL_BYTE_CMD,
                                      0x04, //send receive
                                      0x03, //length
                                      0x02, //options
                                      0x20, //read
                                      0x00  //address
                                     };

const uint8_t CR95HF_CMD_BF_READ_MULTI[] = {CR95HF_CONTROL_BYTE_CMD,
                                            0x04, //send receive
                                            0x00, //length
                                            0x02, //options
                                            0x23, //read
                                            0x00, //address
                                            0x00  //count
                                           };

typedef enum  {
  CR95HF_PROTOCOL_FIELD_OFF = 0x00,
  CR95HF_PROTOCOL_ISO_15693 = 0x01,
  //other not yet supported!
  CR95HF_PROTOCOL_ISO_14443_Type_A = 0x02,// also NFC Forum Tag Type 1 (Topaz), NFC Forum Tag Type 2, NFC Forum Tag Type 4A
  CR95HF_PROTOCOL_ISO_14443_Type_B = 0x03,// also NFC Forum Tag Type 4B
  CR95HF_PROTOCOL_ISO_18092 = 0x04 // also NFC Forum Tag Type 3
} CR95HF_PROTOCOL;


static CR95HF_STATES stateCR95HF = CR95HF_STATE_UNKNOWN;



bool resetCR95HF(void)
{
  uint8_t dummy_byte;

  if (stateCR95HF < CR95HF_STATE_ANSWERING)
  {
    CR95HF_DEBUG("Reset failed, CR95HF not answering\n");
    return false;
  }

  CR95HF_DEBUG("CR95HF: reset\n");

  nrf_gpio_pin_clear(CR95HF_CONFIG_CS_PIN); // pin ss=0
  spiHelperTransferSingleByte(CR95HF_CONTROL_BYTE_RESET, &dummy_byte);
  nrf_gpio_pin_set(CR95HF_CONFIG_CS_PIN); // pin ss=1
  nrf_delay_ms(CR95HF_READY_TIMEOUT);

  stateCR95HF = CR95HF_STATE_UNKNOWN;
  CR95HF_DEBUG("CR95HF: state is now UNKNOWN\r\n");

  return true;
}

bool echoCR95HF(int timeout, bool log)
{
  if (log)
  {
    CR95HF_DEBUG("CR95HF: echo(log=%d)\n", log);
  }

  int len = cr95hfSendCmdPollRead(CR95HF_CMD_BF_ECHO, sizeof(CR95HF_CMD_BF_ECHO), timeout);
  if ((len >= 1) && (rxBuffer[0] == 0x55))
  {
    // stateCR95HF = stateCR95HF > CR95HF_STATE_ANSWERING ? stateCR95HF : CR95HF_STATE_ANSWERING;
    if (stateCR95HF < CR95HF_STATE_ANSWERING)
    {
      // Falls der Zustand noch nicht ANSWERING war, z.B. UNKNOWN, jetzt in ANSWERING wechseln,
      // falls schon PROTOCOLL dann dort verbleiben
      CR95HF_DEBUG("CR95HF: state is now ANSWERING\r\n");
      stateCR95HF = CR95HF_STATE_ANSWERING;
    }
    return true;
  }
  else
  {
    // Es wurde nicht die erwartete Antwort empfangen
    if (log)
    {
      CR95HF_DEBUG("WARNING received trash len: %d \n", len);

      for(int i = 0; i < len; i++)
          CR95HF_DEBUG("rx[%d]: %#x\n", i, rxBuffer[i]);
    }
  }

  CR95HF_DEBUG("CR95HF: state is now UNKNOWN\r\n");
  stateCR95HF = CR95HF_STATE_UNKNOWN;
  return false;
}

CR95HF_STATES getStateCR95HF(void)
{
  return stateCR95HF;
}

// timeout > 10
bool wakeCR95HF(const int timeout)
{
  int t_remaining = timeout / 10;

  CR95HF_DEBUG("CR95HF: wake\n");

#if (CR95HF_SPI_POWER_REDUCE == 1)
  // SPI now needed, reactivate
  nrf_spi_enable(spi.p_registers); // NRF_SPI0->ENABLE = 1;
#endif // CR95HF_SPI_POWER_REDUCE

  /* Startup sequence
   * After the power supply is established at power-on, the CR95HF waits for a low pulse on the pin IRQ_IN (t1)
   * before automatically selecting the external interface (SPI or UART) and entering Ready state after a delay (t3).
   */
  nrf_delay_ms(10+1); // VPS ramp-up time from 0V to VPS max. 10ms
  nrf_gpio_pin_clear(CR95HF_CONFIG_IRQ_IN_PIN); // pin wake = 0 (IRQ_IN)
  nrf_delay_ms(1);                              // Mindestens 10us warten (t1)
  nrf_gpio_pin_set(CR95HF_CONFIG_IRQ_IN_PIN);   // pin wake = 1
  nrf_delay_ms(CR95HF_READY_TIMEOUT); // HFO setup time delay (t3) max. 10ms

  stateCR95HF = CR95HF_STATE_UNKNOWN;
  CR95HF_DEBUG("CR95HF: state is now UNKNOWN\r\n");
  while (!echoCR95HF(10, false) && t_remaining > 0)
  {
    // Mikroe Beispielcode f�hrt an dieser Stelle erneut die Startup-Sequence
    nrf_gpio_pin_clear(CR95HF_CONFIG_IRQ_IN_PIN); // pin wake = 0 (IRQ_IN)
    nrf_delay_ms(1);                              // Mindestens 10us warten (t1)
    nrf_gpio_pin_set(CR95HF_CONFIG_IRQ_IN_PIN);   // pin wake = 1
    nrf_delay_ms(9);
    t_remaining--;
  }
  if (t_remaining < 0)
  {
    return false;
  }

  return true;
}


/*
Send the command: The command data is preceded by a control byte. When sending a command the control byte is 0.
*/
int cr95hfSendCommand(const uint8_t* tx_buf, const uint8_t tx_len)
{
  int retVal;

  nrf_gpio_pin_clear(CR95HF_CONFIG_CS_PIN); // pin ss = 0
  retVal = spiHelperTransferBuffer(tx_buf, tx_len, &rxBuffer[0]); // SPI control byte muss bereits im Puffer stehen!
  nrf_gpio_pin_set(CR95HF_CONFIG_CS_PIN); // pin ss = 1

  return retVal;
}

/* Poll to see if a response is ready: When the CR95HF is ready to respond to a command in SPI mode it will do two things.
  It will set the 3rd bit of the byte it returns when the control byte is 0x03.
  It will pulse the DOUT pin low. If you have an interrupt input you can connect it to DOUT and dispense with the polling.
  Otherwise you should poll to see when you can read data.
*/
int cr95hfPollForResponse(const int timeout)
{
  int ready = 0;
  uint8_t data_out;
  int t_remaining = timeout;

  // TODO: Timer t;
  // TODO: t.start();
  while (!ready)
  {
    nrf_delay_ms(10);
    nrf_gpio_pin_clear(CR95HF_CONFIG_CS_PIN); // pin ss = 0
    spiHelperTransferSingleByte(CR95HF_CONTROL_BYTE_POLL, &data_out); // SPI control byte vorranstellen
    nrf_gpio_pin_set(CR95HF_CONFIG_CS_PIN); // pin ss = 1

    ready = ((data_out & 0x08) == 0x08); // Flags are polled until data is ready (Bit 3 is set when data is ready
    if (ready)
    {
      // TODO: CR95HF_DEBUG("PollForResponse now ready!\r\n");
    }

    // TODO: if(t.read_ms()>timeoutMS) {
    t_remaining -= 100; // TODO: Sehr dirty Hacker like :(
    if (t_remaining <= 0)
    {
      //CR95HF_DEBUG("PollForResponse timeout!\r\n");
      break;
    }
  }

  return ready;
}

/*
Read the data by sending the SPI control byte 0x02 and reading back the data.
*/
int cr95hfReadResponse(void)
{
  uint8_t data_out;

  nrf_gpio_pin_clear(CR95HF_CONFIG_CS_PIN); // pin ss = 0

  spiHelperTransferSingleByte(CR95HF_CONTROL_BYTE_READ , &data_out); // SPI control byte reading back the data
  spiHelperTransferSingleByte(0x00, &data_out); // to read write dummy byte
  rxBuffer[0] = data_out;
  uint8_t len = 0;
  if (rxBuffer[0] != 0x55)
  {
    spiHelperTransferSingleByte(0x00, &data_out); // to read write dummy byte
    len = rxBuffer[1] = data_out;
    memset(&rxBuffer[2], 0x00, len); // Sende <len>-Mal ein Dummy Byte um Daten zu empfangen
    spiHelperTransferBuffer(&rxBuffer[2], len, &rxBuffer[2]); // TODO: Lesen in den Schreibepuffer ;)
    len += 2;
  }
  else
  {
    len = 1;
  }

  nrf_gpio_pin_set(CR95HF_CONFIG_CS_PIN); // pin ss = 1

  return len;
}

int cr95hfSendCmdPollRead(const uint8_t* tx_buf, const uint8_t tx_len, const int timeout)
{
  /* Pseudo Code f�r den kompletten Ablauf:
  // step 1 send the command
  digitalWrite(SSPin, LOW);
  SPI.transfer(0x00); // SPI control byte to send command to CR95HF
  SPI.transfer(0x04); // Send Receive CR95HF command
  SPI.transfer(0x03); // length of data that follows is 0
  SPI.transfer(0x26); // request Flags byte
  SPI.transfer(0x01); // Inventory Command for ISO/IEC 15693
  SPI.transfer(0x00); // mask length for inventory command
  digitalWrite(SSPin, HIGH);
  delay(1);

  // step 2, poll for data ready
  // data is ready when a read byte
  // has bit 3 set (ex: B'0000 1000')
  digitalWrite(SSPin, LOW);
  while(RXBuffer[0] != 8)
  {
    RXBuffer[0] = SPI.transfer(0x03); // Write 3 until
    RXBuffer[0] = RXBuffer[0] & 0x08; // bit 3 is set
  }
  digitalWrite(SSPin, HIGH); delay(1);

  // step 3, read the data
  digitalWrite(SSPin, LOW);
  SPI.transfer(0x02); // SPI control byte for read
  RXBuffer[0] = SPI.transfer(0); // response code
  RXBuffer[1] = SPI.transfer(0); // length of data
  for (i=0;i<RXBuffer[1];i++)
    RXBuffer[i+2]=SPI.transfer(0); // data digitalWrite(SSPin, HIGH);
  delay(1);
  */

  cr95hfSendCommand(tx_buf, tx_len);
  cr95hfPollForResponse(timeout);
  return cr95hfReadResponse();
}

bool protocolISO15693_CR95HF(const uint8_t parameters)
{
  if (stateCR95HF < CR95HF_STATE_ANSWERING)
  {
    CR95HF_DEBUG("SETTING Protocol failed, CR95HF not answering\n");
    return false;
  }

  CR95HF_DEBUG("CR95HF: SETTING Protocol to iso/iec 15693: parameters=0x%#02x\r\n", parameters);

  uint8_t iso[sizeof(CR95HF_CMD_BF_PROTOCOL_ISO_15693)];
  memcpy(iso, CR95HF_CMD_BF_PROTOCOL_ISO_15693, sizeof(CR95HF_CMD_BF_PROTOCOL_ISO_15693));
  iso[sizeof(CR95HF_CMD_BF_PROTOCOL_ISO_15693)-1] = parameters;

  int received = cr95hfSendCmdPollRead(iso, sizeof(iso), 20);
  if ((received  >= 2) && (rxBuffer[0] == 0x00) && (rxBuffer[1] == 0x00))
  {
    if (stateCR95HF < CR95HF_STATE_PROTOCOL)
    {
      // Falls der Zustand schon "weiter" als PROTOCOL war, in diesem Zustand bleiben, ansonsten jetzt in PROTOCOL wechseln!
      CR95HF_DEBUG("CR95HF: state is now PROTOCOL\r\n");
      stateCR95HF = CR95HF_STATE_PROTOCOL;
    }

    return true;
  }
  else
  {
    CR95HF_DEBUG("SETTING Protocol failed: %#x\n", rxBuffer[0]);
    CR95HF_DEBUG("CR95HF: state is now ANSWERING\r\n");
    stateCR95HF = CR95HF_STATE_ANSWERING;
    return false;
  }
}

bool protocolOffCR95HF(void)
{
  if (stateCR95HF < CR95HF_STATE_ANSWERING)
  {
    CR95HF_DEBUG("SETTING Protocol failed, CR95HF not answering\r\n");
    return false;
  }

  CR95HF_DEBUG("CR95HF: SETTING Protocol to OFF\r\n");

  int received = cr95hfSendCmdPollRead(CR95HF_CMD_BF_PROTOCOL_OFF, sizeof(CR95HF_CMD_BF_PROTOCOL_OFF), 20);
  if ((received  >= 2) && (rxBuffer[0] == 0x00) && (rxBuffer[1] == 0x00))
  {
    if (stateCR95HF > CR95HF_STATE_ANSWERING)
    {
      // Falls der Zustand schon "weiter" als ANSWERING war, zurueck in ANSWERING!
      CR95HF_DEBUG("CR95HF: state is now ANSWERING\r\n");
      stateCR95HF = CR95HF_STATE_ANSWERING;
    }
    return true;
  }
  else
  {
    CR95HF_DEBUG("SETTING Protocol failed: %#x\n", rxBuffer[0]);
    stateCR95HF = CR95HF_STATE_ANSWERING;
    CR95HF_DEBUG("CR95HF: state is now ANSWERING\r\n");
    return false;
  }
}

bool identifyCR95HF(CR95HF_IDN* idn, const int timeout)
{
  if (stateCR95HF < CR95HF_STATE_ANSWERING)
  {
    CR95HF_DEBUG("IDN failed, CR95HF not answering\n");
    return false;
  }

  int len = cr95hfSendCmdPollRead(CR95HF_CMD_BF_IDN, sizeof(CR95HF_CMD_BF_IDN), timeout);
  if(len == 17)
  {
    memcpy(idn->deviceID, (const void *)&rxBuffer[2], 13);
    memcpy((void*)&idn->romCRC, (const void *)&rxBuffer[15], 2);
    return true;
  }

  return false;
}

bool hybernateCR95HF(void)
{
  if (stateCR95HF < CR95HF_STATE_ANSWERING)
  {
    CR95HF_DEBUG("SETTING HYBERNATE failed, CR95HF not answering\r\n");
    return false;
  }

  CR95HF_DEBUG("CR95HF: HYBERNATE CR95HF (FIELD_OFF and POWER_DOWN)\r\n");
  if (protocolOffCR95HF())
  {
    cr95hfSendCommand(CR95HF_CMD_BF_HYBERNATE, sizeof(CR95HF_CMD_BF_HYBERNATE));
    stateCR95HF = CR95HF_STATE_SLEEPING;
    CR95HF_DEBUG("CR95HF: state is now SLEEPING\r\n");

#if (CR95HF_SPI_POWER_REDUCE == 1)
    // save power
    nrf_spi_disable(spi.p_registers); // NRF_SPI0->ENABLE = 0;
#endif // CR95HF_SPI_POWER_REDUCE
    return true;
  }
  return false;
}


bool inventoryISO15693_CR95HF(CR95HF_TAG* tag, const int timeout)
{
  if (stateCR95HF < CR95HF_STATE_PROTOCOL)
  {
    CR95HF_DEBUG("inventory failed, CR95HF not in protocol mode\n");
    return false;
  }

  int len = cr95hfSendCmdPollRead(CR95HF_CMD_BF_ISO_15693_INVENTORY, sizeof(CR95HF_CMD_BF_ISO_15693_INVENTORY), timeout);

//  CR95HF_DEBUG("got answer len()=%d\n", len);
//  for(int i = 0; i < len; i++)
//  {
//    CR95HF_DEBUG("%#02x ", rxBuffer[i]);
//  }
//  CR95HF_DEBUG("\n");

  if (rxBuffer[0] != EFrameRecvOK)
  {
    if (EFrameWaitTOut == rxBuffer[0])
    {
      // Fehler, kein Tag in Empfangsreichweite
      // Frame wait time out (no valid reception)
    }
    else
    {
      CR95HF_DEBUG("inventory..got error %#02x\n", rxBuffer[0]);
    }

    if (stateCR95HF > CR95HF_STATE_PROTOCOL)
    {
      // Falls der Zustand schon "weiter" als PROTOCOL war, zur�ck zu PROTOCOL fallen!
      CR95HF_DEBUG("CR95HF: state is now PROTOCOL\r\n");
      stateCR95HF = CR95HF_STATE_PROTOCOL;
    }

    return false;
  }

  int tlen = rxBuffer[1];
  if (tlen < 11)
  {
      CR95HF_DEBUG("to few bytes received \n");
      return false;
  }
  /* this does not work very good, maybe something misinterpreted from docs
  if(rxBuffer[tlen-1] & 0x01) {
      CR95HF_DEBUG("got collision \n");
      return false;
  }
  if(rxBuffer[tlen-1] & 0x02) {
      DEBUG("got bad crc \n");
      return false;
  }*/
  tag->crc[0] = rxBuffer[tlen-2];
  tag->crc[1] = rxBuffer[tlen-3];

  for(int i = 0; i < 9; i++) {
      tag->uid[i] = rxBuffer[11-i];
  }

  stateCR95HF = CR95HF_STATE_TAG_IN_RANGE;
  CR95HF_DEBUG("CR95HF: state is now TAG_IN_RANGE\r\n");

  return true;
}

int readSingleCR95HF(const uint8_t adr, uint8_t *buf, const int len, const int timeout, const unsigned int max_retry_cnt)
{
  if (stateCR95HF < CR95HF_STATE_PROTOCOL)
  {
    CR95HF_DEBUG("read failed, CR95HF not in protocol\n");
    return -1;
  }

  uint8_t cmd[sizeof(CR95HF_CMD_BF_READ)];
  memcpy(cmd, CR95HF_CMD_BF_READ, sizeof(CR95HF_CMD_BF_READ));
  cmd[sizeof(CR95HF_CMD_BF_READ)-1] = adr;

//  CR95HF_DEBUG("read single block at %#2x..\n",adr);
//  for(int i = 0; i < sizeof(cmd); i++) {
//      CR95HF_DEBUG("%#02x ", cmd[i]);
//  }

  int tx = -1;

  for (int i=0; i<max_retry_cnt; i++)
  {
    tx = cr95hfSendCmdPollRead(cmd, sizeof(cmd), timeout);

  //  CR95HF_DEBUG("got answer len()=%d\n",tx);
  //  for(int i = 0; i < tx; i++) {
  //      CR95HF_DEBUG("%#02x ", rxBuffer[i]);
  //  }
  //  CR95HF_DEBUG("\n");

    if (tx >= 1)
    {
      if (rxBuffer[0] == EFrameRecvOK)
      {
        // Ok, Daten empfangen
        break;
      }
      else if (rxBuffer[0] == EFrameWaitTOut)
      {
        // Fehler: Frame wait time out (no valid reception)
        // --> erneut versuchen
      }
      else if (rxBuffer[0] == ERecvLost)
      {
        // Fehler: When reception is lost without EOF received (or subcarrier was lost)
        // --> erneut versuchen
      }
      else
      {
        // Sonstiger Fehler
        // --> Abbrechen!
        CR95HF_DEBUG("got error %#02x\n", rxBuffer[0]);
        return -1;
      }

    }
    else
    {
      CR95HF_DEBUG("Gar nichts empfangen oder Fehler %i\n", tx);
      return -1;
    }
  }


//  CR95HF_DEBUG("flags: %#02x\n",rxBuffer[2]);

  int tlen = rxBuffer[1]-4;
  if(tlen <=0)
    return -1;

//  CR95HF_DEBUG("read resulted in %d bytes, copying %d bytes\n",rxBuffer[1],(tlen < len ? tlen : len));
  tlen = (tlen < len ? tlen : len);
  memcpy(buf,(const void *)&rxBuffer[3],tlen);

  return tlen;
}

int readMultiCR95HF(const uint8_t adr, const int count, uint8_t *buf, const int len, const int timeout)
{
  if (stateCR95HF < CR95HF_STATE_PROTOCOL)
  {
    CR95HF_DEBUG("multi read failed, CR95HF not in protocol\n");
    return -1;
  }

  uint8_t cmd[sizeof(CR95HF_CMD_BF_READ_MULTI)];
  memcpy(cmd, CR95HF_CMD_BF_READ_MULTI, sizeof(CR95HF_CMD_BF_READ));
  cmd[sizeof(CR95HF_CMD_BF_READ_MULTI)-2] = adr & 0xFF; // Adresse einpatchen
  cmd[sizeof(CR95HF_CMD_BF_READ_MULTI)-1] = (count-1) & 0xFF; // Anzahl einpatchen

  CR95HF_DEBUG("multi read at %#4X for %d..\n",adr, count & 0xFF);
  for (int i = 0; i < sizeof(CR95HF_CMD_BF_READ_MULTI); i++)
  {
    CR95HF_DEBUG("%#02x ",cmd[i]);
  }

  int tx = cr95hfSendCmdPollRead(cmd, sizeof(CR95HF_CMD_BF_READ_MULTI), timeout);

  CR95HF_DEBUG("got answer len()=%d\n", tx);
  for(int i = 0; i < tx; i++)
  {
    CR95HF_DEBUG("%02x ", rxBuffer[i]);
  }
  CR95HF_DEBUG("\r\n");

  if(rxBuffer[0] != EFrameRecvOK)
  {
    CR95HF_DEBUG("got error %#04x\n",rxBuffer[0]);
    return -1;
  }

  CR95HF_DEBUG("flags: %#02x\n", rxBuffer[2]);
  int tlen = rxBuffer[1]-4;
  if(tlen <=0)
      return -1;
  CR95HF_DEBUG("read resultet in %d bytes, copying %d bytes\n",rxBuffer[1],(tlen < len ? tlen : len));
  tlen = (tlen < len ? tlen : len);
  memcpy(buf,(const void *)&rxBuffer[3], tlen);

  return tlen;
}

bool initCR95HF(void)
{
  APP_ERROR_CHECK(NRF_LOG_INIT()); // Nur, falls noch nicht geschehen

  CR95HF_DEBUG("CR95HF: init\n");

#if (CR95HF_SPI_POWER_REDUCE == 1)
  nrf_spi_enable(spi.p_registers); // NRF_SPI0->ENABLE = 1;
#endif // CR95HF_SPI_POWER_REDUCE

  nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG(SPI_INSTANCE);
  spi_config.ss_pin = NRF_DRV_SPI_PIN_NOT_USED; // wir muessen den SS-Pin (CR95HF_CONFIG_CS_PIN) manuell steuern!
  spi_config.frequency = NRF_DRV_SPI_FREQ_1M; // bis 2 MHz unterstuetzt
  // spi_config.mode = The CR95HF supports (CPOL = 0, CPHA = 0) and (CPOL = 1, CPHA = 1) modes. // SPI_CR1_BR_2 ???
  spi_config.mode = NRF_DRV_SPI_MODE_3;
  /*
    NRF_DRV_SPI_MODE_0 = NRF_SPI_MODE_0, ///< SCK active high, sample on leading edge of clock.
    NRF_DRV_SPI_MODE_1 = NRF_SPI_MODE_1, ///< SCK active high, sample on trailing edge of clock.
    NRF_DRV_SPI_MODE_2 = NRF_SPI_MODE_2, ///< SCK active low, sample on leading edge of clock.
    NRF_DRV_SPI_MODE_3 = NRF_SPI_MODE_3  ///< SCK active low, sample on trailing edge of clock.

     * mode | POL PHA
     * -----+--------
     *   0  |  0   0
     *   1  |  0   1
     *   2  |  1   0
     *   3  |  1   1

  Mikroe:   SPI3_Init_Advanced(_SPI_FPCLK_DIV64, _SPI_MASTER | _SPI_8_BIT | _SPI_CLK_IDLE_LOW | _SPI_FIRST_CLK_EDGE_TRANSITION | _SPI_MSB_FIRST | _SPI_SS_DISABLE | _SPI_SSM_ENABLE | _SPI_SSI_1, &_GPIO_MODULE_SPI3_PC10_11_12);
  */
  spi_config.bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST; // SPI bit orders: Most significant bit shifted out first

#if (CR95HF_SPI_USE_NON_BLOCKING_MODE == 1)
  APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler)); // wir nutzen den nicht blockierenden Modus und geben einen Event-Handler an
#else
  APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, NULL)); // wir nutzen den  blockierenden Modus und geben keinen Event-Handler an
#endif

  // SS/CS Pin
  nrf_gpio_cfg_output(CR95HF_CONFIG_CS_PIN);
  nrf_gpio_pin_set(CR95HF_CONFIG_CS_PIN); // Default

  // IRQ_IN Pin
  nrf_gpio_cfg_output(CR95HF_CONFIG_IRQ_IN_PIN);
  
#if defined(CR95HF_CONFIG_IRQ_OUT_PIN)
  // IRQ_OUT Pin
  nrf_gpio_cfg_input(CR95HF_CONFIG_IRQ_OUT_PIN, NRF_GPIO_PIN_NOPULL); // Pin pullup resistor disabled
#endif

  stateCR95HF = CR95HF_STATE_UNKNOWN;
  CR95HF_DEBUG("CR95HF: state is now UNKNOWN\r\n");
  return true;
}
