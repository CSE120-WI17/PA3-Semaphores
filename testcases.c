#include <stdio.h>
#include "aux.h"
#include "umix.h"
#include "sys.h"

#define TRUE 1
#define FALSE 0

#define IPOS(FROM)	(((FROM) == WEST) ? 1 : NUMPOS)
#define EDGEPOS(FROM) (((FROM) == WEST)? 0 : (NUMPOS-1))
#define ISWEST(FROM) (((FROM) == WEST)? TRUE : FALSE)
#define NEGPOS(FROM) (((FROM) == WEST)? EAST : WEST)

/* test funcs */
void OriginalTest();
void Test1();
void Test2();
void Test3();
void Test4();
void Test5();
void Test6();
void Test7();
void Test8();
void Test9();
void Test10();
void Test11();
void Test12();
void Test13();
void Test14();
void Test15();
void Test16();
void Test17();
void Test18();
void Test19();
void Test20();
void Test21();
void Test22();
void Test23();
void Test24();
void Test25();
void Test26();
void Test27();
void Test28();
void Test29();
void Test30();
void Test31();
void Test32();
void Test33();

void test_1();
void test_2();
void test_3();
void test_4();
void test_5();
void test_6();
void test_7();
void test_8();
void test_9();
void test_10();
void test_11();

void InitRoad (void);
void driveRoad (int from, int mph);
void forkCar(int delay, int dir, int mph);

struct Shared {
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
	} west;

	struct {
		int gate;
		int waitline;
		int numCarsWaiting;
		//int *earray[NUMSTRUCT];
	} east;

} shvars ;

void Main() {
	//OriginalTest();
	InitRoad();
	for(int i = 0; i<4;i++){
		forkCar(0, WEST, 100);
	}
	driveRoad(WEST,1);
	Exit();
}

void InitRoad() {
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
	for (i = 0; i < NUMPOS; i++) {
		shvars.roadBlocks[i] = Seminit(1);
	}

	//these are semaphores for west and east gates
	shvars.west.gate = Seminit(1);
	shvars.east.gate = Seminit(1);

	shvars.west.waitline = Seminit(1);
	shvars.east.waitline = Seminit(1);
}

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

		Delay (3600 / mph);

		//proceed one semaphore (road block) at a time
		//printf("waiting now\n");
		Wait(shvars.roadBlocks[np - 1]);
		//printf("starting proceeding\n");
		ProceedRoad ();
		//printf("finished proceeding\n");
		Signal(shvars.roadBlocks[p - 1]);
		//printf("next road block empty\n");

		//the waiting spot is empty now, fill it up
		if (EDGEPOS(from) == (p - 1))
			ISWEST(from) ? Signal(shvars.west.waitline) : Signal(shvars.east.waitline);

		//printf("queue again filled\n");

		Wait(shvars.printfsync);
		PrintRoad ();
		Printf ("Car %d moves from %d to %d\n", c, p, np);
		Signal(shvars.printfsync);
	}

	Delay (3600 / mph);
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


void OriginalTest() {
	InitRoad ();
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

void Test1() {
	InitRoad ();
	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 10);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 80);
		Exit ();
	}

	driveRoad (WEST, 5);

	Exit ();
}

void Test2() {
	InitRoad ();

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 10);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 30);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 40);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 50);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 60);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 70);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 80);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 90);
		Exit ();
	}

	driveRoad (WEST, 5);

	Exit ();

}

void Test3() {
	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 10);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 30);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 40);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 50);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 60);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 70);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 80);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 90);
		Exit ();
	}

	driveRoad (WEST, 5);

	Exit ();

}


void Test4() {
	InitRoad ();

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (1000);
		driveRoad (WEST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (1020);
		driveRoad (WEST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (1040);
		driveRoad (WEST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (1000);
		driveRoad (EAST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (1020);
		driveRoad (EAST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (1040);
		driveRoad (EAST, 20);
		Exit ();
	}

	driveRoad (WEST, 20);

	Exit ();

}

void Test5() {
	InitRoad ();

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 50);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (100);
		driveRoad (EAST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (100);
		driveRoad (EAST, 20);
		Exit ();
	}

	driveRoad (WEST, 1);

	Exit ();
}

void Test6() {
	int i;

	InitRoad ();

	driveRoad (WEST, 50);

	for (i = 0; i < 5; i++) {

		if (Fork () == 0) {
			Delay (0);
			driveRoad (WEST, 50);
			Exit ();
		}

		if (Fork () == 0) {
			Delay (0);
			driveRoad (WEST, 50);
			Exit ();
		}

		if (Fork () == 0) {
			Delay (0);
			driveRoad (WEST, 50);
			Exit ();
		}

		if (Fork () == 0) {
			Delay (0);
			driveRoad (EAST, 50);
			Exit ();
		}

		if (Fork () == 0) {
			Delay (0);
			driveRoad (EAST, 50);
			Exit ();
		}

		if (Fork () == 0) {
			Delay (0);
			driveRoad (EAST, 50);
			Exit ();
		}

		Delay(4000);
	}

	Exit ();

}

void Test7() {
	InitRoad ();

	if (Fork () == 0) {
		Delay (10000);
		driveRoad (EAST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (10000);
		driveRoad (WEST, 30);
		Exit ();
	}
	driveRoad (EAST, 30);
	Exit ();
}

void Test8() {
	InitRoad();
	if (Fork () == 0) {
		Delay (1200);			// car 2
		driveRoad (WEST, 60);
		driveRoad (EAST, 30);
		driveRoad (WEST, 100);
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
		driveRoad (WEST, 100);
		Exit ();
	}

	driveRoad (EAST, 40);			// car 1

	Exit ();
}

void Test9() {
	//output will be <<||>>|| or  <|>|<|>|
	InitRoad ();
	if (Fork () == 0) {
		Delay (0); // car 2
		driveRoad (EAST, 3599);
		Exit ();
	}
	if (Fork () == 0) {
		Delay (0); // car 3
		driveRoad (WEST, 3599);
		Exit ();
	}
	if (Fork () == 0) {
		Delay (10); // car 4
		driveRoad (WEST, 3599);
		Exit ();
	}

	driveRoad (EAST, 3599); // car 1
	Exit ();
}

void Test10() { //starvation test
	InitRoad ();

	if (Fork () == 0) {
		driveRoad (WEST, 40);
		Exit ();
	}
	if (Fork () == 0) {
		driveRoad (WEST, 40);
		Exit ();
	}
	if (Fork () == 0) {
		driveRoad (WEST, 40);
		Exit ();
	}
	if (Fork () == 0) {
		Delay (20);
		driveRoad (EAST, 40);
		Exit ();
	}

	Delay (450);

	while (1) {
		Delay (900);
		if (Fork () == 0) {
			driveRoad (EAST, 40);
			Exit ();
		}
		Delay (900);
		if (Fork () == 0) {
			driveRoad (WEST, 40);
			Exit ();
		}
	}
}

void Test11() { //corner case
	InitRoad();
	if (Fork () == 0) {
		Delay (5);          // car 2
		driveRoad (WEST, 60);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (10);         // car 3
		driveRoad (EAST, 50);
		Exit ();
	}

	driveRoad (WEST, 10);           // car 1

	Exit ();
}

void Test12() {
	InitRoad();
	if (Fork() == 0) {
		Delay(600);
		driveRoad(WEST, 70);
		Exit();
	}

	if (Fork() == 0) {
		Delay(610);
		driveRoad(WEST, 70);
		Exit();
	}

	if (Fork () == 0) {
		Delay (620);
		driveRoad (WEST, 50);
		Exit ();
	}

	driveRoad (EAST, 50);

	Exit ();
}

void Test13() { //long delay test case
	InitRoad ();

	if (Fork () == 0) {
		Delay (10000);
		driveRoad (EAST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (10000);
		driveRoad (WEST, 30);
		Exit ();
	}
	driveRoad (EAST, 30);
	Exit ();

}

void Test14() { //Tricky one from discussion slides
	InitRoad ();

	if (Fork () == 0) {
		// car 2
		Delay (5);
		driveRoad (WEST, 60);
		Exit ();
	}
	if (Fork () == 0) {
		// car 3
		Delay (10);
		driveRoad (EAST, 50);
		Exit ();
	}
	// car 1
	driveRoad (WEST, 10);
	Exit ();
}

void Test15() {

	/* Sample output with FIFO semaphore:
	W1---------E Car 1 enters at 1 at 10 mph
	Car 2 wants to drive!!!
	Car 4 wants to drive!!!
	Car 6 wants to drive!!!
	Car 3 wants to drive!!!
	Car 7 wants to drive!!!
	Car 8 wants to drive!!!
	Car 10 wants to drive!!!
	Car 9 wants to drive!!!
	W-1--------E Car 1 moves from 1 to 2
	W--1-------E Car 1 moves from 2 to 3
	W---1------E Car 1 moves from 3 to 4
	W----1-----E Car 1 moves from 4 to 5
	W-----1----E Car 1 moves from 5 to 6
	W------1---E Car 1 moves from 6 to 7
	W-------1--E Car 1 moves from 7 to 8
	W--------1-E Car 1 moves from 8 to 9
	W---------1E Car 1 moves from 9 to 10
	W----------E Car 1 exits road
	W---------3E Car 3 enters at 10 at 90 mph
	W--------3-E Car 3 moves from 10 to 9
	W-------3--E Car 3 moves from 9 to 8
	W------3---E Car 3 moves from 8 to 7
	W-----3----E Car 3 moves from 7 to 6
	W----3-----E Car 3 moves from 6 to 5
	W---3------E Car 3 moves from 5 to 4
	W--3-------E Car 3 moves from 4 to 3
	W-3--------E Car 3 moves from 3 to 2
	W3---------E Car 3 moves from 2 to 1
	W----------E Car 3 exits road
	W2---------E Car 2 enters at 1 at 90 mph
	W-2--------E Car 2 moves from 1 to 2
	W--2-------E Car 2 moves from 2 to 3
	W---2------E Car 2 moves from 3 to 4
	W----2-----E Car 2 moves from 4 to 5
	W-----2----E Car 2 moves from 5 to 6
	W------2---E Car 2 moves from 6 to 7
	W-------2--E Car 2 moves from 7 to 8
	W--------2-E Car 2 moves from 8 to 9
	W---------2E Car 2 moves from 9 to 10
	W----------E Car 2 exits road
	W---------8E Car 8 enters at 10 at 90 mph
	W--------8-E Car 8 moves from 10 to 9
	W-------8--E Car 8 moves from 9 to 8
	W------8---E Car 8 moves from 8 to 7
	W-----8----E Car 8 moves from 7 to 6
	W----8-----E Car 8 moves from 6 to 5
	W---8------E Car 8 moves from 5 to 4
	W--8-------E Car 8 moves from 4 to 3
	W-8--------E Car 8 moves from 3 to 2
	W8---------E Car 8 moves from 2 to 1
	W----------E Car 8 exits road
	W4---------E Car 4 enters at 1 at 90 mph
	W-4--------E Car 4 moves from 1 to 2
	W64--------E Car 6 enters at 1 at 90 mph
	W6-4-------E Car 4 moves from 2 to 3
	W-64-------E Car 6 moves from 1 to 2
	W764-------E Car 7 enters at 1 at 5 mph
	W76-4------E Car 4 moves from 3 to 4
	W7-64------E Car 6 moves from 2 to 3
	W7-6-4-----E Car 4 moves from 4 to 5
	W7--64-----E Car 6 moves from 3 to 4
	W7--6-4----E Car 4 moves from 5 to 6
	W7---64----E Car 6 moves from 4 to 5
	Car 5 wants to drive!!!
	W7---6-4---E Car 4 moves from 6 to 7
	W7----64---E Car 6 moves from 5 to 6
	W7----6-4--E Car 4 moves from 7 to 8
	W7-----64--E Car 6 moves from 6 to 7
	W7-----6-4-E Car 4 moves from 8 to 9
	W7------64-E Car 6 moves from 7 to 8
	W7------6-4E Car 4 moves from 9 to 10
	W7-------64E Car 6 moves from 8 to 9
	W7-------6-E Car 4 exits road
	W7--------6E Car 6 moves from 9 to 10
	W7---------E Car 6 exits road
	W-7--------E Car 7 moves from 1 to 2
	W--7-------E Car 7 moves from 2 to 3
	W---7------E Car 7 moves from 3 to 4
	W----7-----E Car 7 moves from 4 to 5
	W-----7----E Car 7 moves from 5 to 6
	W------7---E Car 7 moves from 6 to 7
	W-------7--E Car 7 moves from 7 to 8
	W--------7-E Car 7 moves from 8 to 9
	W---------7E Car 7 moves from 9 to 10
	W----------E Car 7 exits road
	W---------5E Car 5 enters at 10 at 90 mph
	W--------5-E Car 5 moves from 10 to 9
	W-------5--E Car 5 moves from 9 to 8
	W------5---E Car 5 moves from 8 to 7
	W-----5----E Car 5 moves from 7 to 6
	W----5-----E Car 5 moves from 6 to 5
	W---5------E Car 5 moves from 5 to 4
	W--5-------E Car 5 moves from 4 to 3
	W-5--------E Car 5 moves from 3 to 2
	W5---------E Car 5 moves from 2 to 1
	W----------E Car 5 exits road
	W10---------E Car 10 enters at 1 at 90 mph
	W-10--------E Car 10 moves from 1 to 2
	W910--------E Car 9 enters at 1 at 90 mph
	W9-10-------E Car 10 moves from 2 to 3
	W-910-------E Car 9 moves from 1 to 2
	W-9-10------E Car 10 moves from 3 to 4
	W--910------E Car 9 moves from 2 to 3
	W--9-10-----E Car 10 moves from 4 to 5
	W---910-----E Car 9 moves from 3 to 4
	W---9-10----E Car 10 moves from 5 to 6
	W----910----E Car 9 moves from 4 to 5
	W----9-10---E Car 10 moves from 6 to 7
	W-----910---E Car 9 moves from 5 to 6
	W-----9-10--E Car 10 moves from 7 to 8
	W------910--E Car 9 moves f
	*/
	InitRoad ();

	if (Fork () == 0) {
		Delay(5);           //car 2
		DPrintf("Car 2 wants to drive!!!\n");
		driveRoad (WEST, 90);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (10);
		DPrintf("Car 3 wants to drive!!!\n");
		driveRoad (EAST, 90); //car 3
		Exit ();
	}

	if (Fork () == 0) {
		Delay (5);
		DPrintf("Car 4 wants to drive!!!\n");
		driveRoad (WEST, 90); //car 4
		Exit ();
	}

	if (Fork () == 0) {
		Delay (5000);                       // car 5
		DPrintf("Car 5 wants to drive!!!\n");
		driveRoad (EAST, 90);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (5);                  // car 6
		Printf("Car 6 wants to drive!!!\n");
		driveRoad (WEST, 90);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (5);                  // car 7
		DPrintf("Car 7 wants to drive!!!\n");
		driveRoad (WEST, 5);
		Exit ();
	}
	if (Fork () == 0) {
		Delay (10);                 // car 8
		DPrintf("Car 8 wants to drive!!!\n");
		driveRoad (EAST, 90);
		Exit ();
	}
	if (Fork () == 0) {
		Delay (20);                 // car 9
		DPrintf("Car 9 wants to drive!!!\n");
		driveRoad (WEST, 90);
		Exit ();
	}
	if (Fork () == 0) {
		Delay (20);                 // car 10
		DPrintf("Car 10 wants to drive!!!\n");
		driveRoad (WEST, 90);
		Exit ();
	}

	driveRoad (WEST, 10);                 // car 1
	Exit ();
}

void Test16() {
	InitRoad();
// car 2
	if (Fork () == 0) {
		Delay (200);
		driveRoad (WEST, 10000000);
		Exit ();
	}

	// car 1
	driveRoad (WEST, 20);
	Exit ();
}


void Test17() {
	InitRoad();
// car 2
	if (Fork () == 0) {
		Delay (100);
		driveRoad (WEST, 20);
		Exit ();
	}

	// car 3
	if (Fork () == 0) {
		Delay (200);
		driveRoad (EAST, 20);
		Exit ();
	}

	// car 1
	driveRoad (WEST, 20);
	Exit ();
}

void Test18() {
	InitRoad();
// car 2
	if (Fork () == 0) {
		Delay (200);
		driveRoad (EAST, 20);
		Exit ();
	}

	// car 3
	if (Fork () == 0) {
		Delay (300);
		driveRoad (WEST, 20);
		Exit ();
	}

	// car 1
	driveRoad (WEST, 20);
	Exit ();
}

void Test19() {
	InitRoad();
// car 2
	if (Fork () == 0) {
		Delay (10);
		Printf("CAR %d HAS ARRIVED\n", Getpid());
		driveRoad (WEST, 20);
		Exit ();
	}

	// car 3
	if (Fork () == 0) {
		Delay (15);
		Printf("CAR %d HAS ARRIVED\n", Getpid());
		driveRoad (WEST, 20);
		Exit ();
	}

	// car 4
	if (Fork () == 0) {
		Delay (20);
		Printf("CAR %d HAS ARRIVED\n", Getpid());
		driveRoad (WEST, 20);
		Exit ();
	}

	// car 5
	if (Fork () == 0) {
		Delay (30);
		Printf("CAR %d HAS ARRIVED\n", Getpid());
		driveRoad (EAST, 20);
		Exit ();
	}

	// car 6
	if (Fork () == 0) {
		Delay (35);
		Printf("CAR %d HAS ARRIVED\n", Getpid());
		driveRoad (EAST, 20);
		Exit ();
	}

	// car 7
	if (Fork () == 0) {
		Delay (40);
		Printf("CAR %d HAS ARRIVED\n", Getpid());
		driveRoad (EAST, 20);
		Exit ();
	}

	// car 1
	Printf("CAR %d HAS ARRIVED\n", Getpid());
	driveRoad (WEST, 20);
	Exit ();
}

void Test20() {
	InitRoad();
// car 2
	if (Fork () == 0) {
		Delay (5);
		driveRoad (WEST, 20);
		Exit ();
	}

	// car 3
	if (Fork () == 0) {
		Delay (15);
		driveRoad (EAST, 20);
		Exit ();
	}

	// car 1
	driveRoad (WEST, 20);
	Exit ();
}

void Test21() {
	InitRoad();
// car 2
	if (Fork () == 0) {
		Delay (100);
		Printf("CAR %d HAS ARRIVED\n", Getpid ());
		driveRoad (EAST, 10);
		Exit ();
	}

	// car 3
	if (Fork () == 0) {
		Delay (150);
		Printf("CAR %d HAS ARRIVED\n", Getpid ());
		driveRoad (EAST, 10);
		Exit ();
	}

	// car 4
	if (Fork () == 0) {
		Delay (200);
		Printf("CAR %d HAS ARRIVED\n", Getpid ());
		driveRoad (EAST, 10);
		Exit ();
	}

	// car 5
	if (Fork () == 0) {
		Delay (1800);
		Printf("CAR %d HAS ARRIVED\n", Getpid ());
		driveRoad (WEST, 10);
		Exit ();
	}

	//car 1
	Printf("CAR %d HAS ARRIVED\n", Getpid ());
	driveRoad (WEST, 20);
	Exit ();
}

void Test22() {
	InitRoad();
	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	driveRoad(WEST, 670800000);
	Exit ();
}

void Test23() {
	InitRoad();
	if (Fork () == 0) {
		driveRoad(EAST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(EAST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(EAST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(EAST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(EAST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(EAST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(EAST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(EAST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(EAST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(EAST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(EAST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(WEST, 670800000);
		Exit ();
	}

	if (Fork () == 0) {
		driveRoad(EAST, 670800000);
		Exit ();
	}

	driveRoad(WEST, 670800000);
	Exit ();
}


void test_1()
{
	//Road Trace: >|<|>|<|>|<<||
	InitRoad ();

	if (Fork () == 0) {
		Delay (0);            // car 2
		driveRoad (EAST, 60);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (00);            // car 3
		driveRoad (EAST, 50);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (00);            // car 4
		driveRoad (WEST, 30);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (00);            // car 5
		driveRoad (EAST, 60);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (00);            // car 6
		driveRoad (EAST, 50);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (00);            // car 7
		driveRoad (WEST, 30);
		Exit ();
	}

	driveRoad (WEST, 40);            // car 1
	Exit();
}

void test_2()
{
	InitRoad ();

	if (Fork () == 0) {
		Delay (0);            // car 2
		driveRoad (WEST, 60);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (10);            // car 3
		driveRoad (EAST, 50);
		Exit ();
	}

	driveRoad (WEST, 1);            // car 1

	Exit ();

}

void test_3()
{
	InitRoad ();

	if (Fork () == 0) {
		Delay (700);            // car 2
		driveRoad (EAST, 60);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (600);            // car 3
		driveRoad (WEST, 50);
		Exit ();
	}
	if (Fork () == 0) {
		Delay (800);            // car 3
		driveRoad (EAST, 30);
		Exit ();
	}

	driveRoad (WEST, 40);            // car 1

	Exit ();

}

void test_4()
{
	InitRoad ();

	if (Fork () == 0) {
		Delay (5);            // car 2
		driveRoad (EAST, 60);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (10);            // car 3
		driveRoad (WEST, 50);
		Exit ();
	}

	driveRoad (EAST, 10);            // car 1

	Exit ();
}

void test_5()
{
	//output: Road Trace: <|>|<|>|
	//<<||>>|| ? <- don't know

	InitRoad ();

	if (Fork () == 0) {
		Delay (5);            // car 2
		driveRoad (EAST, 60);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (10);            // car 3
		driveRoad (WEST, 50);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (10);            // car 4
		driveRoad (WEST, 50);
		Exit ();
	}


	driveRoad (EAST, 10);            // car 1

	Exit ();
}

void test_6()
{
	InitRoad ();

	if (Fork () == 0) {
		driveRoad (WEST, 40);
		Exit ();
	}
	if (Fork () == 0) {
		driveRoad (WEST, 40);
		Exit ();
	}
	if (Fork () == 0) {
		driveRoad (WEST, 40);
		Exit ();
	}
	if (Fork () == 0) {
		Delay (20);
		driveRoad (EAST, 40);
		Exit ();
	}

	Delay (450);

	while (1) {
		Delay (900);
		if (Fork () == 0) {
			driveRoad (EAST, 40);
			Exit ();
		}
		Delay (900);
		if (Fork () == 0) {
			driveRoad (WEST, 40);
			Exit ();
		}
	}
}

void test_7()
{
	//<<||>>|| and <|>|<|>|
	InitRoad ();

	if (Fork () == 0) {
		Delay (1);            // car 2
		driveRoad (EAST, 3599);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);            // car 3
		driveRoad (WEST, 3599);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (10);            // car 4
		driveRoad (WEST, 3599);
		Exit ();
	}


	driveRoad (EAST, 3599);            // car 1

	Exit ();
}

void test_8()
{
	InitRoad ();

	if (Fork () == 0) {
		Delay (0);            // car 2
		driveRoad (EAST, 3599);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);            // car 3
		driveRoad (WEST, 3599);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (10);            // car 4
		driveRoad (WEST, 3599);
		Exit ();
	}


	driveRoad (EAST, 3599);            // car 1

	Exit ();
}

void test_9()
{
	InitRoad ();

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 10);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 30);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 40);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 50);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 60);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 70);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 80);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (WEST, 90);
		Exit ();
	}

	driveRoad (WEST, 5);

	Exit ();
}

void test_10()
{
	InitRoad ();

	if (Fork () == 0) {
		Delay (1);              // car 2
		driveRoad (WEST, 3599);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (2);              // car 3
		driveRoad (WEST, 3599);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (3);              // car 4
		driveRoad (WEST, 3599);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (4);              // car 5
		driveRoad (WEST, 3599);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (5);              // car 6
		driveRoad (EAST, 3599);
		Exit ();
	}
	if (Fork () == 0) {
		Delay (6);              // car 7
		driveRoad (EAST, 3599);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (7);              // car 8
		driveRoad (EAST, 3599);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (8);              // car 9
		driveRoad (EAST, 3599);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (9);              // car 10
		driveRoad (EAST, 3599);
		Exit ();
	}
	driveRoad (WEST, 3599);     // car 1

	Exit ();
}

void test_11()
{
	InitRoad ();

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 10);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 20);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 30);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 40);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 50);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 60);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 70);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 80);
		Exit ();
	}

	if (Fork () == 0) {
		Delay (0);
		driveRoad (EAST, 90);
		Exit ();
	}

	driveRoad (EAST, 5);

	Exit ();
}



void Test24() {
	InitRoad();
	if (!Fork()) {
		Delay(0); // car 2 should be in the road as well
		driveRoad(WEST, 60);
		Exit();
	}

	if (!Fork()) {
		Delay(300); // now car 3 should wait until car 1 and car 2 finish   |||    Note: it means.. it will be ran/signal after around 600 DelayTime
		driveRoad(EAST, 60);
		Exit();
	}

	if (!Fork()) {
		Delay(600); // car 4 will be on the road with 3   ||| Note: this is good test... to check if you can avoid car 4 hitting 3 at the initial position ( run couple times )
		driveRoad(EAST, 60);
		Exit();
	}
	if (!Fork()) {
		Delay(900); // car 5 will wait until 3&4 finish.. since its opposite direction    ||| Note: It will arrive when 3&4 is already on the road
		driveRoad(WEST, 60);
		Exit();
	}
	if (!Fork()) {
		Delay(1000); // car 6 will enter the road after car 5  ||| Note: this is arrive when 3&4 is around the middle of lane with Car 5 on the waiting list
		driveRoad(EAST, 60);
		Exit();
	}

	if (!Fork()) {
		Delay(1600); // car 7  is on waiting... but car 8 will be out first.   ||| Note: car 7 will arrive when 5 is on the road  and 6 on waiting list
		driveRoad(EAST, 60);
		Exit();
	}
	if (!Fork()) {
		Delay(1800); // car 8 arrives after car 7... but it will be on the road before car 7 ||| Note: car 8 arrives when 5 is on road.  6,7 on the waiting list
		driveRoad(WEST, 60);
		Exit();
	}

	driveRoad(WEST, 60); // car 1 on the road
	Exit();


//expected output:
//>>||<<||>|<|>|<|
}

void Test25() {
	InitRoad();
	int i;
	for ( i = 0; i < 9; i++ ) {
		if ( !Fork() ) {
			Delay(0);
			driveRoad(WEST, 120);
			Exit();
		}
	}
	driveRoad(WEST, 10);

//Expected: >>>>>>>>>>||||||||||
}

void Test26() {
	/* W---------2E Car 2 enters at 10 at 10 mph
	W--------2-E Car 2 moves from 10 to 9
	W-------2--E Car 2 moves from 9 to 8
	W------2---E Car 2 moves from 8 to 7
	W-----2----E Car 2 moves from 7 to 6
	W----2-----E Car 2 moves from 6 to 5
	W---2------E Car 2 moves from 5 to 4
	W--2-------E Car 2 moves from 4 to 3
	W-2--------E Car 2 moves from 3 to 2
	W2---------E Car 2 moves from 2 to 1
	W----------E Car 2 exits road
	W3---------E Car 3 enters at 1 at 11 mph
	W-3--------E Car 3 moves from 1 to 2
	W--3-------E Car 3 moves from 2 to 3
	W---3------E Car 3 moves from 3 to 4
	W----3-----E Car 3 moves from 4 to 5
	W-----3----E Car 3 moves from 5 to 6
	W------3---E Car 3 moves from 6 to 7
	W-------3--E Car 3 moves from 7 to 8
	W--------3-E Car 3 moves from 8 to 9
	W---------3E Car 3 moves from 9 to 10
	W----------E Car 3 exits road
	W---------4E Car 4 enters at 10 at 12 mph
	W--------4-E Car 4 moves from 10 to 9
	W-------4--E Car 4 moves from 9 to 8
	W------4---E Car 4 moves from 8 to 7
	W-----4----E Car 4 moves from 7 to 6
	W----4-----E Car 4 moves from 6 to 5
	W---4------E Car 4 moves from 5 to 4
	W--4-------E Car 4 moves from 4 to 3
	W-4--------E Car 4 moves from 3 to 2
	W4---------E Car 4 moves from 2 to 1
	W----------E Car 4 exits road
	W5---------E Car 5 enters at 1 at 13 mph
	W-5--------E Car 5 moves from 1 to 2
	W--5-------E Car 5 moves from 2 to 3
	W---5------E Car 5 moves from 3 to 4
	W----5-----E Car 5 moves from 4 to 5
	W-----5----E Car 5 moves from 5 to 6
	W------5---E Car 5 moves from 6 to 7
	W-------5--E Car 5 moves from 7 to 8
	W--------5-E Car 5 moves from 8 to 9
	W---------5E Car 5 moves from 9 to 10
	W----------E Car 5 exits road
	W---------6E Car 6 enters at 10 at 14 mph
	W--------6-E Car 6 moves from 10 to 9
	W-------6--E Car 6 moves from 9 to 8
	W------6---E Car 6 moves from 8 to 7
	W-----6----E Car 6 moves from 7 to 6
	W----6-----E Car 6 moves from 6 to 5
	W---6------E Car 6 moves from 5 to 4
	W--6-------E Car 6 moves from 4 to 3
	W-6--------E Car 6 moves from 3 to 2
	W6---------E Car 6 moves from 2 to 1
	W----------E Car 6 exits road
	W7---------E Car 7 enters at 1 at 15 mph
	W-7--------E Car 7 moves from 1 to 2
	W--7-------E Car 7 moves from 2 to 3
	W---7------E Car 7 moves from 3 to 4
	W----7-----E Car 7 moves from 4 to 5
	W-----7----E Car 7 moves from 5 to 6
	W------7---E Car 7 moves from 6 to 7
	W-------7--E Car 7 moves from 7 to 8
	W--------7-E Car 7 moves from 8 to 9
	W---------7E Car 7 moves from 9 to 10
	W----------E Car 7 exits road
	W---------8E Car 8 enters at 10 at 16 mph
	W--------8-E Car 8 moves from 10 to 9
	W-------8--E Car 8 moves from 9 to 8
	W------8---E Car 8 moves from 8 to 7
	W-----8----E Car 8 moves from 7 to 6
	W----8-----E Car 8 moves from 6 to 5
	W---8------E Car 8 moves from 5 to 4
	W--8-------E Car 8 moves from 4 to 3
	W-8--------E Car 8 moves from 3 to 2
	W8---------E Car 8 moves from 2 to 1
	W----------E Car 8 exits road
	W9---------E Car 9 enters at 1 at 17 mph
	W-9--------E Car 9 moves from 1 to 2
	W--9-------E Car 9 moves from 2 to 3
	W---9------E Car 9 moves from 3 to 4
	W----9-----E Car 9 moves from 4 to 5
	W-----9----E Car 9 moves from 5 to 6
	W------9---E Car 9 moves from 6 to 7
	W-------9--E Car 9 moves from 7 to 8
	W--------9-E Car 9 moves from 8 to 9
	W---------9E Car 9 moves from 9 to 10
	W----------E Car 9 exits road
	W---------10E Car 10 enters at 10 at 18 mph
	W--------10-E Car 10 moves from 10 to 9
	W-------10--E Car 10 moves from 9 to 8
	W------10---E Car 10 moves from 8 to 7
	W-----10----E Car 10 moves from 7 to 6
	W----10-----E Car 10 moves from 6 to 5
	W---10------E Car 10 moves from 5 to 4
	W--10-------E Car 10 moves from 4 to 3
	W-10--------E Car 10 moves from 3 to 2
	W10---------E Car 10 moves from 2 to 1
	W----------E Car 10 exits road
	Road Trace: <|>|<|>|<|>|<|>|<|
	*/
	InitRoad();
	int count = 0;
	for (; count < MAXPROCS; ++count)
	{
		if (Fork() == 0)
		{
			Delay(count * 10);
			driveRoad (EAST, count + 10);
			Exit();
		}

		++count;

		if (Fork() == 0)
		{
			Delay(count * 10);
			driveRoad (WEST, count + 10);
			Exit();
		}
	}
	Exit();//I added this
}

void Test27() {
	/* W---------2E Car 2 enters at 10 at 10 mph
	W--------2-E Car 2 moves from 10 to 9
	W-------2--E Car 2 moves from 9 to 8
	W------2---E Car 2 moves from 8 to 7
	W-----2----E Car 2 moves from 7 to 6
	W----2-----E Car 2 moves from 6 to 5
	W---2------E Car 2 moves from 5 to 4
	W--2-------E Car 2 moves from 4 to 3
	W-2--------E Car 2 moves from 3 to 2
	W2---------E Car 2 moves from 2 to 1
	W----------E Car 2 exits road
	W3---------E Car 3 enters at 1 at 11 mph
	W-3--------E Car 3 moves from 1 to 2
	W--3-------E Car 3 moves from 2 to 3
	W---3------E Car 3 moves from 3 to 4
	W----3-----E Car 3 moves from 4 to 5
	W-----3----E Car 3 moves from 5 to 6
	W------3---E Car 3 moves from 6 to 7
	W-------3--E Car 3 moves from 7 to 8
	W--------3-E Car 3 moves from 8 to 9
	W---------3E Car 3 moves from 9 to 10
	W----------E Car 3 exits road
	W---------4E Car 4 enters at 10 at 12 mph
	W--------4-E Car 4 moves from 10 to 9
	W-------4--E Car 4 moves from 9 to 8
	W------4---E Car 4 moves from 8 to 7
	W-----4----E Car 4 moves from 7 to 6
	W----4-----E Car 4 moves from 6 to 5
	W---4------E Car 4 moves from 5 to 4
	W--4-------E Car 4 moves from 4 to 3
	W-4--------E Car 4 moves from 3 to 2
	W4---------E Car 4 moves from 2 to 1
	W----------E Car 4 exits road
	W5---------E Car 5 enters at 1 at 13 mph
	W-5--------E Car 5 moves from 1 to 2
	W--5-------E Car 5 moves from 2 to 3
	W---5------E Car 5 moves from 3 to 4
	W----5-----E Car 5 moves from 4 to 5
	W-----5----E Car 5 moves from 5 to 6
	W------5---E Car 5 moves from 6 to 7
	W-------5--E Car 5 moves from 7 to 8
	W--------5-E Car 5 moves from 8 to 9
	W---------5E Car 5 moves from 9 to 10
	W----------E Car 5 exits road
	W---------1E Car 1 enters at 10 at 6 mph
	W--------1-E Car 1 moves from 10 to 9
	W-------1--E Car 1 moves from 9 to 8
	W------1---E Car 1 moves from 8 to 7
	W-----1----E Car 1 moves from 7 to 6
	W----1-----E Car 1 moves from 6 to 5
	W---1------E Car 1 moves from 5 to 4
	W--1-------E Car 1 moves from 4 to 3
	W-1--------E Car 1 moves from 3 to 2
	W1---------E Car 1 moves from 2 to 1
	W----------E Car 1 exits road
	W7---------E Car 7 enters at 1 at 15 mph
	W-7--------E Car 7 moves from 1 to 2
	W--7-------E Car 7 moves from 2 to 3
	W---7------E Car 7 moves from 3 to 4
	W----7-----E Car 7 moves from 4 to 5
	W-----7----E Car 7 moves from 5 to 6
	W------7---E Car 7 moves from 6 to 7
	W-------7--E Car 7 moves from 7 to 8
	W--------7-E Car 7 moves from 8 to 9
	W---------7E Car 7 moves from 9 to 10
	W----------E Car 7 exits road
	W---------6E Car 6 enters at 10 at 14 mph
	W--------6-E Car 6 moves from 10 to 9
	W-------6--E Car 6 moves from 9 to 8
	W------6---E Car 6 moves from 8 to 7
	W-----6----E Car 6 moves from 7 to 6
	W----6-----E Car 6 moves from 6 to 5
	W---6------E Car 6 moves from 5 to 4
	W--6-------E Car 6 moves from 4 to 3
	W-6--------E Car 6 moves from 3 to 2
	W6---------E Car 6 moves from 2 to 1
	W----------E Car 6 exits road
	W9---------E Car 9 enters at 1 at 17 mph
	W-9--------E Car 9 moves from 1 to 2
	W--9-------E Car 9 moves from 2 to 3
	W---9------E Car 9 moves from 3 to 4
	W----9-----E Car 9 moves from 4 to 5
	W-----9----E Car 9 moves from 5 to 6
	W------9---E Car 9 moves from 6 to 7
	W-------9--E Car 9 moves from 7 to 8
	W--------9-E Car 9 moves from 8 to 9
	W---------9E Car 9 moves from 9 to 10
	W----------E Car 9 exits road
	W---------8E Car 8 enters at 10 at 16 mph
	W--------8-E Car 8 moves from 10 to 9
	W--------810E Car 10 enters at 10 at 18 mph
	W-------8-10E Car 8 moves from 9 to 8
	W-------810-E Car 10 moves from 10 to 9
	W------8-10-E Car 8 moves from 8 to 7
	W------810--E Car 10 moves from 9 to 8
	W-----8-10--E Car 8 moves from 7 to 6
	W-----810---E Car 10 moves from 8 to 7
	W----8-10---E Car 8 moves from 6 to 5
	W----810----E Car 10 moves from 7 to 6
	W---8-10----E Car 8 moves from 5 to 4
	W---810-----E Car 10 moves from 6 to 5
	W--8-10-----E Car 8 moves from 4 to 3
	W--810------E Car 10 moves from 5 to 4
	W-8-10------E Car 8 moves from 3 to 2
	W-810-------E Car 10 moves from 4 to 3
	W8-10-------E Car 8 moves from 2 to 1
	W810--------E Car 10 moves from 3 to 2
	W-10--------E Car 8 exits road
	W10---------E Car 10 moves from 2 to 1
	W----------E Car 10 exits road
	Road Trace: <|>|<|>|<|>|<|>|<<||
	*/
	InitRoad();
	int count = 0;
	for (; count < MAXPROCS; ++count)
	{
		if (Fork() == 0)
		{
			Delay(count * 10);
			driveRoad (EAST, count + 10);
			Exit();
		}

		++count;

		if (Fork() == 0)
		{
			Delay(count * 10);
			driveRoad (WEST, count + 10);
			Exit();
		}
	}
	Delay(50);
	driveRoad(EAST, 6);
	Exit();//I added this
}

void Test28() {
	/* W1---------E Car 1 enters at 1 at 40 mph
	W-1--------E Car 1 moves from 1 to 2
	W--1-------E Car 1 moves from 2 to 3
	W---1------E Car 1 moves from 3 to 4
	W----1-----E Car 1 moves from 4 to 5
	W-----1----E Car 1 moves from 5 to 6
	W------1---E Car 1 moves from 6 to 7
	W-------1--E Car 1 moves from 7 to 8
	W--------1-E Car 1 moves from 8 to 9
	W---------1E Car 1 moves from 9 to 10
	W----------E Car 1 exits road
	W---------2E Car 2 enters at 10 at 60 mph
	W--------2-E Car 2 moves from 10 to 9
	W-------2--E Car 2 moves from 9 to 8
	W------2---E Car 2 moves from 8 to 7
	W-----2----E Car 2 moves from 7 to 6
	W----2-----E Car 2 moves from 6 to 5
	W---2------E Car 2 moves from 5 to 4
	W--2-------E Car 2 moves from 4 to 3
	W-2--------E Car 2 moves from 3 to 2
	W2---------E Car 2 moves from 2 to 1
	W----------E Car 2 exits road
	W4---------E Car 4 enters at 1 at 50 mph
	W-4--------E Car 4 moves from 1 to 2
	W--4-------E Car 4 moves from 2 to 3
	W---4------E Car 4 moves from 3 to 4
	W----4-----E Car 4 moves from 4 to 5
	W-----4----E Car 4 moves from 5 to 6
	W------4---E Car 4 moves from 6 to 7
	W-------4--E Car 4 moves from 7 to 8
	W--------4-E Car 4 moves from 8 to 9
	W---------4E Car 4 moves from 9 to 10
	W----------E Car 4 exits road
	W---------3E Car 3 enters at 10 at 50 mph
	W--------3-E Car 3 moves from 10 to 9
	W-------3--E Car 3 moves from 9 to 8
	W------3---E Car 3 moves from 8 to 7
	W-----3----E Car 3 moves from 7 to 6
	W----3-----E Car 3 moves from 6 to 5
	W---3------E Car 3 moves from 5 to 4
	W--3-------E Car 3 moves from 4 to 3
	W-3--------E Car 3 moves from 3 to 2
	W3---------E Car 3 moves from 2 to 1
	W----------E Car 3 exits road
	W5---------E Car 5 enters at 1 at 50 mph
	W-5--------E Car 5 moves from 1 to 2
	W--5-------E Car 5 moves from 2 to 3
	W---5------E Car 5 moves from 3 to 4
	W----5-----E Car 5 moves from 4 to 5
	W-----5----E Car 5 moves from 5 to 6
	W------5---E Car 5 moves from 6 to 7
	W-------5--E Car 5 moves from 7 to 8
	W--------5-E Car 5 moves from 8 to 9
	W---------5E Car 5 moves from 9 to 10
	W----------E Car 5 exits road
	Road Trace: >|<|>|<|>|
	*/
	InitRoad();
	if (Fork () == 0) {            /* Car 2 */
		Delay (100);
		driveRoad (EAST, 60);
		Exit ();
	}

	if (Fork () == 0) {            /* Car 3 */
		Delay (150);
		driveRoad (EAST, 50);
		Exit ();
	}

	if (Fork () == 0) {            /* Car 4 */
		Delay (200);
		driveRoad (WEST, 50);
		Exit ();
	}
	if (Fork () == 0) {            /* Car 5 */
		Delay (250);
		driveRoad (WEST, 50);
		Exit ();
	}
	driveRoad (WEST, 40);            /* Car 1 */

	Exit ();
}

void Test29() {
	//Road Trace: >>>>>>|>|>|>|>||||||<<<<<<|<|<|<|<||||||
	//Road Trace: >>>>>>>>>>||||||||||<<<<<<<<<<||||||||||
	InitRoad();
	int i;
	for (i = 0; i < 9; ++i) {
		if (!Fork()) {
			Delay(0);
			if (WEST)driveRoad(WEST, 120);
			else driveRoad(EAST, 120);
			Exit();
		}
	}
	/* Car 1 for loop 1 */
	if (WEST)driveRoad(WEST, 120);
	else driveRoad(EAST, 120);
	Delay(2000);
	for (i = 0; i < 9; ++i) {
		if (!Fork()) {
			Delay(0);
			if (WEST)driveRoad(EAST, 120);
			else driveRoad(WEST, 120);
			Exit();
		}
	}
	/* Car 1 for loop 2 */
	if (WEST)driveRoad(EAST, 120);
	else driveRoad(WEST, 120);
	Exit();
}

void Test30() {
	InitRoad();
	int i;
	for ( i = 0; i < 9; i++ ) {
		if ( !Fork() ) {
			Delay(0);
			driveRoad(WEST, 120);
			Exit();
		}
	}
	driveRoad(WEST, 10);
	Exit();
}

void forkCar(int delay, int dir, int mph) {
	if (!Fork()) {
		Delay(delay);
		driveRoad(dir, mph);
		Exit();
	}
}

void Test31() { //Deadlock tester
	//common ones:
	/* My Road Trace [1] : >>>>>>>>>>||||||||||<<<<<<<<<|||||||||>>>>|>|>||>>|>|>||||>>>>|>|>|>|>|>||||>>>>>>>>>>||||||||||<<<<
	My Road Trace [2] : >>>>>>>>>>||||||||||<<<<<<<<<|||||||||>>>>>||>|>||>>||>|||>>>>|>|>|>|>|>||||>>>>>>>>>|>|||||||||<<<<
	My Road Trace [3] : >>>>>>>>>>||||||||||<<<<<<<<<|||||||||>>>>|>|>||>>||>>||||>>>>|>|>|>|>|>||||>>>>>>>>>|>|||||||||<<<<
	*/
	InitRoad();
	int i;
	for ( i = 0; i < 9; i++)
		forkCar(0, WEST, 120);
	driveRoad(WEST, 120);
	Delay(2000);
	for ( i = 0; i < 9; i++)
		forkCar(0, EAST, 120);
	Delay(200);
	for (int i = 0; i < 3; i++)
		forkCar(0, WEST, 130);
	for (i = 0; i < 3; i++)
		forkCar(100, EAST, 120);
	for (i = 0; i < 3; i++)
		forkCar(200, WEST, 140);
	Delay(900);
	for (i = 0; i < 9; i++)
		forkCar(100 * i + 1, WEST, 120);
	driveRoad(WEST, 120);
	Delay(900);
	for (i = 0; i < 9; i++)
		forkCar(100 * i + 1, WEST, 100);
	Delay(2000);
	for ( i = 0 ; i < 9; i++)
		forkCar(30 + i * 10, WEST, 110);
	for ( i = 0 ; i < 9; i++)
		forkCar(10 + 10 * i, EAST, 140);
	driveRoad(WEST, 120);
	for ( i = 0 ; i < 9; i++)
		forkCar(10 + 10 * i, EAST, 130);
	Delay(350);
	for ( i = 0 ; i < 9; i++)
		forkCar(10 + 40 * i, EAST, 120);
	Delay(450);
	for ( i = 0 ; i < 9; i++)
		forkCar(10 + 50 * i, EAST, 150);
	Delay(550);
	for ( i = 0 ; i < 9; i++)
		forkCar(30 + 20 * i, WEST, 130);
	Delay(650);
	for ( i = 0 ; i < 9; i++)
		forkCar(50 + 30 * i, WEST, 110);
	Delay(1000);
	for ( i = 0 ; i < 9; i++)
		forkCar(0, WEST, 120);
	driveRoad(WEST, 120);
	Exit();
}

void Test32() { //speed test
	//<<<<<<|<|<|<|<||||||

	/* W---------1E Car 1 enters at 10 at 120 mph
	W--------1-E Car 1 moves from 10 to 9
	W--------12E Car 2 enters at 10 at 120 mph
	W-------1-2E Car 1 moves from 9 to 8
	W-------12-E Car 2 moves from 10 to 9
	W-------123E Car 3 enters at 10 at 120 mph
	W------1-23E Car 1 moves from 8 to 7
	W------12-3E Car 2 moves from 9 to 8
	W------123-E Car 3 moves from 10 to 9
	W------1234E Car 4 enters at 10 at 120 mph
	W-----1-234E Car 1 moves from 7 to 6
	W-----12-34E Car 2 moves from 8 to 7
	W-----123-4E Car 3 moves from 9 to 8
	W-----1234-E Car 4 moves from 10 to 9
	W-----12345E Car 5 enters at 10 at 120 mph
	W----1-2345E Car 1 moves from 6 to 5
	W----12-345E Car 2 moves from 7 to 6
	W----123-45E Car 3 moves from 8 to 7
	W----1234-5E Car 4 moves from 9 to 8
	W----12345-E Car 5 moves from 10 to 9
	W----123456E Car 6 enters at 10 at 120 mph
	W---1-23456E Car 1 moves from 5 to 4
	W---12-3456E Car 2 moves from 6 to 5
	W---123-456E Car 3 moves from 7 to 6
	W---1234-56E Car 4 moves from 8 to 7
	W---12345-6E Car 5 moves from 9 to 8
	W---123456-E Car 6 moves from 10 to 9
	W---1234567E Car 7 enters at 10 at 120 mph
	W--1-234567E Car 1 moves from 4 to 3
	W--12-34567E Car 2 moves from 5 to 4
	W--123-4567E Car 3 moves from 6 to 5
	W--1234-567E Car 4 moves from 7 to 6
	W--12345-67E Car 5 moves from 8 to 7
	W--123456-7E Car 6 moves from 9 to 8
	W--1234567-E Car 7 moves from 10 to 9
	W--12345678E Car 8 enters at 10 at 120 mph
	W-1-2345678E Car 1 moves from 3 to 2
	W-12-345678E Car 2 moves from 4 to 3
	W-123-45678E Car 3 moves from 5 to 4
	W-1234-5678E Car 4 moves from 6 to 5
	W-12345-678E Car 5 moves from 7 to 6
	W-123456-78E Car 6 moves from 8 to 7
	W-1234567-8E Car 7 moves from 9 to 8
	W-12345678-E Car 8 moves from 10 to 9
	W-123456789E Car 9 enters at 10 at 120 mph
	W1-23456789E Car 1 moves from 2 to 1
	W12-3456789E Car 2 moves from 3 to 2
	W123-456789E Car 3 moves from 4 to 3
	W1234-56789E Car 4 moves from 5 to 4
	W12345-6789E Car 5 moves from 6 to 5
	W123456-789E Car 6 moves from 7 to 6
	W1234567-89E Car 7 moves from 8 to 7
	W12345678-9E Car 8 moves from 9 to 8
	W123456789-E Car 9 moves from 10 to 9
	W12345678910E Car 10 enters at 10 at 120 mph
	W-2345678910E Car 1 exits road
	W2-345678910E Car 2 moves from 2 to 1
	W23-45678910E Car 3 moves from 3 to 2
	W234-5678910E Car 4 moves from 4 to 3
	W2345-678910E Car 5 moves from 5 to 4
	W23456-78910E Car 6 moves from 6 to 5
	W234567-8910E Car 7 moves from 7 to 6
	W2345678-910E Car 8 moves from 8 to 7
	W23456789-10E Car 9 moves from 9 to 8
	W2345678910-E Car 10 moves from 10 to 9
	W-345678910-E Car 2 exits road
	W3-45678910-E Car 3 moves from 2 to 1
	W34-5678910-E Car 4 moves from 3 to 2
	W345-678910-E Car 5 moves from 4 to 3
	W3456-78910-E Car 6 moves from 5 to 4
	W34567-8910-E Car 7 moves from 6 to 5
	W345678-910-E Car 8 moves from 7 to 6
	W3456789-10-E Car 9 moves from 8 to 7
	W345678910--E Car 10 moves from 9 to 8
	W-45678910--E Car 3 exits road
	W4-5678910--E Car 4 moves from 2 to 1
	W45-678910--E Car 5 moves from 3 to 2
	W456-78910--E Car 6 moves from 4 to 3
	W4567-8910--E Car 7 moves from 5 to 4
	W45678-910--E Car 8 moves from 6 to 5
	W456789-10--E Car 9 moves from 7 to 6
	W45678910---E Car 10 moves from 8 to 7
	W-5678910---E Car 4 exits road
	W5-678910---E Car 5 moves from 2 to 1
	W56-78910---E Car 6 moves from 3 to 2
	W567-8910---E Car 7 moves from 4 to 3
	W5678-910---E Car 8 moves from 5 to 4
	W56789-10---E Car 9 moves from 6 to 5
	W5678910----E Car 10 moves from 7 to 6
	W-678910----E Car 5 exits road
	W6-78910----E Car 6 moves from 2 to 1
	W67-8910----E Car 7 moves from 3 to 2
	W678-910----E Car 8 moves from 4 to 3
	W6789-10----E Car 9 moves from 5 to 4
	W678910-----E Car 10 moves from 6 to 5
	W-78910-----E Car 6 exits road
	W7-8910-----E Car 7 moves from 2 to 1
	W78-910-----E Car 8 moves from 3 to 2
	W789-10-----E Car 9 moves from 4 to 3
	W78910------E Car 10 moves from 5 to 4
	W-8910------E Car 7 exits road
	W8-910------E Car 8 moves from 2 to 1
	W89-10------E Car 9 moves from 3 to 2
	W8910-------E Car 10 moves from 4 to 3
	W-910-------E Car 8 exits road
	W9-10-------E Car 9 moves from 2 to 1
	W910--------E Car 10 moves from 3 to 2
	W-10--------E Car 9 exits road
	W10---------E Car 10 moves from 2 to 1
	W----------E Car 10 exits road*/
	InitRoad();
	int i;
	for (i = 0; i < 9; i++)
		forkCar(0, EAST, 120);
	driveRoad(EAST, 120);
	Exit();
}

void Test33() {
	/* W---------1E Car 1 enters at 10 at 2000 mph
	W--------1-E Car 1 moves from 10 to 9
	W-------1--E Car 1 moves from 9 to 8
	W------1---E Car 1 moves from 8 to 7
	W-----1----E Car 1 moves from 7 to 6
	W-----1---2E Car 2 enters at 10 at 720 mph
	W----1----2E Car 1 moves from 6 to 5
	W---1-----2E Car 1 moves from 5 to 4
	W--1------2E Car 1 moves from 4 to 3
	W--1-----2-E Car 2 moves from 10 to 9
	W--1----2--E Car 2 moves from 9 to 8
	W--1----2-3E Car 3 enters at 10 at 720 mph
	W-1-----2-3E Car 1 moves from 3 to 2
	W1------2-3E Car 1 moves from 2 to 1
	W-------2-3E Car 1 exits road
	W-------23-E Car 3 moves from 10 to 9
	W-------234E Car 4 enters at 10 at 720 mph
	W------2-34E Car 2 moves from 8 to 7
	W------23-4E Car 3 moves from 9 to 8
	W------234-E Car 4 moves from 10 to 9
	W------2345E Car 5 enters at 10 at 720 mph
	W-----2-345E Car 2 moves from 7 to 6
	W-----23-45E Car 3 moves from 8 to 7
	W-----234-5E Car 4 moves from 9 to 8
	W-----2345-E Car 5 moves from 10 to 9
	W-----23456E Car 6 enters at 10 at 720 mph
	W----2-3456E Car 2 moves from 6 to 5
	W----23-456E Car 3 moves from 7 to 6
	W----234-56E Car 4 moves from 8 to 7
	W----2345-6E Car 5 moves from 9 to 8
	W----23456-E Car 6 moves from 10 to 9
	W----234567E Car 7 enters at 10 at 720 mph
	W---2-34567E Car 2 moves from 5 to 4
	W--2--34567E Car 2 moves from 4 to 3
	W--2-3-4567E Car 3 moves from 6 to 5
	W--2-34-567E Car 4 moves from 7 to 6
	W--2-345-67E Car 5 moves from 8 to 7
	W--2-3456-7E Car 6 moves from 9 to 8
	W--2-34567-E Car 7 moves from 10 to 9
	W--2-345678E Car 8 enters at 10 at 720 mph
	W-2--345678E Car 2 moves from 3 to 2
	W-2-3-45678E Car 3 moves from 5 to 4
	W2--3-45678E Car 2 moves from 2 to 1
	W2-3--45678E Car 3 moves from 4 to 3
	W2-3-4-5678E Car 4 moves from 6 to 5
	W2-3-45-678E Car 5 moves from 7 to 6
	W2-3-456-78E Car 6 moves from 8 to 7
	W2-3-4567-8E Car 7 moves from 9 to 8
	W2-3-45678-E Car 8 moves from 10 to 9
	W2-3-456789E Car 9 enters at 10 at 720 mph
	W--3-456789E Car 2 exits road
	W--34-56789E Car 4 moves from 5 to 4
	W--345-6789E Car 5 moves from 6 to 5
	W--3456-789E Car 6 moves from 7 to 6
	W--34567-89E Car 7 moves from 8 to 7
	W--345678-9E Car 8 moves from 9 to 8
	W--3456789-E Car 9 moves from 10 to 9
	W--345678910E Car 10 enters at 10 at 720 mph
	W-3-45678910E Car 3 moves from 3 to 2
	W-34-5678910E Car 4 moves from 4 to 3
	W-345-678910E Car 5 moves from 5 to 4
	W-3456-78910E Car 6 moves from 6 to 5
	W-34567-8910E Car 7 moves from 7 to 6
	W-345678-910E Car 8 moves from 8 to 7
	W-3456789-10E Car 9 moves from 9 to 8
	W-345678910-E Car 10 moves from 10 to 9
	W3-45678910-E Car 3 moves from 2 to 1
	W34-5678910-E Car 4 moves from 3 to 2
	W345-678910-E Car 5 moves from 4 to 3
	W3456-78910-E Car 6 moves from 5 to 4
	W34567-8910-E Car 7 moves from 6 to 5
	W345678-910-E Car 8 moves from 7 to 6
	W3456789-10-E Car 9 moves from 8 to 7
	W345678910--E Car 10 moves from 9 to 8
	W-45678910--E Car 3 exits road
	W4-5678910--E Car 4 moves from 2 to 1
	W45-678910--E Car 5 moves from 3 to 2
	W456-78910--E Car 6 moves from 4 to 3
	W4567-8910--E Car 7 moves from 5 to 4
	W45678-910--E Car 8 moves from 6 to 5
	W456789-10--E Car 9 moves from 7 to 6
	W45678910---E Car 10 moves from 8 to 7
	W-5678910---E Car 4 exits road
	W5-678910---E Car 5 moves from 2 to 1
	W56-78910---E Car 6 moves from 3 to 2
	W567-8910---E Car 7 moves from 4 to 3
	W5678-910---E Car 8 moves from 5 to 4
	W56789-10---E Car 9 moves from 6 to 5
	W5678910----E Car 10 moves from 7 to 6
	W-678910----E Car 5 exits road
	W6-78910----E Car 6 moves from 2 to 1
	W67-8910----E Car 7 moves from 3 to 2
	W678-910----E Car 8 moves from 4 to 3
	W6789-10----E Car 9 moves from 5 to 4
	W678910-----E Car 10 moves from 6 to 5
	W-78910-----E Car 6 exits road
	W7-8910-----E Car 7 moves from 2 to 1
	W--8910-----E Car 7 exits road
	W-8-910-----E Car 8 moves from 3 to 2
	W-89-10-----E Car 9 moves from 4 to 3
	W-8910------E Car 10 moves from 5 to 4
	W8-910------E Car 8 moves from 2 to 1
	W89-10------E Car 9 moves from 3 to 2
	W8910-------E Car 10 moves from 4 to 3
	W-910-------E Car 8 exits road
	W9-10-------E Car 9 moves from 2 to 1
	W910--------E Car 10 moves from 3 to 2
	W-10--------E Car 9 exits road
	W10---------E Car 10 moves from 2 to 1
	W----------E Car 10 exits road
	Road Trace: <<<|<<<<<<|<||||||||*/
	InitRoad();
	int i;
	for (i = 0; i < 9; i++)
		forkCar(0, EAST, 720);
	driveRoad(EAST, 2000);
	Exit();
}
