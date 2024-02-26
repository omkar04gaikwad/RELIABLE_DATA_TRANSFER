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

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

float timeout = 45.0;

int base_window;
int next_sequence_number;
int sequence_A;
int sequence_B;
int window_size_N ;
int packet_buffer_size;


int receive_checksum(struct pkt packet){
	    int checksum = 0;
	        checksum += packet.seqnum + packet.acknum;
		    for(int i = 0; i < 20; i++){
			            checksum += packet.payload[i];
				        }
		        return checksum;
}

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

struct pkt send_packet;


/* called from layer 5, passed the data to be sent to other side */

struct pkt create_packet(struct msg message)
{
	  struct pkt packet;
	    packet.seqnum = sequence_A;
	      packet.acknum = 0;
	        for(int x = 0;x<20;x++){
			    packet.payload[x] = message.data[x];
			      }
		  packet.checksum = receive_checksum(packet);
		    return packet;
}

void A_output(message)
  struct msg message;
{
	if(next_sequence_number < base_window + window_size_N){
		        pkt_insert(create_packet(message));
			        tolayer3(0, pkt_buffer[sequence_A]);
				        if(base_window == next_sequence_number){
						            starttimer(0, timeout);
							            }
					        next_sequence_number++;
						    }
	    else{
		            pkt_insert(create_packet(message));
			            packet_buffer_size++;
				        }
	        sequence_A++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	if(receive_checksum(packet) == packet.checksum && packet.acknum >= base_window){
		        base_window = packet.acknum + 1;
			        stoptimer(0);
				        if(base_window < next_sequence_number){
						            starttimer(0, timeout);
							            }
					        for(int i = next_sequence_number; i< base_window + window_size_N && packet_buffer_size != 0; i++){
							            tolayer3(0, pkt_buffer[i]);
								                if(base_window == next_sequence_number){
											                starttimer(0, timeout);
													            }
										            next_sequence_number++;
											                packet_buffer_size--;
													        }
						    } 
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	starttimer(0, timeout);
	    int i = base_window;
	        while(i<next_sequence_number){
			        tolayer3(0,pkt_buffer[i]);
				        i++;
					    }
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	base_window = 0;
	   next_sequence_number = 0;
	      sequence_A = 0;
	         window_size_N = getwinsize();
		    packet_buffer_size = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	if(packet.checksum == receive_checksum(packet) && sequence_B == packet.seqnum)
		  {
			          struct pkt ack_packet={0};
				          ack_packet.acknum = sequence_B;
					          ack_packet.seqnum = sequence_B;
						          ack_packet.checksum = receive_checksum(ack_packet);
							          tolayer5(1, packet.payload);
								          tolayer3(1, ack_packet);
									          sequence_B++;
										    }
	  else if(packet.checksum == receive_checksum(packet))
		    {
			            struct pkt ack_packet={0};
				            ack_packet.acknum = sequence_B-1;
					            ack_packet.seqnum = sequence_B-1;
						            ack_packet.checksum = receive_checksum(ack_packet);
							            tolayer3(1, ack_packet);
								      }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	sequence_B = 0;
}
