


#include <iostream>
#include <fstream>
#include <string>


using namespace std;

/** Maximum number of turn points can be handled? */
#define N_TPS	10000
#define TRI_LEN	9

#define THRESHOLD_HEIGHT	4000

#define CLOCKWISE	1
#define ANTICLOCKWISE	2

#define RADIANS *360*64/(M_PI*2.0)

#define IMG_WIDTH	1600
#define IMG_HEIGHT	900

#define COMMENTS false
#define MAX_COMMENTS	false

#define SHOW_READ	false

#define SHOW_CHORD	false

#define SHOW_MAP_TO_SCREEN	false


/** Type of airspace. */
enum AirspaceType { Unknown, Restricted, Danger, Prohibited,
	ClassA, ClassB, ClassC, ClassD, NoGlider, Ctr, WaveBox, Uncontrolled };


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
	position& operator= (position& p1){ lat = p1.lat; lon = p1.lon; l = p1.l; };
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




