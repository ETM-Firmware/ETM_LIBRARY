/*********************************************************************
 *
 *	TCP/IP Stack Manager
 *  Module for Microchip TCP/IP Stack
 *	 -Handles internal RX packet pre-processing prior to dispatching 
 *    to upper application layers.
 *	 -Reference: AN833
 *
 *********************************************************************
 * FileName:        StackTsk.c
 * Dependencies:    ARP, IP, Network layer interface (ENC28J60.c, 
 *					ETH97J60.c, ENCX24J600.c, or WFMac.c)
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
 * V5.36 ---- STACK_USE_MPFS has been removed.
********************************************************************/
#define __STACKTSK_C

#include "TCPIP.h"

// Stack FSM states.
typedef enum
{
    SM_STACK_IDLE,
    SM_STACK_MAC,
    SM_STACK_IP,
    SM_STACK_ARP,
    SM_STACK_TCP,
    SM_STACK_UDP
} SM_STACK;
static SM_STACK smStack;

NODE_INFO remoteNode;



/*********************************************************************
 * Function:        void StackInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Stack and its componets are initialized
 *
 * Side Effects:    None
 *
 * Note:            This function must be called before any of the
 *                  stack or its component routines are used.
 *
 ********************************************************************/
void StackInit(void)
{
	static BOOL once = FALSE;
    smStack                     = SM_STACK_IDLE;



	if (!once) {
		// Seed the LFSRRand() function
		LFSRSeedRand(GenerateRandomDWORD());
		once = TRUE;
	}

    MACInit();


    ARPInit();


#if defined(STACK_USE_TCP)
    TCPInit();
#endif

}


/*********************************************************************
 * Function:        void StackTask(void)
 *
 * PreCondition:    StackInit() is already called.
 *
 * Input:           None
 *
 * Output:          Stack FSM is executed.
 *
 * Side Effects:    None
 *
 * Note:            This FSM checks for new incoming packets,
 *                  and routes it to appropriate stack components.
 *                  It also performs timed operations.
 *
 *                  This function must be called periodically to
 *                  ensure timely responses.
 *
 ********************************************************************/
void StackTask(void)
{
    WORD dataCount;
    IP_ADDR tempLocalIP;
	BYTE cFrameType;
	BYTE cIPFrameType;



	#if defined(STACK_USE_TCP)
	// Perform all TCP time related tasks (retransmit, send acknowledge, close connection, etc)
	TCPTick();
	#endif



	// Process as many incomming packets as we can
	while(1)
	{

		// Fetch a packet (throws old one away, if not thrown away 
		// yet)
		if(!MACGetHeader(&remoteNode.MACAddr, &cFrameType))
			break;
		
		// Dispatch the packet to the appropriate handler
		switch(cFrameType)
		{
			case MAC_ARP:
				ARPProcess();
				break;
	
			case MAC_IP:
				if(!IPGetHeader(&tempLocalIP, &remoteNode, &cIPFrameType, &dataCount))
					break;

				#if defined(STACK_USE_ICMP_SERVER) || defined(STACK_USE_ICMP_CLIENT)
				if(cIPFrameType == IP_PROT_ICMP)
				{

					// Process this ICMP packet if it the destination IP address matches our address or one of the broadcast IP addressees
					if( (tempLocalIP.Val == AppConfig.MyIPAddr.Val) ||
						(tempLocalIP.Val == 0xFFFFFFFF) ||
#if defined(STACK_USE_ZEROCONF_LINK_LOCAL) || defined(STACK_USE_ZEROCONF_MDNS_SD)
                                                (tempLocalIP.Val == 0xFB0000E0) ||
#endif
						(tempLocalIP.Val == ((AppConfig.MyIPAddr.Val & AppConfig.MyMask.Val) | ~AppConfig.MyMask.Val)))
					{
						ICMPProcess(&remoteNode, dataCount);
					}

					break;
				}
				#endif
				
				#if defined(STACK_USE_TCP)
				if(cIPFrameType == IP_PROT_TCP)
				{
					TCPProcess(&remoteNode, &tempLocalIP, dataCount);
					break;
				}
				#endif
				

				break;
		}
	}
}
