/*********************************************************************
 *
 *  Internet Control Message Protocol (ICMP) Server
 *  Module for Microchip TCP/IP Stack
 *   -Provides "ping" diagnostics
 *	 -Reference: RFC 792
 *
 *********************************************************************
 * FileName:        ICMP.c
 * Dependencies:    IP, ARP
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
 * Author               Date    	Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Howard Schlunder		03/16/07	Original
 ********************************************************************/
#define __ICMP_C

#include "TCPIP.h"

#if defined(STACK_USE_ICMP_SERVER) || defined(STACK_USE_ICMP_CLIENT)


/*********************************************************************
 * Function:        void ICMPProcess(void)
 *
 * PreCondition:    MAC buffer contains ICMP type packet.
 *
 * Input:           *remote: Pointer to a NODE_INFO structure of the 
 *					ping requester
 *					len: Count of how many bytes the ping header and 
 *					payload are in this IP packet
 *
 * Output:          Generates an echo reply, if requested
 *					Validates and sets ICMPFlags.bReplyValid if a 
 *					correct ping response to one of ours is received.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
void ICMPProcess(NODE_INFO *remote, WORD len)
{
	DWORD_VAL dwVal;

    // Obtain the ICMP header Type, Code, and Checksum fields
    MACGetArray((BYTE*)&dwVal, sizeof(dwVal));
	
	// See if this is an ICMP echo (ping) request
	if(dwVal.w[0] == 0x0008u)
	{
		// Validate the checksum using the Microchip MAC's DMA module
		// The checksum data includes the precomputed checksum in the 
		// header, so a valid packet will always have a checksum of 
		// 0x0000 if the packet is not disturbed.
		if(MACCalcRxChecksum(0+sizeof(IP_HEADER), len))
			return;
	
		// Calculate new Type, Code, and Checksum values
		dwVal.v[0] = 0x00;	// Type: 0 (ICMP echo/ping reply)
		dwVal.v[2] += 8;	// Subtract 0x0800 from the checksum
		if(dwVal.v[2] < 8u)
		{
			dwVal.v[3]++;
			if(dwVal.v[3] == 0u)
				dwVal.v[2]++;
		}
	
	    // Wait for TX hardware to become available (finish transmitting 
	    // any previous packet)
	    while(!IPIsTxReady());

		// Position the write pointer for the next IPPutHeader operation
		// NOTE: do not put this before the IPIsTxReady() call for WF compatbility
	    MACSetWritePtr(BASE_TX_ADDR + sizeof(ETHER_HEADER));
        	
		// Create IP header in TX memory
		IPPutHeader(remote, IP_PROT_ICMP, len);
	
		// Copy ICMP response into the TX memory
		MACPutArray((BYTE*)&dwVal, sizeof(dwVal));
		MACMemCopyAsync(-1, -1, len-4);
		while(!MACIsMemCopyDone());
	
		// Transmit the echo reply packet
	    MACFlush();
	}

}

#endif