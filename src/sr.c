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
/* called from layer 5, passed the data to be sent to other side */
#include "simulator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>

int A_application = 0;
int A_transport = 0;
int B_application = 0;
int B_transport = 0;
float time_local = 0;

/* Define variables specific to the SR protocol below this space */

/* Basic variables pertaining to SR */
float timerIncrement;
int windowSize;
int slidingWindow;
int senderBuffer = 20000;
int receiverBuffer = 20000;
int baseA;
int baseB;
int seekerA;
int seekerB;
bool repeatedPacket;
float timeElapsed;

/* Variables for extra stats information */
int denominator;
int at;
int ackWithInvalidSeqNum;
int corruptPacket;
int corruptAck;
int ackCount;

/* Extra struct types used */
struct msg storeLastMsg;
struct pkt storeLastAck;
struct pkt ackFromB;

/* Define variables specific to the SR protocol above this space */

/* How to add more fields to an existing struct without
 * modifying it? (No alterations allowed to simulator.h and simulator.cpp)
 * Logic read & understood from:
 * https://bytes.com/topic/c/answers/435918-add-new-member-struct
 * And how to use nested structures, read & understood from:
 * http://www.c4learn.com/c-programming/c-nested-structure/
 */
struct packInfoA
{
    struct pkt packet;
    bool partOfBuffer;
    float sendingTime;
    bool pushedtToL3;
    bool pushedtToL5;
};

struct packInfoB
{
    struct pkt packet;
    bool partOfBuffer;
};

/* Struct memory allocation and array handling - http://www.programiz.com/c-programming/c-structures-pointers */
struct packInfoA *ptrA = (struct packInfoA *)malloc(senderBuffer * sizeof(struct packInfoA));
struct packInfoB *ptrB = (struct packInfoB *)malloc(receiverBuffer * sizeof(struct packInfoB));

int calculatePayloadChecksum(struct pkt packPayload)
{
    int payloadChecksum = 0;
    int i = 0;
    while (i < (sizeof(packPayload.payload)))
    {
        payloadChecksum = payloadChecksum + packPayload.payload[i];
        i++;
    }
    return payloadChecksum;
}

int calculateChecksum(struct pkt pack)
{
    int initChecksum = 0;
    initChecksum = (pack.acknum + pack.seqnum + calculatePayloadChecksum(pack)) * 2;
    return initChecksum;
}

/* Various setters */
int setToZero(int a)
{
    a = 0;
    return a;
}

bool setToFalse(bool a)
{
    a = false;
    return a;
}

bool setToTrue(bool a)
{
    a = true;
    return a;
}
/* Various setters */

int isCorrupt(int a, int b)
{
    if (a == b)
    {
        // printf("@@TEST - GOOD PACKET@@\n");
        return 1; // packet is not corrupt
    }
    else
    {
        // printf("@@TEST - BAD PACKET@@\n");
        return 0; // packet is corrupt
    }
}

float performanceCalculator(int a, int b)
{
    return (float)a / b;
}

void adaptiveTimeout()
{
    // printf("ackcount: %d\n",ackCount);
    // printf("A_app count: %d\n",denominator);
    float eff = performanceCalculator(ackCount, denominator);
    // printf("eff: %f\n",eff);
    if (eff >= 1.0)
    {
        timerIncrement = 12.0;
        printf("selected - 12\n");
        return;
    }
    if (eff > 0.6 && eff < 1.0)
    {
        timerIncrement = 11.0;
        printf("selected - 11\n");
        return;
    }
    if (eff > 0.30 && eff < .60)
    {
        timerIncrement = 10.0;
        printf("selected - 10\n");
        return;
    }
    if (eff < 0.3)
    {
        timerIncrement = 9.0;
        printf("selected - 9\n");
        return;
    }
    else
    {
        return;
    }
}

void storeAndPacketize(int seek, struct pkt packet, struct msg message)
{
    /* Why use strncpy instead of strcpy understood from -
     http://stackoverflow.com/questions/1258550/why-should-you-use-strncpy-instead-of-strcpy */
    /* Use strncpy. strcpy gives seg fault */

    /* How to add more fields to existing struct without
     * modifying it ? (no alterations allowed to simulator.h and simulator.cpp),
     * logic read & understood from -
     * https://bytes.com/topic/c/answers/435918-add-new-member-struct
     * And how to use nested structures, read & understood from -
     * http://www.c4learn.com/c-programming/c-nested-structure/
     */

    strncpy(packet.payload, message.data, sizeof(packet.payload));
    packet.checksum = calculateChecksum(packet);

    /* Accessing struct members - http://www.c4learn.com/c-programming/c-accessing-element-in-structure-array/
     * and C++ the complete reference textbook
     */

    ptrA[seek].pushedtToL3 = setToFalse(ptrA[seek].pushedtToL3);
    ptrA[seek].partOfBuffer = setToTrue(ptrA[seek].partOfBuffer);
    ptrA[seek].pushedtToL5 = setToFalse(ptrA[seek].pushedtToL5);
    ptrA[seek].packet = packet;
}

void A_output(struct msg message)
{
    /* What to do when data received from above? - ROSS KUROSE - Page 226 */
    // printf("--------------------Inside A_Output---------------------\n");
    printf("-------------------------------%d------------------------------\n", seekerA);
    denominator++;
    struct pkt packet;
    setToZero(packet.checksum);
    setToZero(packet.acknum);
    packet.seqnum = seekerA;
    storeAndPacketize(seekerA, packet, message);
    slidingWindow = baseA + windowSize;

    if (seekerA > slidingWindow)
    {
        // Simply update seeker so that new incoming data is packetized and stored at a new seeker position
        seekerA++;
        // printf("value of seeker @: %d\n", seekerA);
    }
    else
    {
        int y = baseA;
        while (y < slidingWindow)
        {
            if (ptrA[y].partOfBuffer == true)
            {
                if (ptrA[y].pushedtToL3 == false && ptrA[y].pushedtToL5 == false)
                {
                    ptrA[y].sendingTime = get_sim_time(); // Each packet has its own sent time. This will be used to monitor packet loss.
                    // Send packet
                    tolayer3(0, ptrA[y].packet);
                    /* Interrupt every unit time to check which packets did not make it */
                    // printf("packet %d sent with checksum %d \n", ptrA[y].packet.seqnum, ptrA[y].packet.checksum);
                    ptrA[y].pushedtToL3 = setToTrue(ptrA[y].pushedtToL3);
                    at++;
                    seekerA++;
                    // printf("value of seeker #: %d\n", seekerA);
                    if (at % 5 == 0)
                    {
                        adaptiveTimeout();
                    }
                }
            }
            y++; // Increment while
        }
    } // End of else
    // printf("exiting A_output\n");
}

/* Called from layer 3 when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    // printf("--------------------Inside A_Input---------------------\n");
    int payloadChecksum = 0;
    int verifyChecksum = 0;
    verifyChecksum = packet.acknum + packet.seqnum;

    int i = 0;
    while (i < (sizeof(packet.payload)))
    {
        payloadChecksum = payloadChecksum + packet.payload[i];
        i++;
    }
    verifyChecksum = (verifyChecksum + payloadChecksum) * 2;
    // printf("@@@@@@@@@@@@@@@ Sent from B: %d\n", packet.checksum);
    // printf("@@@@@@@@@@@@@@@ Received at A: %d\n", verifyChecksum);

    int goodOrBad = isCorrupt(packet.checksum, verifyChecksum);
    slidingWindow = baseA + windowSize;

    if (goodOrBad == 1)
    {
        // printf("ack is good moving forward\n");
        if (packet.acknum >= baseA)
        {
            if (packet.acknum < slidingWindow)
            {
                // printf("A has received a Valid Ack # %d with checksum %d \n", packet.acknum,packet.checksum);
                // Control will reach here if B receives a valid ack
                ackCount++;

                /*
                 Next steps would be:
                 1. Mark packet as delivered.
                 2. Check for other packets. If delivered, move the window.
                 3. Send packets that were stored due to high incoming rate from L5
                 */

                // STEP 1
                // setToFalse(ptrA[packet.acknum].pushedtToL5);

                // printf("previous #STATUS: %d\n", ptrA[packet.acknum].pushedtToL5);
                ptrA[packet.acknum].pushedtToL5 = setToTrue(ptrA[packet.acknum].pushedtToL5);
                // printf("packet marked as delivered, #STATUS %d\n", ptrA[packet.acknum].pushedtToL5);

                // STEP 2
                int y = baseA;
                while (y <= slidingWindow)
                {
                    if (ptrA[y].pushedtToL5 == false)
                    {
                        // baseA++;
                        break;
                        // printf("window moved\n");
                        // printf("base A is: %d\n", baseA);
                    }
                    else
                    {
                        baseA++;
                        // printf("TEST\n");
                        // break;
                    }
                    y++; // Increment while
                }

                // STEP 3
                int g = baseA;
                while (g < slidingWindow)
                {
                    if (ptrA[g].partOfBuffer == true)
                    {
                        if (ptrA[g].pushedtToL3 == false && ptrA[g].pushedtToL5 == false)
                        {
                            ptrA[g].sendingTime = get_sim_time();
                            tolayer3(0, ptrA[g].packet);
                            // printf("buffered packet(s) were stored\n");
                            // setToFalse(ptrA[g].sent);
                            ptrA[g].pushedtToL3 = setToTrue(ptrA[g].pushedtToL3);

                            if (at % 5 == 0)
                            {
                                adaptiveTimeout();
                            }
                        }
                    }
                    g++; // Increment while
                }
            }
        }
    } // if of goodOrBad ends
    else
    {
        if (goodOrBad == 0)
        {
            // printf("A has received an Invalid Ack # %d with checksum %d \n", packet.acknum, packet.checksum);
            corruptAck++;
        }
        else
        {
            ackWithInvalidSeqNum++;
        }
    }
} // Method ends


/* called when A's timer goes off */
void A_timerinterrupt()
{
    /* Compare time elapsed since each packet was sent with the timeout */
    // printf("-------------inside timerinterrupt---------------------\n");
    int y = baseA;
    slidingWindow = baseA + windowSize;
    while (y < slidingWindow)
    {
        if (ptrA[y].pushedtToL3 == true)
        {
            if (ptrA[y].pushedtToL5 == false)
            {
                float currentTime = get_sim_time();
                timeElapsed = currentTime - ptrA[y].sendingTime;
                if (timeElapsed >= timerIncrement)
                {
                    ptrA[y].sendingTime = get_sim_time();
                    tolayer3(0, ptrA[y].packet);
                    // printf("packet #%d retransmitted\n", ptrA[y].packet.seqnum);

                    if (at % 5 == 0)
                    {
                        adaptiveTimeout();
                    }
                }
            }
        }
        y++; // Increment while
    }
    starttimer(0, 1.0);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    windowSize=getwinsize();
    //printf("window size selected is: %d\n", windowSize);
    timerIncrement=12.0;
    baseA=1;
    seekerA=1;
    corruptAck=0;
    ackWithInvalidSeqNum=0;
    corruptPacket=0;
    starttimer(0,1.0);
    denominator=0;
    at=0;
    
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    printf("--------------------Inside B_input---------------------\n");
    // setToFalse(repeatedPacket);

    int verifyChecksum = 0;
    int payloadChecksum = 0;
    verifyChecksum = packet.seqnum + packet.acknum;

    int i = 0;
    while (i < (sizeof(packet.payload)))
    {
        payloadChecksum = payloadChecksum + packet.payload[i];
        i++;
    }
    verifyChecksum = (verifyChecksum + payloadChecksum) * 2;
    // printf("@@@@@@@@@@@@@@@ Sent from A : %d\n",pack.checksum);
    // printf("@@@@@@@@@@@@@@@ Received at B : %d\n",verifyChecksum);

    int goodOrBad = isCorrupt(packet.checksum, verifyChecksum);
    if (goodOrBad == 0)
    {
        // printf("bad packet\n");
        corruptPacket++;
        return;
    }

    repeatedPacket = false;

    if (ptrB[packet.seqnum].partOfBuffer == true)
    {
        if (ptrB[packet.seqnum].packet.seqnum == packet.seqnum)
        {
            // setToTrue(repeatedPacket);
            repeatedPacket = setToTrue(repeatedPacket);
        }
    }

    slidingWindow = baseB + windowSize;
    if (goodOrBad == 1)
    {
        // printf("good packet moving forward\n");
        if (repeatedPacket == false)
        {
            // printf("packet not duplicate moving forward\n");
            // printf("pacekt.seqnum value is: %d\n", packet.seqnum);
            // printf("base B value is: %d\n", baseB);
            if (packet.seqnum >= baseB)
            {
                if (packet.seqnum < slidingWindow)
                {
                    // printf("packet %d falls in sliding window\n",packet.seqnum);
                    /*control will reach here if B reveives a valid packet

                     Next steps would be:
                     STEP 1. Prepare ack (packetize) and send to A
                     STEP 2. update seeker postion in buffer
                     STEP 3. buffer packet at that position
                     STEP 4. check if the packet can be sent to L7?

                    */

                    // STEP 1
                    setToZero(ackFromB.checksum);
                    ackFromB.acknum = packet.seqnum;
                    ackFromB.seqnum = packet.seqnum;
                    strncpy(ackFromB.payload, "acknowledgement", sizeof(ackFromB.payload));
                    ackFromB.checksum = calculateChecksum(ackFromB);
                    tolayer3(1, ackFromB);
                    // printf("ack #%d with checksum %d sent from B sent\n",ackFromB.acknum,ackFromB.checksum);

                    // STEP 2
                    seekerB = packet.seqnum;

                    // STEP 3
                    ptrB[seekerB].packet = packet;
                    ptrB[seekerB].partOfBuffer = setToTrue(ptrB[seekerB].partOfBuffer);

                    // STEP 4
                    int g = baseB;
                    while (g < slidingWindow)
                    {
                        if (ptrB[g].partOfBuffer == true)
                        {
                            tolayer5(1, ptrB[g].packet.payload);
                            // printf("##############packet %d received at L5\n", ptrB[g].packet.seqnum);
                            baseB++;
                        }
                        else
                        {
                            break;
                        }
                        g++; // increment while
                    }
                }
            }
        }
    }

    if (goodOrBad == 1 && repeatedPacket == true)
    {
        setToZero(ackFromB.checksum);
        ackFromB.acknum = packet.seqnum;
        ackFromB.seqnum = packet.seqnum;
        strncpy(ackFromB.payload, "acknowledgement", sizeof(ackFromB.payload));
        ackFromB.checksum = calculateChecksum(ackFromB);
        tolayer3(1, ackFromB);
        // printf("ack # %d from B sent for duplicate packet\n",ackFromB.acknum);
        return;
    }
    // if of goodOrBad ends

    // printf("--------------------Exiting B_input---------------------\n");
} // method ends


/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    baseB=1;
    seekerB=0;
}
