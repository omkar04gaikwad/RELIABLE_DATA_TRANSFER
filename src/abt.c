#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/
float timeout = 22.0;

bool transmitting_A;

bool sequence_A;
bool acknum_A;

bool sequence_B;
bool acknum_B;
struct pkt send_packet;
struct pkt pkt_buffer[1000];
int first = -1;
int last = -1;
int itemcount = 0;
void pkt_insert(struct pkt packet){
	    if(last == 1000 - 1){
		            return;
			        }
	        else{
			        if(first == -1){
					            first = 0;
						            }
				        last = last + 1;
					        pkt_buffer[last] = packet;
						        itemcount++;
							    }

}

void delete_pkt(){
	    if(first == -1 || first > last ){
		            return;
			        }
	        else{
			        first = first + 1;
				        itemcount--;
					    }
}

int size(){
	    return itemcount;
}
bool isempty(){
	    return itemcount == 0;
}

int receive_checksum(struct pkt packet){
	    int checksum = 0;
	        checksum += packet.seqnum + packet.acknum;

		    for(int i = 0; i < 20; i++){
			            checksum += packet.payload[i];
				        }

		        return checksum;
}
/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
	memset(&send_packet,0,sizeof send_packet);
	send_packet.seqnum = sequence_A;
	send_packet.acknum = acknum_A;
	struct msg message_froma = message;
		        for(int ch=0;ch<20;ch++){
				    	send_packet.payload[ch] = message_froma.data[ch];
					    }
			    send_packet.checksum = receive_checksum(send_packet);
			        pkt_insert(send_packet);
				    struct pkt buffer_packet_zero = pkt_buffer[first];
				       	if(transmitting_A == true){
							buffer_packet_zero.seqnum = sequence_A;
								buffer_packet_zero.checksum = receive_checksum(buffer_packet_zero);
									starttimer (0, timeout);
									    tolayer3 (0, buffer_packet_zero);     
									    	transmitting_A = false;
										    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	int a_checksum = receive_checksum(packet);
	        if(packet.acknum == sequence_A && (a_checksum == packet.checksum)){
			        		stoptimer(0);
						        		transmitting_A = true;
										    		sequence_A = !sequence_A;
																if(isempty() == false){
																						delete_pkt();
																																														if(isempty() == false){																										struct pkt send_buffer_packet = pkt_buffer[first];
																															send_buffer_packet.seqnum = sequence_A;
																																				send_buffer_packet.checksum = receive_checksum(send_buffer_packet);
																																					        		tolayer3 (0, send_buffer_packet);
																																									        		starttimer (0, timeout);
																																																	transmitting_A = false;
																																																					}																				        }}		        else{
							struct pkt send_buffer_packet = pkt_buffer[first];
										send_buffer_packet.seqnum = sequence_A;
													send_buffer_packet.checksum = receive_checksum(send_buffer_packet);
													        	tolayer3 (0, send_buffer_packet);
																		stoptimer(0);
																		        	starttimer (0, timeout);
																							transmitting_A = false;
																									}	
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	if(isempty() == false){
		    	struct pkt resend_packet_zero = pkt_buffer[first];
					resend_packet_zero.seqnum = sequence_A;
							resend_packet_zero.checksum = receive_checksum(resend_packet_zero);
								    tolayer3(0, resend_packet_zero);
								    	    starttimer(0, timeout);
									    		transmitting_A = false;
												}
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	sequence_A = false; 
	 	transmitting_A = true;  
			acknum_A = false;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	struct pkt ack_packet;
		if(receive_checksum(packet)== packet.checksum && packet.seqnum == sequence_B){	
					ack_packet.acknum = sequence_B;
							ack_packet.seqnum = packet.seqnum;
									for(int cha=0;cha<20;cha++){
													ack_packet.payload[cha] = packet.payload[cha];
															}
											ack_packet.checksum = receive_checksum(ack_packet);
													tolayer5 (1, packet.payload);
														   	sequence_B = !sequence_B;
															            tolayer3 (1, ack_packet);
																        }
		    else{   

			    		if(sequence_B == 0){
									acknum_B = 1;
												ack_packet.acknum = acknum_B;
															ack_packet.seqnum = packet.seqnum;
																		for(int cha=0;cha<20;cha++){
																							ack_packet.payload[cha] = packet.payload[cha];
																										}
																					ack_packet.checksum = receive_checksum(ack_packet);
																						        tolayer3 (1, ack_packet);
																									}
							else{
											acknum_B = 0;
														ack_packet.acknum = acknum_B;
																	ack_packet.seqnum = packet.seqnum;
																				for(int cha=0;cha<20;cha++){
																									ack_packet.payload[cha] = packet.payload[cha];
																												}
																							ack_packet.checksum = receive_checksum(ack_packet);
																								        tolayer3 (1, ack_packet);
																											}
									
							    }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	sequence_B = false;
	    acknum_B = false;
}
