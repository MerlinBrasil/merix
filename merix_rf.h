/**
   USE OF THIS SOFTWARE IS GOVERNED BY THE TERMS AND CONDITIONS
   OF THE LICENSE STATEMENT AND LIMITED WARRANTY FURNISHED WITH
   THE PRODUCT.
   <p/>
   IN PARTICULAR, YOU WILL INDEMNIFY AND HOLD B2N LTD., ITS
   RELATED COMPANIES AND ITS SUPPLIERS, HARMLESS FROM AND AGAINST ANY
   CLAIMS OR LIABILITIES ARISING OUT OF THE USE, REPRODUCTION, OR
   DISTRIBUTION OF YOUR PROGRAMS, INCLUDING ANY CLAIMS OR LIABILITIES
   ARISING OUT OF OR RESULTING FROM THE USE, MODIFICATION, OR
   DISTRIBUTION OF PROGRAMS OR FILES CREATED FROM, BASED ON, AND/OR
   DERIVED FROM THIS SOURCE CODE FILE.
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RF


// use this to test without actual network and client and server
//#define RF_NETWORK_SIMULATION

// uset his to use RadioHeada
#define RF_RADIO_HEAD

#if defined(RF_RADIO_HEAD)
#include <RH_NRF24.h>
#include <RHReliableDatagram.h>
#include <SPI.h> // Not actualy used but needed to compile

#if defined(MODULE_IS_SERVER)
RH_NRF24 RF_DRIVER(RF_CE_PIN, RF_CSN_PIN);
#endif

#if defined(MODULE_IS_CLIENT)
RH_NRF24 RF_DRIVER;
#endif

#define RF_BROADCAST_ADDRESS RH_BROADCAST_ADDRESS
#define RH_RETRANSMIT_COUNT 20

#if not defined(RF_NETWORK_SIMULATION)
#if defined(MODULE_IS_SERVER)
RHReliableDatagram RF_MANAGER = RHReliableDatagram(RF_DRIVER, 0);
#endif
#if defined(MODULE_IS_CLIENT)
RHReliableDatagram RF_MANAGER = RHReliableDatagram(RF_DRIVER, MODULE_HANDSHAKE_DELAY_INDEX);
#endif
#endif

# define RF_MAX_MESSAGE_LEN RH_NRF24_MAX_MESSAGE_LEN
#endif

#if defined(RF_NETWORK_SIMULATION)
#define RF_BROADCAST_ADDRESS 255
# define RF_MAX_MESSAGE_LEN 30
uint8_t RF_SIMULATION_BUF[RF_MAX_MESSAGE_LEN];
uint8_t RF_SIMULATION_SIZE;
#endif

uint8_t RF_LAST_RECEIVED_BUF[RF_MAX_MESSAGE_LEN];
uint8_t RF_LAST_RECEIVED_SIZE;

inline void RF_INIT()
{
  RF_LAST_RECEIVED_SIZE = 0;

#if not defined(RF_NETWORK_SIMULATION)

#if defined(RF_RADIO_HEAD)

  RF_DRIVER.init();
  if (!RF_DRIVER.setRF(RH_NRF24::DataRate::DataRate250kbps, RH_NRF24::TransmitPower::TransmitPower0dBm))
  {
    LOG64_SET(F("RF: INIT RadioHead - Cannot set rate and power"));
    LOG64_NEW_LINE;
  }

  RF_MANAGER.setRetries(RH_RETRANSMIT_COUNT);
  if (!RF_MANAGER.init())
  {
    LOG64_SET(F("RF: INIT RadioHead FAILED!"));
    LOG64_NEW_LINE;
  }

  RF_MANAGER.resetRetransmissions();
  RF_DRIVER.available(); // trurns the driver ON
#endif
#endif

#if defined(RF_NETWORK_SIMULATION)
  RF_SIMULATION_SIZE = 0;
#endif
  LOG64_SET(F("RF: INIT"));
  LOG64_NEW_LINE;
}

inline void RF_SEND_DATA(uint8_t buf[], uint8_t size, uint8_t to)
{

#if not defined(RF_NETWORK_SIMULATION)

#if defined(RF_RADIO_HEAD)
  if (size >= RF_DRIVER.maxMessageLength())
#endif
  {
    LOG64_SET(F("RF: SEND : MESSAGE BIGGER THEN MAXIMUM SUPPORTED : SIZE["));
    LOG64_SET(size);
    LOG64_SET(F("MAXIMUM SUPPORTED["));
#if defined(RF_RADIO_HEAD)
    LOG64_SET(RF_DRIVER.maxMessageLength());
#endif
    LOG64_SET(F("]"));
    LOG64_NEW_LINE;
  }

#if defined(RF_RADIO_HEAD)
  if (!RF_MANAGER.sendtoWait(buf, size, to))
  {
    LOG64_SET(F("RF: SEND : MESSAGE HAS NOT BEEN RECEIVED : SIZE["));
    LOG64_SET(size);
    LOG64_SET(F("]"));
    LOG64_NEW_LINE;
  }
#endif

  LOG64_SET(F("RF: SEND SIZE["));
  LOG64_SET(size);
  LOG64_SET(F("] RETRIES["));
  LOG64_SET(RF_MANAGER.retries());
  LOG64_SET(F("]"));
  LOG64_NEW_LINE;

  RF_MANAGER.resetRetransmissions();

#endif

#if defined(RF_NETWORK_SIMULATION)
  RF_SIMULATION_SIZE = size;
  memcpy(RF_SIMULATION_BUF, buf, size);
#endif

}

inline bool RF_CMP(uint8_t buf[], uint8_t & size, uint8_t buf_tmp[], uint8_t & size_tmp)
{
  if (size != size_tmp)
  {
    return false;
  }

  return (memcmp(buf, buf_tmp, size) == 0);
}

inline bool RF_RECEIVE_DATA(uint8_t buf[], uint8_t & size)
{
  // LOG64_SET(F("RF: RECEIVE_DATA: MILLIS["));
  // LOG64_SET(millis());
  // LOG64_SET(F("]"));
  // LOG64_NEW_LINE;

  uint8_t buf_tmp[RF_MAX_MESSAGE_LEN];
  uint8_t size_tmp = RF_MAX_MESSAGE_LEN;

#if not defined(RF_NETWORK_SIMULATION)

#if defined(RF_RADIO_HEAD)
  // nothing required from RadioHead here when using manager
  {
#endif

#if defined(RF_RADIO_HEAD)
    if (RF_MANAGER.recvfromAck(buf_tmp, &size_tmp))
#endif
    {

      if (RF_CMP(RF_LAST_RECEIVED_BUF, RF_LAST_RECEIVED_SIZE, buf_tmp, size_tmp))
      {
        LOG64_SET(F("RF: RECEIVE_DATA: DUPLICATE: SIZE["));
        LOG64_SET(size_tmp);
        LOG64_SET(F("]"));
        LOG64_NEW_LINE;

        return false;
      }

      MONITOR_UP();

      memcpy(RF_LAST_RECEIVED_BUF, buf_tmp, size_tmp);
      RF_LAST_RECEIVED_SIZE = size_tmp;

      memcpy(buf, buf_tmp, size_tmp);
      size = size_tmp;

      LOG64_SET(F("RF: RECEIVE_DATA: SIZE["));
      LOG64_SET(size_tmp);
      LOG64_SET(F("]"));
      LOG64_NEW_LINE;

      return true;
    }
  }
#endif
#if defined(RF_NETWORK_SIMULATION)
  if (RF_SIMULATION_SIZE > 0)
  {
    size_tmp = RF_SIMULATION_SIZE;
    memcpy(buf_tmp, RF_SIMULATION_BUF, size_tmp);
    RF_SIMULATION_SIZE = 0;

    if (RF_CMP(RF_LAST_RECEIVED_BUF, RF_LAST_RECEIVED_SIZE, buf_tmp, size_tmp))
    {
      LOG64_SET(F("RF: RECEIVE_DATA: DUPLICATE: SIZE["));
      LOG64_SET(size_tmp);
      LOG64_SET(F("]"));
      LOG64_NEW_LINE;

      return false;
    }

    memcpy(RF_LAST_RECEIVED_BUF, buf_tmp, size_tmp);
    RF_LAST_RECEIVED_SIZE = size_tmp;

    memcpy(buf, buf_tmp, size_tmp);
    size = size_tmp;

    LOG64_SET(F("RF: RECEIVE_DATA: SIZE["));
    LOG64_SET(size_tmp);
    LOG64_SET(F("]"));
    LOG64_NEW_LINE;

    return true;
  }
#endif
  return false;
}

inline void RF_REINIT()
{
#if not defined(RF_NETWORK_SIMULATION)
  uint8_t buf[RF_MAX_MESSAGE_LEN];
  uint8_t size = RF_MAX_MESSAGE_LEN;
  RF_RECEIVE_DATA(buf, size);
#if defined(RF_RADIO_HEAD)
  RF_MANAGER.resetRetransmissions();
  RF_DRIVER.available(); // trurns the driver ON
#endif
#else
  RF_SIMULATION_SIZE = 0;
#endif
}

inline void RF_SEND_HANDSHAKE_REQUEST()
{

  uint8_t out_buf[RF_MAX_MESSAGE_LEN];
  uint8_t out_size = RF_MAX_MESSAGE_LEN;

  ID_GET_HANDSHAKE_REQUEST_PACKET(out_buf, out_size);

  RF_SEND_DATA(out_buf, out_size, RF_BROADCAST_ADDRESS);
}

inline void RF_SEND_RESET_REQUEST()
{
  uint8_t out_buf[RF_MAX_MESSAGE_LEN];
  uint8_t out_size = RF_MAX_MESSAGE_LEN;

  ID_GET_RESET_REQUEST_PACKET(out_buf, out_size);

  RF_SEND_DATA(out_buf, out_size, RF_BROADCAST_ADDRESS);
}

inline void RF_SEND_DATA_REQUEST(uint16_t id, uint8_t to)
{
  uint8_t out_buf[RF_MAX_MESSAGE_LEN];
  uint8_t out_size = RF_MAX_MESSAGE_LEN;

  ID_GET_DATA_REQUEST_PACKET(out_buf, out_size, id);

  RF_SEND_DATA(out_buf, out_size, to);
}

inline void RF_()
{
  uint8_t buf[RF_MAX_MESSAGE_LEN];
  uint8_t size = RF_MAX_MESSAGE_LEN;

  //LOG64_SET(F("RF: TRY RECEIVE DATA"));
  //LOG64_NEW_LINE;

  if (RF_RECEIVE_DATA(buf, size))
  {
    LOG64_SET(F("RF: DATA RECEIVED"));
    LOG64_NEW_LINE;

    OPER_PACKET oper_packet;
    uint8_t oper = OPER_PARSE_PACKET(buf, oper_packet);

    switch (oper)
    {
      case OPER_NONE :
        {
          LOG64_SET(F("RF: PROCESSING:  OPER_NONE"));
          LOG64_NEW_LINE;
        }; return;

#if defined(MODULE_IS_SERVER)
      case OPER_HANDSHAKE :
        {
          LOG64_SET(F("RF: PROCESSING:  OPER_HANDSHAKE"));
          LOG64_NEW_LINE;

          if (ID_SERVER_ID != oper_packet.handshake_packet.server_id)
          {
            LOG64_SET(F("RF: PROCESSING:  OPER_HANDSHAKE: HANDSHAKE NOT TO US"));
            LOG64_NEW_LINE;

            return;
          }

          for (uint8_t i = 0; i < MAX_CLIENTS; i++)
          {
            if (SERVER_STORE_CLIENT_ID[i] == 0xFFFF)
            {
              SERVER_STORE_CLIENT_ID[i] = oper_packet.handshake_packet.id;
              SERVER_STORE_CLIENT_INCLUDE[i] = oper_packet.handshake_packet.include;
              strcpy(SERVER_STORE_CLIENT_NAME[i], oper_packet.handshake_packet.name);

              break;
            }
          }

        }; return;
      case OPER_DATA :
        {
          LOG64_SET(F("RF: PROCESSING:  OPER_DATA"));
          LOG64_NEW_LINE;

          if (ID_SERVER_ID != oper_packet.data_packet.server_id)
          {
            LOG64_SET(F("RF: PROCESSING:  OPER_DATA: DATA NOT TO US"));
            LOG64_NEW_LINE;

            return;
          }
          SERVER_STORE_PROCESS_DATA(oper_packet.data_packet.id, oper_packet.data_packet.amps, oper_packet.data_packet.volts, oper_packet.data_packet.seq, oper_packet.data_packet.ah);
        }; return;
#endif

#if defined(MODULE_IS_CLIENT)
      case OPER_HANDSHAKE_REQUEST :
        {
          LOG64_SET(F("RF: PROCESSING:  OPER_HANDSHAKE_REQUEST"));
          LOG64_NEW_LINE;

          if (ID_SERVER_ID == 0xFFFF)
          {
            ID_SET_SERVER_ID(oper_packet.handshake_request_packet.server_id);
          }

          if (ID_SERVER_ID != oper_packet.handshake_request_packet.server_id)
          {
            LOG64_SET(F("RF: PROCESSING:  OPER_HANDSHAKE_REQUEST : NOT FROM OUR SERVER"));
            LOG64_NEW_LINE;

            return;
          }

          delay(ID_DELAY);

          uint8_t out_buf[RF_MAX_MESSAGE_LEN];
          uint8_t out_size = RF_MAX_MESSAGE_LEN;

          ID_GET_HANDSHAKE_PACKET(out_buf, out_size);

          RF_SEND_DATA(out_buf, out_size, 0);

        }; return;
      case OPER_RESET_REQUEST :
        {
          LOG64_SET(F("RF: PROCESSING:  OPER_RESET_REQUEST"));
          LOG64_NEW_LINE;

          if (ID_SERVER_ID == oper_packet.reset_request_packet.server_id)
          {
            ID_REINIT();
          }

        }; return;
      case OPER_DATA_REQUEST :
        {
          LOG64_SET(F("RF: PROCESSING:  OPER_DATA_REQUEST"));
          LOG64_NEW_LINE;

          if (ID_SERVER_ID != oper_packet.data_request_packet.server_id)
          {
            LOG64_SET(F("RF: PROCESSING:  OPER_DATA_REQUEST : NOT FROM OUR SERVER"));
            LOG64_NEW_LINE;

            return;
          }

          if (ID_ID != oper_packet.data_request_packet.id)
          {
            LOG64_SET(F("RF: PROCESSING:  OPER_DATA_REQUEST : NOT FOR US"));
            LOG64_NEW_LINE;

            return;
          }

          uint8_t out_buf[RF_MAX_MESSAGE_LEN];
          uint8_t out_size = RF_MAX_MESSAGE_LEN;

          float amps;
          float volts;
          FLOAT_FLOAT ah;

          CLIENT_SENSORS_GET(amps, volts, ah);

          ID_GET_DATA_PACKET(out_buf, out_size, amps, volts, ah);

          RF_SEND_DATA(out_buf, out_size, 0);

        }; return;
#endif
    }
  }
}


