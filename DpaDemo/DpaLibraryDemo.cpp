/**
 * Copyright 2015-2017 MICRORISC s.r.o.
 * Copyright 2017 IQRF Tech s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "DpaHandler.h"
#include "DpaTask.h"
#include "PrfThermometer.h"
#include "PrfLeds.h"
#include "DpaLibraryDemo.h"
#include "unexpected_peripheral.h"
#include "IqrfCdcChannel.h"
#include "IqrfSpiChannel.h"
#include "IqrfLogging.h"

#include <iostream>
#include <string.h>
#include <iomanip>
#include <thread>

 /**
 * Demo app for testing DPA library.
 *
 * @author Radek Kuchta, Jaroslav Kadlec
 * @author Frantisek Mikulu, Rostislav Spinar
 */

TRC_INIT();

int main(int argc, char** argv)
{
  std::string port_name;

  if (argc < 2) {
    std::cerr << "Usage" << std::endl;
    std::cerr << "  DpaDemo <port-name>" << std::endl << std::endl;
    std::cerr << "Example" << std::endl;
    std::cerr << "  DpaDemo COM5" << std::endl;
    std::cerr << "  DpaDemo /dev/ttyACM0" << std::endl;
    std::cerr << "  DpaDemo /dev/spidev0.0" << std::endl;
    return (-1);
  }
  else {
    port_name = argv[1];
  }
  std::cout << "Start demo app ...\n";

  IChannel* dpaInterface(nullptr);
  size_t found = port_name.find("spi");

  if (found != std::string::npos) {
    try {
      spi_iqrf_config_struct cfg(IqrfSpiChannel::SPI_IQRF_CFG_DEFAULT);
      memset(cfg.spiDev, 0, sizeof(cfg.spiDev));
      auto sz = port_name.size();
      if (sz > sizeof(cfg.spiDev)) sz = sizeof(cfg.spiDev);
      std::copy(port_name.c_str(), port_name.c_str() + sz, cfg.spiDev);

      dpaInterface = new IqrfSpiChannel(cfg);
    }
    catch (SpiChannelException& e) {
      std::cout << e.what() << std::endl;
      return (-1);
    }
  }
  else {
    try {
      //make a default val here if necessary "/dev/ttyACM0";
      dpaInterface = new IqrfCdcChannel(port_name);
    }
    catch (unexpected_peripheral& e) {
      std::cout << e.what() << std::endl;
      return (-1);
    }
    catch (std::exception& e) {
      std::cout << e.what() << std::endl;
      return (-1);
    }
  }

  // start the app
  DpaLibraryDemo* demoApp = new DpaLibraryDemo(dpaInterface);
  demoApp->start();
  std::cout << "That's all for today...";

  // clean
  delete demoApp;
  return 0;
}

/**
 * Constructor
 * @param communicationInterface IQRF interface
 */
DpaLibraryDemo::DpaLibraryDemo(IChannel* communicationInterface)
  : m_dpaHandler(nullptr) {
  m_dpaInterface = communicationInterface;
}

/**
* Destructor
*/
DpaLibraryDemo::~DpaLibraryDemo() {
  delete m_dpaHandler;
}


/**
* App logic
*/
void DpaLibraryDemo::start() {
  try {
    m_dpaHandler = new DpaHandler(m_dpaInterface);
    m_dpaHandler->RegisterAsyncMessageHandler(std::bind(&DpaLibraryDemo::unexpectedMessage,
      this,
      std::placeholders::_1));
  }
  catch (std::invalid_argument& ae) {
    std::cout << "There was an error during DPA handler creation: " << ae.what() << std::endl;
  }

  // default timeout is infinite
  m_dpaHandler->Timeout(100);
  m_dpaHandler->SetRfCommunicationMode(kStd);

  int16_t i = 100;
  //wait for a while, there could be some unread message in CDC
  std::this_thread::sleep_for(std::chrono::seconds(1));

  while (i--) {
    pulseLedRDpaTransaction(0x00);		// Pulse with red led on coordinator
    //pulseLedRDpaTransaction(0x01);	// Pulse with red led on node1

    readTemperatureDpaTransaction(0x00);	// Get temperature from node0
    //ReadTemperatureDpaTransaction(0x01);	// Get temperature from node1

    //PulseLed(0x00, kLedRed);		// Pulse with red led on coordinator
    //PulseLed(0x00, kLedGreen);    // Pulse with green led on coordinator
    //PulseLed(0xFF, kLedRed);		// Pulse with red led using broadcast
    //PulseLed(0xFF, kLedGreen);    // Pulse with green led using broadcast

    //ReadTemperature(0x00);        // Get temperature from coordinator
    //ReadTemperature(0x01);        // Get temperature from node1

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}


/**
* Pulsing LedR on the node using DPA transaction
* @param address Node address
*/
void DpaLibraryDemo::pulseLedRDpaTransaction(uint16_t address)
{
  PrfLedR pulse((int)address, PrfLed::Cmd::PULSE);

  DpaTransactionResult result = m_dpaHandler->ExecuteAsTransaction(pulse);
  if (result.getErrorCode() == 0) {
	  std::cout << pulse.getPrfName() << " " << pulse.getAddress() << " " << pulse.encodeCommand() << std::endl;
  } else {
	  std::cout << "error: " << result.getErrorString() << std::endl;
  }
}


/**
 * Reading temperature from the node using DPA transaction
 * @param address Node address
 */
void DpaLibraryDemo::readTemperatureDpaTransaction(uint16_t address)
{
  PrfThermometer thermometer((int)address, PrfThermometer::Cmd::READ);
  DpaTransactionTask transaction(thermometer);

  m_dpaHandler->ExecuteDpaTransaction(transaction);
  int errorCode = transaction.waitFinish();

  if (errorCode == 0)
    std::cout << NAME_PAR(Temperature, thermometer.getIntTemperature()) << std::endl;
  else
    std::cout << "Failed to read Temperature at: " << NAME_PAR(addres, thermometer.getAddress()) << PAR(errorCode) << std::endl;
}


/**
 * Pulsing LedR on the node using DPA transaction
 * @param address Node address
 */
void DpaLibraryDemo::pulseLed(uint16_t address, LedColor color) {
  DpaMessage::DpaPacket_t packet;

  packet.DpaRequestPacket_t.NADR = address;

  if (color == kLedRed)
    packet.DpaRequestPacket_t.PNUM = PNUM_LEDR;
  else
    packet.DpaRequestPacket_t.PNUM = PNUM_LEDG;

  packet.DpaRequestPacket_t.PCMD = CMD_LED_PULSE;
  packet.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

  DpaMessage message;
  message.DataToBuffer(packet.Buffer, sizeof(TDpaIFaceHeader));

  // sends request and receive response
  executeCommand(message);
}


/**
 * Reading temperature from the node
 * @param address Node address
 */
void DpaLibraryDemo::readTemperature(uint16_t address) {
  static int num(0);
  DpaMessage::DpaPacket_t packet;

  packet.DpaRequestPacket_t.NADR = address;
  packet.DpaRequestPacket_t.PNUM = PNUM_THERMOMETER;

  packet.DpaRequestPacket_t.PCMD = CMD_THERMOMETER_READ;
  packet.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

  // message container
  DpaMessage message;
  message.DataToBuffer(packet.Buffer, sizeof(TDpaIFaceHeader));

  // sends request and receive response
  executeCommand(message);

  // process the response
  if (m_dpaHandler->Status() == DpaTransfer::DpaTransferStatus::kProcessed) {
    int16_t temperature =
      m_dpaHandler->CurrentTransfer().ResponseMessage().DpaPacket().DpaResponsePacket_t.DpaMessage.PerThermometerRead_Response.IntegerValue;

    std::cout << num++ << " Temperature: "
      << std::dec << temperature << " oC" << std::endl;
  }
}


/**
 * Execute DPA transaction, sends request and waits for response
 * @param Message DPA message
 */
void DpaLibraryDemo::executeCommand(DpaMessage& message) {
  static uint16_t sent_messages = 0;
  static uint16_t timeouts = 0;

  try {
    m_dpaHandler->SendDpaMessage(message);
  }
  catch (std::logic_error& le) {
    std::cout << "Send error occured: " << le.what() << std::endl;
    return;
  }

  ++sent_messages;

  while (m_dpaHandler->IsDpaMessageInProgress()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  if (m_dpaHandler->Status() == DpaTransfer::DpaTransferStatus::kTimeout) {
    ++timeouts;
    std::cout << message.NodeAddress()
      << " - Timeout ..."
      << sent_messages
      << ':'
      << timeouts
      << '\n';
  }
}

void DpaLibraryDemo::unexpectedMessage(const DpaMessage& message) {
  std::cout << "Unexpected message received.\n";
}
