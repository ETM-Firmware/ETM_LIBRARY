/*********************************************************************
 *
 *  Generic TCP Client Example Application
 *  Module for Microchip TCP/IP Stack
 *   -Implements an example HTTP client and should be used as a basis 
 *	  for creating new TCP client applications
 *	 -Reference: None.  Hopefully AN833 in the future.
 *
 *********************************************************************
 * FileName:        GenericTCPClient.c
 * Dependencies:    TCP, DNS, ARP, Tick
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32
 * Compiler:        Microchip C32 v1.05 or higher
 *					Microchip C30 v3.12 or higher
 *					Microchip C18 v3.30 or higher
 *					HI-TECH PICC-18 PRO 9.63PL2 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright (C) 2002-2009 Microchip Technology Inc.  All rights
 * reserved.
 *
 * Microchip licenses to you the right to use, modify, copy, and
 * distribute:
 * (i)  the Software when embedded on a Microchip microcontroller or
 *      digital signal controller product ("Device") which is
 *      integrated into Licensee's product; or
 * (ii) ONLY the Software driver source files ENC28J60.c, ENC28J60.h,
 *		ENCX24J600.c and ENCX24J600.h ported to a non-Microchip device
 *		used in conjunction with a Microchip ethernet controller for
 *		the sole purpose of interfacing with the ethernet controller.
 *
 * You should refer to the license agreement accompanying this
 * Software for additional information regarding your rights and
 * obligations.
 *
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 *
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Howard Schlunder     8/01/06	Original
 ********************************************************************/

/*
  Extensive rework by Dan Parker 11/21/2017
*/
#include <xc.h>
#include "TCPIPConfig.h"
#include "TCPIP.h"
#include "TCPmodbus.h"
//#include "ETM.h"  // Requires ETM Library Version 5 or greater.
#include "ETM_TICK.h"
#include "ETM_IO_PORTS.h"



static void ETMTCPClient(void);
#ifdef ADD_SECOND_MODBUS_CLIENT
static void ETMTCPClient2(void);
#endif

typedef struct {
  unsigned int sm_process_response_timeout;
  unsigned int sm_socket_obtained_timeout;
  unsigned int sm_process_response_timeout_last_command;
  unsigned int sm_socket_obtained_messages_sent;
  unsigned int sm_process_response_messages_received;
  unsigned int sm_disconnect_count;

#ifdef ADD_SECOND_MODBUS_CLIENT
  unsigned int sm_process_response_timeout2;
  unsigned int sm_socket_obtained_timeout2;
  unsigned int sm_process_response_timeout_last_command2;
  unsigned int sm_socket_obtained_messages_sent2;
  unsigned int sm_process_response_messages_received2;
  unsigned int sm_disconnect_count2;
#endif
} TYPE_ETM_ETHERNET_DEBUG;

static TYPE_ETM_ETHERNET_DEBUG etm_ethernet_debug;
static unsigned int  tcp_connected_timeout_milliseconds;
static unsigned int  sm_process_response_timeout_milliseconds;

unsigned char rx_data[MAX_RX_SIZE];

// Declare AppConfig structure and some other supporting stack variables
APP_CONFIG AppConfig;


void ETMTCPModbusInitialize(IPCONFIG* ip_config, unsigned int connection_timeout_milliseconds, unsigned int response_timeout_milliseconds) {
  
  // Initialize Stack and application related NV variables into AppConfig.
  AppConfig.Flags.bIsDHCPEnabled = 0;
  AppConfig.Flags.bInConfigMode = 0;

  AppConfig.MyMACAddr.v[0] = ip_config->mac_addr[0];
  AppConfig.MyMACAddr.v[1] = ip_config->mac_addr[1];
  AppConfig.MyMACAddr.v[2] = ip_config->mac_addr[2];
  AppConfig.MyMACAddr.v[3] = ip_config->mac_addr[3];
  AppConfig.MyMACAddr.v[4] = ip_config->mac_addr[4];
  AppConfig.MyMACAddr.v[5] = ip_config->mac_addr[5];

  AppConfig.MyIPAddr.Val = ip_config->ip_addr;
  AppConfig.DefaultIPAddr.Val = AppConfig.MyIPAddr.Val;
  AppConfig.MyMask.Val = ip_config->mask;
  AppConfig.DefaultMask.Val = AppConfig.MyMask.Val;
  AppConfig.MyGateway.Val = ip_config->gate;
  AppConfig.PrimaryDNSServer.Val = ip_config->dns;
  AppConfig.SecondaryDNSServer.Val = MY_DEFAULT_SECONDARY_DNS_BYTE1 | MY_DEFAULT_SECONDARY_DNS_BYTE2<<8ul  | MY_DEFAULT_SECONDARY_DNS_BYTE3<<16ul  | MY_DEFAULT_SECONDARY_DNS_BYTE4<<24ul;
  AppConfig.MyRemIPAddr.Val = ip_config->remote_ip_addr;
#ifdef ADD_SECOND_MODBUS_CLIENT  
  AppConfig.MyRemIPAddr2.Val = ip_config->remote_ip_addr2;
#endif
    
  // Load the NetBIOS Host Name
  AppConfig.NetBIOSName[0]  = ip_config->net_bios_name[0];
  AppConfig.NetBIOSName[1]  = ip_config->net_bios_name[1];
  AppConfig.NetBIOSName[2]  = ip_config->net_bios_name[2];
  AppConfig.NetBIOSName[3]  = ip_config->net_bios_name[3];
  AppConfig.NetBIOSName[4]  = ip_config->net_bios_name[4];
  AppConfig.NetBIOSName[5]  = ip_config->net_bios_name[5];
  AppConfig.NetBIOSName[6]  = ip_config->net_bios_name[6];
  AppConfig.NetBIOSName[7]  = ip_config->net_bios_name[7];
  AppConfig.NetBIOSName[8]  = ip_config->net_bios_name[8];
  AppConfig.NetBIOSName[9]  = ip_config->net_bios_name[9];
  AppConfig.NetBIOSName[10] = ip_config->net_bios_name[10];
  AppConfig.NetBIOSName[11] = ip_config->net_bios_name[11];
  AppConfig.NetBIOSName[12] = ip_config->net_bios_name[12];
  AppConfig.NetBIOSName[13] = ip_config->net_bios_name[13];
  AppConfig.NetBIOSName[14] = ip_config->net_bios_name[14];
  AppConfig.NetBIOSName[15] = ip_config->net_bios_name[15];

  FormatNetBIOSName(AppConfig.NetBIOSName);

  tcp_connected_timeout_milliseconds = connection_timeout_milliseconds;
  sm_process_response_timeout_milliseconds = response_timeout_milliseconds;

  // Initialize core stack layers (MAC, ARP, TCP, UDP) and
  // application modules (HTTP, SNMP, etc.)
  StackInit();
}


void ETMTCPModbusTask(void) {
  // This task performs normal stack task including checking
  // for incoming packet, type of packet and calling
  // appropriate stack entity to process it.

  StackTask();
  ETMTCPClient();
#ifdef ADD_SECOND_MODBUS_CLIENT
  ETMTCPClient2();
#endif  
}





/*****************************************************************************
  Function:
	void ETMTCPClient(void)

  Summary:
	Implements a simple HTTP client (over TCP).

  Description:
	This function implements a simple HTTP client, which operates over TCP.  
	The function is called periodically by the stack, and waits for BUTTON1 
	to be pressed.  When the button is pressed, the application opens a TCP
	connection to an Internet search engine, performs a search for the word
	"Microchip" on "microchip.com", and prints the resulting HTML page to
	the UART.
	
	This example can be used as a model for many TCP and HTTP client
	applications.

  Precondition:
	TCP is initialized.

  Parameters:
	None

  Returns:
  	None
***************************************************************************/
void ETMTCPClient(void) {
  unsigned int w;
  unsigned int len;



  ETMModbusTXData tx_data;

  static  unsigned char last_id;
  static DWORD		Timer;
  static TCP_SOCKET	MySocket = INVALID_SOCKET;
  static enum _ETMTCPState {
    SM_HOME = 0,
    SM_SOCKET_OBTAINED,
    SM_PROCESS_RESPONSE,
    SM_DISCONNECT,
    SM_DONE
  } ETMTCPState = SM_DONE;

  switch(ETMTCPState) 
    {
    case SM_HOME:
      // Connect a socket to the remote TCP server,
      MySocket = TCPOpen(AppConfig.MyRemIPAddr.Val, TCP_OPEN_IP_ADDRESS, 502, TCP_PURPOSE_TCP_MODBUS_CLIENT);
      
      // Abort operation if no TCP socket of type TCP_PURPOSE_TCP_MODBUS_CLIENT is available
      // If this ever happens, you need to go add one to TCPIPConfig.h
      if(MySocket == INVALID_SOCKET)
	break;
      
      ETMTCPState = SM_SOCKET_OBTAINED;
      Timer = ETMTickGet();
      break;


    case SM_SOCKET_OBTAINED:
      // Wait for the remote server to accept our connection request
      if(!TCPIsConnected(MySocket)) {
	// Time out if too much time is spent in this state
	if(ETMTickGreaterThanNMilliseconds(tcp_connected_timeout_milliseconds, Timer)) {
	  // Close the socket so it can be used by other modules
	  TCPDisconnect(MySocket);
	  MySocket = INVALID_SOCKET;
	  ETMTCPState = SM_HOME;
	  etm_ethernet_debug.sm_socket_obtained_timeout++;
	}
	break;
      }
      
      Timer = ETMTickGet();
      
      tx_data.tx_ready = 0;
      //tx_data = ETMModbusApplicationSpecificTXData();
      ETMModbusApplicationSpecificTXData(&tx_data);

      // don't want to send anything for now, stay in this state
      if (tx_data.tx_ready == 0) {
		break;
      }
      if ((tx_data.header_length + tx_data.data_length) > TCPIsPutReady(MySocket)) {
	// The socket can not be written to
		break;
      }
      
      last_id = tx_data.header_data[6];

      TCPPutArray(MySocket, &tx_data.header_data[0], tx_data.header_length);
      TCPPutArray(MySocket, tx_data.data_ptr, tx_data.data_length);
      
      // Send the packet
      TCPFlush(MySocket);
      etm_ethernet_debug.sm_socket_obtained_messages_sent++;
      ETMTCPState = SM_PROCESS_RESPONSE;
      break;


    case SM_PROCESS_RESPONSE:
      // Check to see if the remote node has disconnected from us or sent us any application data
      if(!TCPIsConnected(MySocket)) {
	ETMTCPState = SM_DISCONNECT;
	// Do not break;  We might still have data in the TCP RX FIFO waiting for us
      }
      
      // Get count of RX bytes waiting
      w = TCPIsGetReady(MySocket);	
      
      if (w) {
	if (w > (MAX_RX_SIZE-1)) {
	  w = (MAX_RX_SIZE-1);
	}
	
	len = TCPGetArray(MySocket, rx_data, w);
	w -= len;
	
	ETMModbusApplicationSpecificRXData(rx_data);
	etm_ethernet_debug.sm_process_response_messages_received++;
	ETMTCPState = SM_SOCKET_OBTAINED; // repeat sending
	
      } else {
	// Time out if too much time is spent in this state
	if(ETMTickGreaterThanNMilliseconds(sm_process_response_timeout_milliseconds, Timer)) {
	  // Close the socket so it can be used by other modules
	  TCPDisconnect(MySocket);
	  MySocket = INVALID_SOCKET;
	  ETMTCPState = SM_HOME;
	  etm_ethernet_debug.sm_process_response_timeout++;
	  etm_ethernet_debug.sm_process_response_timeout_last_command = last_id;
	}
      }
      
      break;

      
    case SM_DISCONNECT:
      // Close the socket so it can be used by other modules
      // For this application, we wish to stay connected, but this state will still get entered if the remote server decides to disconnect
      TCPDisconnect(MySocket);
      etm_ethernet_debug.sm_disconnect_count++;
      MySocket = INVALID_SOCKET;
      ETMTCPState = SM_DONE;
      break;

      
    case SM_DONE:
      ETMTCPState = SM_HOME;
      break;
    }
}


unsigned int ETMTCPModbusGetErrorInfo(unsigned char error) {
  switch (error) 
    {
    case ERROR_COUNT_SM_PROCESS_RESPONSE_TIMEOUT:
      return etm_ethernet_debug.sm_process_response_timeout;
      break;

    case ERROR_COUNT_SM_SOCKET_OBTAINED_TIMEOUT:
      return etm_ethernet_debug.sm_socket_obtained_timeout;
      break;

    case ERROR_SM_PROCESS_RESPONSE_TIMEOUT_ID:
      return etm_ethernet_debug.sm_process_response_timeout_last_command;
      break;

    case COUNT_SM_SOCKET_OBTAINED_MSG_TX:
      return etm_ethernet_debug.sm_socket_obtained_messages_sent;
      break;

    case COUNT_SM_PROCESS_RESPONSE_MSG_RX:
      return etm_ethernet_debug.sm_process_response_messages_received;
      break;

    case ERROR_COUNT_SM_DISCONNECT:
      return etm_ethernet_debug.sm_disconnect_count;
      break;

#ifdef ADD_SECOND_MODBUS_CLIENT
    case ERROR_COUNT_SM_PROCESS_RESPONSE_TIMEOUT2:
      return etm_ethernet_debug.sm_process_response_timeout2;
      break;

    case ERROR_COUNT_SM_SOCKET_OBTAINED_TIMEOUT2:
      return etm_ethernet_debug.sm_socket_obtained_timeout2;
      break;

    case ERROR_SM_PROCESS_RESPONSE_TIMEOUT_ID2:
      return etm_ethernet_debug.sm_process_response_timeout_last_command2;
      break;

    case COUNT_SM_SOCKET_OBTAINED_MSG_TX2:
      return etm_ethernet_debug.sm_socket_obtained_messages_sent2;
      break;

    case COUNT_SM_PROCESS_RESPONSE_MSG_RX2:
      return etm_ethernet_debug.sm_process_response_messages_received2;
      break;

    case ERROR_COUNT_SM_DISCONNECT2:
      return etm_ethernet_debug.sm_disconnect_count2;
      break;
#endif
    }

  return 0;
}

#ifdef ADD_SECOND_MODBUS_CLIENT
/*****************************************************************************
  Function:
	void ETMTCPClient2(void)

  Summary:
	Implements a simple HTTP client (over TCP).

  Description:
	This function implements a simple HTTP client, which operates over TCP.  
	The function is called periodically by the stack, and waits for BUTTON1 
	to be pressed.  When the button is pressed, the application opens a TCP
	connection to an Internet search engine, performs a search for the word
	"Microchip" on "microchip.com", and prints the resulting HTML page to
	the UART.
	
	This example can be used as a model for many TCP and HTTP client
	applications.

  Precondition:
	TCP is initialized.

  Parameters:
	None

  Returns:
  	None
***************************************************************************/
void ETMTCPClient2(void) {
  unsigned int w;
  unsigned int len;



  ETMModbusTXData tx_data;

  static  unsigned char last_id;
  static DWORD		Timer;
  static TCP_SOCKET	MySocket = INVALID_SOCKET;
  static enum _ETMTCPState {
    SM_HOME = 0,
    SM_SOCKET_OBTAINED,
    SM_PROCESS_RESPONSE,
    SM_DISCONNECT,
    SM_DONE
  } ETMTCPState = SM_DONE;

  switch(ETMTCPState) 
    {
    case SM_HOME:
      // Connect a socket to the remote TCP server,
      MySocket = TCPOpen(AppConfig.MyRemIPAddr2.Val, TCP_OPEN_IP_ADDRESS, 502, TCP_PURPOSE_TCP_MODBUS_CLIENT);
      
      // Abort operation if no TCP socket of type TCP_PURPOSE_TCP_MODBUS_CLIENT is available
      // If this ever happens, you need to go add one to TCPIPConfig.h
      if(MySocket == INVALID_SOCKET)
	break;
      
      ETMTCPState = SM_SOCKET_OBTAINED;
      Timer = ETMTickGet();
      break;


    case SM_SOCKET_OBTAINED:
      // Wait for the remote server to accept our connection request
      if(!TCPIsConnected(MySocket)) {
	// Time out if too much time is spent in this state
	if(ETMTickGreaterThanNMilliseconds(tcp_connected_timeout_milliseconds, Timer)) {
	  // Close the socket so it can be used by other modules
	  TCPDisconnect(MySocket);
	  MySocket = INVALID_SOCKET;
	  ETMTCPState = SM_HOME;
	  etm_ethernet_debug.sm_socket_obtained_timeout2++;
	}
	break;
      }
      
      Timer = ETMTickGet();
      
      tx_data.tx_ready = 0;
      //tx_data = ETMModbusApplicationSpecificTXData();
      ETMModbusApplicationSpecificTXData2(&tx_data);

      // don't want to send anything for now, stay in this state
      if (tx_data.tx_ready == 0) {
		break;
      }
      if ((tx_data.header_length + tx_data.data_length) > TCPIsPutReady(MySocket)) {
	// The socket can not be written to
		break;
      }
      
      last_id = tx_data.header_data[6];

      TCPPutArray(MySocket, &tx_data.header_data[0], tx_data.header_length);
      TCPPutArray(MySocket, tx_data.data_ptr, tx_data.data_length);
      
      // Send the packet
      TCPFlush(MySocket);
      etm_ethernet_debug.sm_socket_obtained_messages_sent2++;
      ETMTCPState = SM_PROCESS_RESPONSE;
      break;


    case SM_PROCESS_RESPONSE:
      // Check to see if the remote node has disconnected from us or sent us any application data
      if(!TCPIsConnected(MySocket)) {
	ETMTCPState = SM_DISCONNECT;
	// Do not break;  We might still have data in the TCP RX FIFO waiting for us
      }
      
      // Get count of RX bytes waiting
      w = TCPIsGetReady(MySocket);	
      
      if (w) {
	if (w > (MAX_RX_SIZE-1)) {
	  w = (MAX_RX_SIZE-1);
	}
	
	len = TCPGetArray(MySocket, rx_data, w);
	w -= len;
	
	ETMModbusApplicationSpecificRXData2(rx_data);
	etm_ethernet_debug.sm_process_response_messages_received2++;
	ETMTCPState = SM_SOCKET_OBTAINED; // repeat sending
	
      } else {
	// Time out if too much time is spent in this state
	if(ETMTickGreaterThanNMilliseconds(sm_process_response_timeout_milliseconds, Timer)) {
	  // Close the socket so it can be used by other modules
	  TCPDisconnect(MySocket);
	  MySocket = INVALID_SOCKET;
	  ETMTCPState = SM_HOME;
	  etm_ethernet_debug.sm_process_response_timeout2++;
	  etm_ethernet_debug.sm_process_response_timeout_last_command2 = last_id;
	}
      }
      
      break;

      
    case SM_DISCONNECT:
      // Close the socket so it can be used by other modules
      // For this application, we wish to stay connected, but this state will still get entered if the remote server decides to disconnect
      TCPDisconnect(MySocket);
      etm_ethernet_debug.sm_disconnect_count2++;
      MySocket = INVALID_SOCKET;
      ETMTCPState = SM_DONE;
      break;

      
    case SM_DONE:
      ETMTCPState = SM_HOME;
      break;
    }
}

#endif