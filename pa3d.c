/* Programming Assignment 3: Exercise D
 *
 * Now that you have a working implementation of semaphores, you can
 * implement a more sophisticated synchronization scheme for the car
 * simulation.
 *
 * Study the program below.  Car 1 begins driving over the road, entering
 * from the East at 40 mph.  After 900 seconds, both Car 3 and Car 4 try to
 * enter the road.  Car 1 may or may not have exited by this time (it should
 * exit after 900 seconds, but recall that the times in the simulation are
 * approximate).  If Car 1 has not exited and Car 4 enters, they will crash
 * (why?).  If Car 1 has exited, Car 3 and Car 4 will be able to enter the
 * road, but since they enter from opposite directions, they will eventually
 * crash.  Finally, after 1200 seconds, Car 2 enters the road from the West,
 * and traveling twice as fast as Car 4.  If Car 3 was not coming from the
 * opposite direction, Car 2 would eventually reach Car 4 from the back and
 * crash.  (You may wish to experiment with reducing the initial delay of
 * Car 2, or increase the initial delay of Car 3, to cause Car 2 to crash
 * with Car 4 before Car 3 crashes with Car 4.)
 *
 *
 * Exercises
 *
 * 1. Modify the procedure driveRoad such that the following rules are obeyed:
 *
 *	A. Avoid all collisions.
 *
 *	B. Multiple cars should be allowed on the road, as long as they are
 *	traveling in the same direction.
 *
 *	C. If a car arrives and there are already other cars traveling in the
 *	SAME DIRECTION, the arriving car should be allowed enter as soon as it
 *	can. Two situations might prevent this car from entering immediately:
 *	(1) there is a car immediately in front of it (going in the same
 *	direction), and if it enters it will crash (which would break rule A);
 *	(2) one or more cars have arrived at the other end to travel in the
 *	opposite direction and are waiting for the current cars on the road
 *	to exit, which is covered by the next rule.
 *
 *	D. If a car arrives and there are already other cars traveling in the
 *	OPPOSITE DIRECTION, the arriving car must wait until all these other
 *	cars complete their course over the road and exit.  It should only wait
 *	for the cars already on the road to exit; no new cars traveling in the
 *	same direction as the existing ones should be allowed to enter.
 *
 *	E. This last rule implies that if there are multiple cars at each end
 *	waiting to enter the road, each side will take turns in allowing one
 *	car to enter and exit.  (However, if there are no cars waiting at one
 *	end, then as cars arrive at the other end, they should be allowed to
 *	enter the road immediately.)
 *	
 *	F. If the road is free (no cars), then any car attempting to enter
 *	should not be prevented from doing so.
 *
 *	G. All starvation must be avoided.  For example, any car that is
 *	waiting must eventually be allowed to proceed.
 *
 * This must be achieved ONLY by adding synchronization and making use of
 * shared memory (as described in Exercise C).  You should NOT modify the
 * delays or speeds to solve the problem.  In other words, the delays and
 * speeds are givens, and your goal is to enforce the above rules by making
 * use of only semaphores and shared memory.
 *
 * 2. Devise different tests (using different numbers of cars, speeds,
 * directions) to see whether your improved implementation of driveRoad
 * obeys the rules above.
 *
 * IMPLEMENTATION GUIDELINES
 * 
 * 1. Avoid busy waiting. In class one of the reasons given for using
 * semaphores was to avoid busy waiting in user code and limit it to
 * minimal use in certain parts of the kernel. This is because busy
 * waiting uses up CPU time, but a blocked process does not. You have
 * semaphores available to implement the driveRoad function, so you
 * should not use busy waiting anywhere.
 *
 * 2. Prevent race conditions. One reason for using semaphores is to
 * enforce mutual exclusion on critical sections to avoid race conditions.
 * You will be using shared memory in your driveRoad implementation.
 * Identify the places in your code where there may be race conditions
 * (the result of a computation on shared memory depends on the order
 * that processes execute).  Prevent these race conditions from occurring
 * by using semaphores.
 *
 * 3. Implement semaphores fully and robustly.  It is possible for your
 * driveRoad function to work with an incorrect implementation of
 * semaphores, because controlling cars does not exercise every use of
 * semaphores.  You will be penalized if your semaphores are not correctly
 * implemented, even if your driveRoad works.
 *
 * 4. Control cars with semaphores: Semaphores should be the basic
 * mechanism for enforcing the rules on driving cars. You should not
 * force cars to delay in other ways inside driveRoad such as by calling
 * the Delay function or changing the speed of a car. (You can leave in
 * the delay that is already there that represents the car's speed, just
 * don't add any additional delaying).  Also, you should not be making
 * decisions on what cars do using calculations based on car speed (since
 * there are a number of unpredictable factors that can affect the
 * actual cars' progress).
 *
 * GRADING INFORMATION
 *
 * 1. Semaphores: We will run a number of programs that test your
 * semaphores directly (without using cars at all). For example:
 * enforcing mututal exclusion, testing robustness of your list of
 * waiting processes, calling signal and wait many times to make sure
 * the semaphore state is consistent, etc.
 *
 * 2. Cars: We will run some tests of cars arriving in different ways,
 * to make sure that you correctly enforce all the rules for cars given
 * in the assignment.  We will use a correct semaphore implementation for
 * these tests so that even if your semaphores are not correct you could
 * still get full credit on the driving part of the grade.  Think about
 * how your driveRoad might handle different situations and write your
 * own tests with cars arriving in different ways to make sure you handle
 * all cases correctly.
 *
 *
 * WHAT TO TURN IN
 *
 * You must turn in two files: mykernel3.c and p3d.c.  mykernel3.c should
 * contain you implementation of semaphores, and p3d.c should contain
 * your modified version of InitRoad and driveRoad (Main will be ignored).
 * Note that you may set up your static shared memory struct and other
 * functions as you wish. They should be accessed via InitRoad and driveRoad,
 * as those are the functions that we will call to test your code.
 *
 * Your programs will be tested with various Main programs that will exercise
 * your semaphore implementation, AND different numbers of cars, directions,
 * and speeds, to exercise your driveRoad function.  Our Main programs will
 * first call InitRoad before calling driveRoad.  Make sure you do as much
 * rigorous testing yourself to be sure your implementations are robust.
 */

#include <stdio.h>
#include "aux.h"
#include "sys.h"
#include "umix.h"

/*#define NUMSTRUCT 3
#define GATE 0
#define WAITLINE 1
#define NUMCARSWAITING 2*/
#define TRUE 1
#define FALSE 0

#define IPOS(FROM)	(((FROM) == WEST) ? 1 : NUMPOS)
#define EDGEPOS(FROM) (((FROM) == WEST)? 0 : (NUMPOS-1))
#define ISWEST(FROM) (((FROM) == WEST)? TRUE : FALSE)
#define NEGPOS(FROM) (((FROM) == WEST)? EAST : WEST)

void InitRoad (void);
void driveRoad (int from, int mph);
//void NowWait(int from, int var);
//void NowSignal(int from, int var);
//void updateVal(int from, int var, int val);

struct Shared{
	int numCarsOnRoad;
	int isturn;
	int roadBlocks[NUMPOS];
	int printfsync;
	int numCarsSync;

	struct {
		int gate;
		int waitline;
		int numCarsWaiting;
		//int * warray[NUMSTRUCT];
	}west;
	
	struct { 
		int gate;
		int waitline;
		int numCarsWaiting;
		//int *earray[NUMSTRUCT];
	}east;
	
} shvars ;

void Main ()
{
	InitRoad ();

	/* The following code is specific to this particular simulation,
	 * e.g., number of cars, directions, and speeds.  You should
	 * experiment with different numbers of cars, directions, and
	 * speeds to test your modification of driveRoad.  When your
	 * solution is tested, we will use different Main procedures,
	 * which will first call InitRoad before any calls to driveRoad.
	 * So, you should do any initializations in InitRoad.
	 */

	if (Fork () == 0) {
		Delay (1200);			// car 2
		driveRoad (WEST, 60);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (900);			// car 3
		driveRoad (EAST, 50);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (900);			// car 4
		driveRoad (WEST, 30);
		Exit ();
	}

	driveRoad (EAST, 40);			// car 1

	Exit ();
}

/* Our tests will call your versions of InitRoad and driveRoad, so your
 * solution to the car simulation should be limited to modifying the code
 * below.  This is in addition to your implementation of semaphores
 * contained in mykernel3.c.
 */

void InitRoad ()
{
	/* do any initializations here */
	Regshm ((char *) &shvars, sizeof (shvars));
	int i;

	//counters
	shvars.numCarsOnRoad = 0;
	shvars.west.numCarsWaiting = 0;
	shvars.east.numCarsWaiting = 0;

	shvars.printfsync = Seminit(1);
	shvars.isturn = Seminit(1);
	shvars.numCarsSync = Seminit(1);

	//semaphores for resources
	/*If there is no one in the line, don't block, let them go.
	This is handled by giving it value of 1 */
	for (i = 0; i < NUMPOS; i++){
		shvars.roadBlocks[i] = Seminit(1);
	}

	//these are semaphores for west and east gates
 	shvars.west.gate = Seminit(1);
	shvars.east.gate = Seminit(1);

	shvars.west.waitline = Seminit(1);
	shvars.east.waitline = Seminit(1);
	
	
	/*shvars.west.warray[GATE] = &shvars.west.gate;
	shvars.east.earray[GATE] = &shvars.east.gate;

	shvars.west.warray[WAITLINE] = &shvars.west.waitline;
	shvars.east.earray[WAITLINE] = &shvars.east.waitline;

	shvars.west.warray[NUMCARSWAITING] = &shvars.west.numCarsWaiting;
	shvars.east.earray[NUMCARSWAITING] = &shvars.east.numCarsWaiting;*/

}

/*void NowWait(int from, int var){
	int res;
	if (from == WEST)
		res = *(shvars.west.warray[var]);
	else
		res = *(shvars.east.earray[var]);
	Wait(res);
}

void NowSignal(int from, int var){
	int res;
	if (from == WEST)
		res = *(shvars.west.warray[var]);
	else
		res = *(shvars.east.earray[var]);
	Signal(res);
}

void updateVal(int from, int var, int val){
	if (from == WEST)
		*(shvars.west.warray[var]) = val;
	else
		*(shvars.east.earray[var]) = val;
}*/

void driveRoad (from, mph)
	int from;			// coming from which direction
	int mph;			// speed of car
{
	int c;				// car ID c = process ID
	int p, np, i;			// positions

	c = Getpid ();			// learn this car's ID

	//printf("From: %s\n",ISWEST(from)?"WEST":"EAST");

	//add yourself to the line
	Wait(ISWEST(from) ? shvars.west.waitline : shvars.east.waitline);
	//is this your turn right now?
	//printf("waitline\n");
	Wait(shvars.isturn);
	//printf("turn\n");
	//is the gate open?
	Wait(ISWEST(from) ? shvars.west.gate : shvars.east.gate);
	//printf("gate\n");
    //printf("gate %d car %d\n", EDGEPOS(from), c);
	Wait(shvars.roadBlocks[EDGEPOS(from)]);
	//printf("got 1st block\n");

	//the following 2 lines must come before assigning turn to the other cars
	EnterRoad (from);
	//printf("entered road\n");

	/*we want to close the other gate if no cars going from "from" to other end
	because it's our turn.*/
	Wait(shvars.numCarsSync);
	if (!shvars.numCarsOnRoad) (Wait(ISWEST(from) ? shvars.east.gate : shvars.west.gate));
	Signal(shvars.numCarsSync);

	//wait on edge position to become available
	//printf("close opposite gate\n");

	//protect the shared variable
	Wait(shvars.numCarsSync);
	shvars.numCarsOnRoad += 1;
	Signal(shvars.numCarsSync);

	//open the gate you came from
	Signal(ISWEST(from) ? shvars.west.gate : shvars.east.gate);
	Signal(shvars.isturn);
	

	//synchronize printing so it's perfectly displayed
	Wait(shvars.printfsync); 
	PrintRoad ();
	Printf ("Car %d enters at %d at %d mph\n", c, IPOS(from), mph);
	Signal(shvars.printfsync);

	for (i = 1; i < NUMPOS; i++) {
		if (from == WEST) {
			p = i;
			np = i + 1;
		} else {
			p = NUMPOS + 1 - i;
			np = p - 1;
		}

		Delay (3600/mph);

		//proceed one semaphore (road block) at a time
		//printf("waiting now\n");
		Wait(shvars.roadBlocks[np - 1]);
		//printf("starting proceeding\n");
		ProceedRoad ();
		//printf("finished proceeding\n");
		Signal(shvars.roadBlocks[p - 1]);
		//printf("next road block empty\n");

		//the waiting spot is empty now, fill it up
		if (EDGEPOS(from)==(p-1))
			ISWEST(from) ? Signal(shvars.west.waitline) : Signal(shvars.east.waitline);

		//printf("queue again filled\n");

		Wait(shvars.printfsync);
		PrintRoad ();
		Printf ("Car %d moves from %d to %d\n", c, p, np);
		Signal(shvars.printfsync);
	}

	Delay (3600/mph);
	ProceedRoad ();
	//printf("proceeding end road\n");

	//protect the shared variable in case of context switch messing everything up
	Wait(shvars.numCarsSync);
	shvars.numCarsOnRoad -= 1;
	Signal(shvars.numCarsSync);

	//printf("num cars decrement\n");

	//free the edge positions of the road for next availability
	Signal(shvars.roadBlocks[EDGEPOS(NEGPOS(from))]); 
	//printf("edge block released %d car %d\n", NEGPOS(from), c);

	Wait(shvars.printfsync);
	PrintRoad ();
	Printf ("Car %d exits road\n", c);
	Signal(shvars.printfsync);

	Wait(shvars.numCarsSync);
	if (!shvars.numCarsOnRoad) (Signal(ISWEST(from) ? shvars.east.gate : shvars.west.gate));
	Signal(shvars.numCarsSync);
	//printf("end\n");
}
