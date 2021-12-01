/****************************************************************************************

TurningPoints.h
---------------

Version 0.1 11/10/2018 (11th October 2018)

Copyright (C) Roger Burghall 2018

This software is issued under the General Public Licence, GPL3

No liability is accepted for this software whether in regard for correctness
of its output or for any other reason.

Development stage 0.07: Trying to rebuild.

	Version 0.12	7/12/2018 RB
		First usable triangular task calculation.

	Version 0.2 RB: Addition of out and return task calculation.

	Version 0.3 RB: Addition of 4 legged tasks

	Version 0.31 RB: Corrected/improved pre-selection of TPs.

This software is made available under the conditions of GPL v3


****************************************************************************************/

#ifndef TURNINGPOINTSH
#define TURNINGPOINTSH

#include <iostream>
#include <fstream>
#include <string>



#define COMMENTS false
#define SHOW_TASK false
#define REJECT_THIN_TRIANGLES	true


using namespace std;

/** The Haversine method is a crude and somewhat inaccurate way of calculating the distance between 
two points on the Earth. The Vincenty algorithm is a more complex, iterative, calculation, but as
accurate as is desired (assuming the Earth to be an ellipsoid!). We use the Vincenty algorithm.
*/
#define EarthRadius ((6378137.0 + 6356752.3) / 2.0)
#define HAVERSINE false


/** How much over the minimum distance is acceptable? */
#define MAX_DIST_FACT	1.25
#define MIN_LEG		0.15
#define MAX_SOLUTIONS	50

/** Maximum number of turn points can be handled? */
#define N_TPS	10000
#define TRI_LEN	9


bool GetTaskNames(int i, char *Description);
double GetTaskDistance(int selected);


/** Turn point structure. */
struct tp {
	double latitude, longitude;
	char name[32], code[16], location[64];
	bool LatLong(string pos, double &Lat, double &Long);
	bool LatLong(string pos);
	tp operator=(tp *tp1);
};



/// 'position' describes a point on the WGS84 datum ellipsoid
struct position {
	double lat, lon;
	double l;
	position(void) { };
	position(double lat1, double long1) { lat = lat1; lon = long1; };
//	position operator= (tp& tp1) { lat = tp1.get_lat( ); lon = tp1.get_long( ); };
	position operator= (tp *tp1) { lat = tp1->latitude; lon = tp1->longitude; return(*this); };
	position& operator= (position& p1){ lat = p1.lat; lon = p1.lon; l = p1.l; return(*this); };
	double operator- (position&);
	void GetLatLong(double &lat1, double &long1) { lat1 = lat, long1 = lon; };
};


/// 'task' defines a group of turning points constituting a task and its distance
struct task {
	tp *tp1, *tp2, *tp3, *tp4;
	double distance;
	int legs;
	double nmin, emin, nmax, emax;
	public:
	task( ) { };
	task(tp&, tp&, double&, int);
	task(tp&, tp&, tp&, double&, int);
	task(tp*, tp*, tp*, double, int);
	task operator=(task *t1);
	void show_task(void);
};




#endif
