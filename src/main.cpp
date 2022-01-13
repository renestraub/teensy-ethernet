/*
 * Teensy 4.1 Ethernet Tests
 */
#include <Arduino.h>
#include <NativeEthernet.h>

EthernetServer iperf_server(5001);

static void usage();
static void test_iperf2();
static void teensyMAC(uint8_t *mac);

static uint8_t mac[6];
static IPAddress our_ip;


//-----------------------------------------------------------------------------

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  analogWrite(LED_BUILTIN, 20);

  // Wait for console connection so we can show menu
  while (!Serial)
  {
  }

  setup_eth_ip();

  usage();
}

//-----------------------------------------------------------------------------

void loop()
{
  if (Serial.available() > 0)
  {
    auto incomingByte = Serial.read();

    // Serial.print("I received: ");
    // Serial.println(incomingByte, DEC);

    // Flush all other bytes received
    if (Serial.available() > 0)
    {
      (void)Serial.read();
    }

    analogWrite(LED_BUILTIN, 200);

    switch (incomingByte)
    {
    case 'h':
      usage();
      break;

    case '1':
      test_iperf2();
      break;

    default:
      usage();
    }

    analogWrite(LED_BUILTIN, 20);
  }
}

//-----------------------------------------------------------------------------

static void usage()
{
  Serial.println("Teensy USB Host Experiments");
  Serial.println(" h: show this help page");
  Serial.println(" 1: run iperf2 server");
}

//-----------------------------------------------------------------------------

static void test_iperf2()
{
  Serial.println("Waiting for client");
  Serial.print("Start client like iperf ");
  Serial.println(our_ip);

  for (;;)
  {
    EthernetClient client = iperf_server.available();
    if (client)
    {
      uint8_t buf[1024];

      analogWrite(LED_BUILTIN, 250);

      Serial.println("Here is new a client for check arduino performance");
      while (client.connected())
      {
        if (client.available()) {
          client.read(buf, 1024);
          Serial.println("reading client data");
        }
      }
      client.stop();
      Serial.println("Client disonnected");

      analogWrite(LED_BUILTIN, 20);
    }
  }
}


//-----------------------------------------------------------------------------

static void setup_eth_ip()
{
  teensyMAC(mac);

  // Start Ethernet and UDP
  Serial.println("Starting Ethernet system with DHCP. Please wait...");

  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
      Serial.println("Ethernet shield was not found. Sorry, can't run without hardware. :(");
    }
    else if (Ethernet.linkStatus() == LinkOFF)
    {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    for (;;)
    {
      delay(1);
    }
  }

  our_ip = Ethernet.localIP();
  Serial.print("Got DHCP address ");
  Serial.println(our_ip);

  Serial.println("Starting iperf server");
  iperf_server.begin();
}

//-----------------------------------------------------------------------------

static void teensyMAC(uint8_t *mac)
{
  static char teensyMac[23];

#if defined(HW_OCOTP_MAC1) && defined(HW_OCOTP_MAC0)
  Serial.println("using HW_OCOTP_MAC* - see https://forum.pjrc.com/threads/57595-Serial-amp-MAC-Address-Teensy-4-0");
  for (uint8_t by = 0; by < 2; by++)
    mac[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
  for (uint8_t by = 0; by < 4; by++)
    mac[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;

#define MAC_OK

#else

  mac[0] = 0x04;
  mac[1] = 0xE9;
  mac[2] = 0xE5;

  uint32_t SN = 0;
  __disable_irq();

#if defined(HAS_KINETIS_FLASH_FTFA) || defined(HAS_KINETIS_FLASH_FTFL)
  Serial.println("using FTFL_FSTAT_FTFA - vis teensyID.h - see https://github.com/sstaub/TeensyID/blob/master/TeensyID.h");

  FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL;
  FTFL_FCCOB0 = 0x41;
  FTFL_FCCOB1 = 15;
  FTFL_FSTAT = FTFL_FSTAT_CCIF;
  while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF))
    ; // wait
  SN = *(uint32_t *)&FTFL_FCCOB7;

#define MAC_OK

#elif defined(HAS_KINETIS_FLASH_FTFE)
  Serial.println("using FTFL_FSTAT_FTFE - vis teensyID.h - see https://github.com/sstaub/TeensyID/blob/master/TeensyID.h");

  kinetis_hsrun_disable();
  FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL;
  *(uint32_t *)&FTFL_FCCOB3 = 0x41070000;
  FTFL_FSTAT = FTFL_FSTAT_CCIF;
  while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF))
    ; // wait
  SN = *(uint32_t *)&FTFL_FCCOBB;
  kinetis_hsrun_enable();

#define MAC_OK

#endif

  __enable_irq();

  for (uint8_t by = 0; by < 3; by++)
    mac[by + 3] = (SN >> ((2 - by) * 8)) & 0xFF;

#endif

#ifdef MAC_OK
  sprintf(teensyMac, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(teensyMac);
#else
  Serial.println("ERROR: could not get MAC");
#endif
}
