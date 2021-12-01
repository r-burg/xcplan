/******************************************************************************

								map.cc

	Copyright (C) 2019 Roger Burghall all rights reserved.

	Version 0.1 22nd February 2019
		Initial version


******************************************************************************/

/// WARNING! This program must output nothing to standard output except the name
/// of the (temporary) image file! and that must come from a non-blocking function
/// i.e. 'write( )'.



#if COMMENTS
#include <stdio.h>
#endif
#include <stdlib.h>
#include <fcntl.h>
#include <cmath>
#include <string>
#include <string.h>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <gtk/gtk.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "map.h"



tp tps[10000];						/// List of turn points in the selected region.
int n_tps = 0;						/// Number of turn points in the selected region.
char tp[5][64];						/// Codes for turn points in the task.
int tpi[4] = { -1, -1, -1, -1 };			/// Turnpoint indices for the task.
int nlegs = 0;						/// Task must have 2, 3 or 4 legs.
/// Screen height and width.
static 	int height = IMG_HEIGHT, width = IMG_WIDTH, xmid, ymid;
/// task latitude and longitude limits.
double lat1, long1, lat2, long2;
static double scalex, scaley, long_mid, lat_mid;
static double scale;
/// The cosine of the weighted mean longitude is used for horizontal scaling.
static double cosine, cosine1, cosine2;
char region[64];
double task_distance;


double square(double x)
{
  return(x * x);
}



/** Accurate but complex recursive method of computing the distance between points
 on an ellipsoid.
*/
double Vincenty(double lat1, double lon1, double lat2, double lon2 /*, double &bearing*/)
{
	void ToRad(char *ptr, double &frad);
	double a, b;                                  	/// Major and minor semiaxes of the ellipsoid.
	a = 6378137.0;
	b = 6356752.314245;
	double f = (a - b) / a;                       	/// Flattening.
//	assert((f > 1.0 / 298.257224) && (f < 1.0 / 298.257223));
	double Phi1 = lat1,
	Phi2 = lat2;                         			/// Geodetic latitude.
	double L = lon2-lon1;       /// Difference in longitude.
	double U1 = atan((1.0 - f) * tan(Phi1));      	/// "Reduced latitude" = atan((1 - f) * tan(Phi1)).
	double U2 = atan((1.0 - f) * tan(Phi2));      	/// "Reduced latitude" = atan((1 - f) * tan(Phi2)).
	double Lambda = L;                              /// = L as first approximation.
	double Lambda1, diff;
	double sigma, sinsigma, cossigma, deltasigma;
	double sinalpha, cossqalpha, cos2sigmam, C;
	double alpha1, alpha2;
	double usq, A, B, s;
	double sinlambda, coslambda;
	double sinU1 = sin(U1), sinU2 = sin(U2), cosU1 = cos(U1), cosU2 = cos(U2);
	double i = 0, n = 100;

	do {
		sinlambda = sin(Lambda), coslambda = cos(Lambda);
		sinsigma = sqrt(square(cosU2 * sinlambda) + square(cosU1 * sinU2 - sinU1 * cosU2 * coslambda));
		if(sinsigma == 0.0) return(0.0);
		cossigma = sinU1 * sinU2 + cosU1 * cosU2 * coslambda;
		sigma = atan2(sinsigma, cossigma);
		sinalpha = cosU1 * cosU2 * sinlambda / sinsigma;
		cossqalpha = 1.0 - sinalpha * sinalpha;
		cos2sigmam = cossigma - 2.0 * sinU1 * sinU2 / cossqalpha;
		C = f / 16.0 * cossqalpha * (4.0 + f *(4.0 - 3.0 * cossqalpha));
		Lambda1 = L + (1.0 - C) * f * sinalpha * (sigma + C * sinsigma * (cos2sigmam + C * cossigma * (-1.0 + 2.0 * cos2sigmam * cos2sigmam)));
		diff = Lambda - Lambda1;
		Lambda = Lambda1;
//    cout << "Lambda = " << Lambda << "\n";
		if(++i > n) break;
	} while(fabs(diff) > 1e-13);
	  
	usq = cossqalpha * (a * a - b * b) / (b * b);
	A = 1.0 + usq / 16384.0 * (4096.0 + usq * (-768.0 + usq * (320.0 -175.0 * usq)));
	B = usq / 1024.0 * (256.0 + usq * (-128.0 + usq * (74.0 - 47.0 * usq)));
	deltasigma = B * sinsigma * (cos2sigmam + B / 4.0 * (cossigma * (-1.0 + 2.0 * cos2sigmam * cos2sigmam) - B / 6.0 * cos2sigmam * (-3.0 + 4.0 * sinsigma * sinsigma) * (-3.0 + 4.0 * cos2sigmam * cos2sigmam)));
	s = b * A * (sigma - deltasigma);
	alpha1 = atan2(cosU2 * sin(Lambda), cosU1 * sinU2 - sinU1 * cosU2 * cos(Lambda));
	alpha2 = atan2(cosU1 * sin(Lambda), -sinU1 * cosU2 + cosU1 * sinU2 * cos(Lambda));
//    bearing = (alpha1+alpha2)/2.0;
	return(s / 1000.0);
}




/// Parse a turning point line into name, code and location strings.
bool parse(string line, string &name, string &code, string &location)
{
	size_t i, j;						/// If line is "abc,def, , ghi"
	string dump;
	if(line.length( ) == 0) return(false);
	i = line.find(",");					/// then i = 3
	if(i < 0) return(false);
	name = line.substr(0, i);			/// name = "abc"
	j = i;
	i = line.find(",", i+1);			/// i = 7, j = 3
	if(i < 0) return(false);
	code = line.substr(j+1, i-j-1);		/// code = "def"
	j = i;
	i = line.find(",", i+1);
	if(i < 0) return(false);
	dump = line.substr(j+1, i-j-1);		/// 
	j = i;
	i = line.find(",", i+1);
	if(i < 0) return(false);
	dump = line.substr(j+1, i-j-1);		/// 
	j = i;
	i = line.find(",", i+1);
	if(i < 0) return(false);
	dump = line.substr(j+1, i-j-1);		/// 
	j = i;
	location = line.substr(j+1);		/// location = "ghi"
	return(true);
}



/** Turn the position string into two 'doubles', for latitude and longitude. */
bool LatLong(char *pos, double &Lat, double &Long)
{
	int i, j, l, signN, signE;
	char *str1, *degreesN, *minutesN, *degreesE, *minutesE;
	char *ptr1, *ptr2;


	// pos = "54 13.728N 001 12.580W\"
	// If the separators are colons, assume dd:mm:ss otherwise dd mm.mmm
	ptr2 = strchr(pos, ':');
/// Set pointers to degrees and minutes North, and to degrees and minutes East.
	if(ptr2 == NULL) {
		degreesN = pos;
		minutesN = strchr(pos, ' ');
		*minutesN++ = '\0';				// Terminate degrees North, increment to point at minutes.
		degreesE = strchr(minutesN, ' ');
		*degreesE++ = '\0';				// Terminate minutes, increment to point at degrees East
		minutesE = strchr(degreesE, ' ');
		*minutesE++ = '\0';				// Terminate degrees East, increment to point at minutes.
	} else {
		return(false);
	}
	/// Check signs to be applied.
	signN = (strchr(minutesN, 'S')? -1: 1);
	signE = (strchr(minutesE, 'W')? -1: 1);
	/// Convert to magnitudes.
	double northing = atof(degreesN) + atof(minutesN) / 60.0;
	double easting = atof(degreesE) + atof(minutesE) / 60.0;
	/// Apply signs.
	northing *= signN;
	easting *= signE;
	Lat = northing;
	Long = easting;

	return(true);
}



/// Clear the turning point combobox.
void clear_tps(void)
{

	n_tps = 0;
}



/** Remove leading quotation mark if there is one. */
string dequote(string str)
{
	int i, l;
	string str1;
	i = str.find("\"");
	if(i < 0) return(str);
	l = str.length( );
	str1 = str.substr(1, l - 2);
	return(str1);
}



int rd_line(int filedes, char *buffer, int maxlen)
{
	char buffer1[3];
	int i = 0, j = 0;
	do {
		i = read(filedes, buffer1, 1);
		if(i == 0) {
			// End of file!
			buffer[j] = '\0';
			if(j == 0) return(EOF);
			return(j);
		}
		if(*buffer1 == '\n') {
			buffer[j] = '\0';
			return(0);
		} else buffer[j++] = *buffer1;
	} while(i);

	return(j);
}



/** Read TP file and store TPs in 'tps[]'. */
void ReadTPs(string filename)
{
	string dequote(string str);
	bool LatLong(char *pos, double &Lat, double &Long);
	int res;
	int i;
	char loc[64];
	char line[256];
	string name, code, location;
	int infile;
	infile = open(filename.c_str( ), O_RDONLY);
	if(infile) {
		i = 0;
		while(1) {
			res = rd_line(infile, line, 255);
			if(parse(line, name, code, location)) {
				location = dequote(location);
				code = dequote(code);
				strncpy(tps[i].name, name.c_str( ), 31); strncpy(tps[i].code, code.c_str( ), 15); strncpy(tps[i].location, location.c_str( ), 63);
				strcpy(loc, location.c_str( ));
				LatLong(loc, tps[i].latitude, tps[i].longitude);
				i++;
			}
			if(res == EOF) break;
		}
		n_tps = i;
	}
	close(infile);
}



/** Copy TP codes from 'tps[]' to Start/End combobox. */
void CopyToSE(void)
{
	int i, se = 1;
	for(i=0; i<n_tps; i++) {

	}
}



/// Read the TP file and fill the TP table.
bool FillTps(string filename)
{
	clear_tps( );									/// Erase old TPs
	ReadTPs(filename);								/// Read new TP file to 'tps[]'.
	CopyToSE( );									/// Copy TP codes to start/end combobox.

	return(TRUE);
}



/// Find the index of a specified TP code.
int find_tp(char *code)
{
	int i;
	for(i = 0; i < n_tps; i++) {
		if(strcmp(tps[i].code, code) == 0) return(i);
	}
}



/// Convert latitude and longitude to map coordinates.
bool MapToScreen(double East, double North, int &x, int &y)
{
	x = xmid + (East - long_mid) * scale * cosine;
	y = ymid - (North - lat_mid) * scale;

	if(x < 0) return false;
	if(y < 0) return false;
	if(x >= width) return false;
	if(y >= height) return false;
	return true;
}



/** Calculate radius of circular airspace in pixels. */
double RadiusOnScreen(double Nm)
{
	// How many pixels to 'Nm' nautical miles?
	// 'scale' is pixels per degree of latitude;
	// Earth's "radius" is about (6378137.0 + 6356752.3) / 2.0;
	// there are (6080 / 3.281) m per nm
	double m = Nm * (6080 / 3.281);					// Airspace radius on Earth in metres.
	double c = (6378137.0 + 6356752.3) * M_PI;			// circumference of the Earth
	c /= 360.0;							// One degree in metres
	m /= c;								// Airspace radius in degrees
	return(scale * m);						// Airspace radius in degrees	
}



double radius(double x, double y)
{
	return(sqrt(x*x+y*y));
}



/// Convert 'C' string to double.
void ToRad(char *ptr, double &frad)
{
	char *p = ptr;
	frad = atof(ptr);
}



double ToRad(double degrees)
{
	double frad;
	frad = degrees * M_PI / 180.0;
	return(frad);
}



//        3PI/2
//          |
//      -,- | +,-
//          |
//  PI------------- 0
//          |
//      -,+ | +,+
//          |
//         PI/2

double angle(double x, double y)
{
	double alpha;
	double mx, my;
	/// Get magnitudes, x and y.
	mx = (x >= 0.0)? x: -x;
	my = (y >= 0.0)? y: -y;
														  
	if(x == 0.0) alpha = M_PI/2.0;
	else alpha = atan2(my, mx);
	if(x < 0.0) alpha = M_PI - alpha;

	if(y < 0.0) alpha = 2.0 * M_PI - alpha;

	return(alpha);
}



/** Calculate 'Northings' from 'C' string. */
char *dmsN(char *ptr, double& lat0)
{
char *p = ptr;
double d=0, m=0, s=0;
	/// Get degrees
	while(isdigit(*p) ) {
		char c = *p++;
		d = d * 10 + c-'0';
	}
	if(*p++ != ':') return(ptr);
	/// Get minutes
	while(isdigit(*p) ) {
		char c = *p++;
		m = m * 10 + c-'0';
	}
	if(*p++ != ':') return(ptr);
	/// Get seconds
	while(isdigit(*p) ) {
		char c = *p++;
		s = s * 10 + c-'0';
	}
	lat0 = d + m/60.0 + s/3600.0;
	/// North or South?
	while(*p == ' ') p++;
	if(*p++ == 'S') lat0 = -lat0;
	/// Return pointer
	return(ptr = p);
}



/** Calculate 'Eastings' from 'C' string. */
char *dmsE(char *ptr, double& long0)
{
char *p = ptr;
double North;
double East;
double d=0, m=0, s=0;
	while(*p == ' ') p++;
	/// Get degrees
	while(isdigit(*p) ) {
		char c = *p++;
		d = d * 10 + c-'0';
	}
	if(*p++ != ':') return(ptr);
	/// Get minutes
	while(isdigit(*p) ) {
		char c = *p++;
		m = m * 10 + c-'0';
	}
	if(*p++ != ':') return(ptr);
	/// Get seconds
	while(isdigit(*p) ) {
		char c = *p++;
		s = s * 10 + c-'0';
	}
	long0 = d + m/60.0 + s/3600.0;
	/// East or West?
	while(*p == ' ') p++;
	if(*p++ == 'W') long0 = -long0;

	/// Return pointer
	return(ptr = p);
}



/** Get the latitudes and longitudes of the rectangle enclosing the task. */
bool GetTaskLimits(double &nmin, double &emin, double &nmax, double &emax)
{
	bool f1, f2, f3;
	string tri;
	double n1, e1, n2, e2, n3, e3, n4, e4;
	double Ht, Wh;
	char t1[9], t2[9], t3[9];
	if(nlegs == 0) {
		return(false);
	}
	n1 = tps[tpi[0]].latitude;
	e1 = tps[tpi[0]].longitude;
	n2 = tps[tpi[1]].latitude;
	e2 = tps[tpi[1]].longitude;
	if(nlegs >= 3) {
		n3 = tps[tpi[2]].latitude;
		e3 = tps[tpi[2]].longitude;
	}
	if(nlegs == 4) {
		n4 = tps[tpi[3]].latitude;
		e4 = tps[tpi[3]].longitude;
	}
	nmax = nmin = n1;
	emax = emin = e1;
	nmax = (n2 > nmax)? n2: nmax;
	nmin = (n2 < nmin)? n2: nmin;
	emax = (e2 > emax)? e2: emax;
	emin = (e2 < emin)? e2: emin;
	if(nlegs >= 3) {
		nmax = (n3 > nmax)? n3: nmax;
		nmin = (n3 < nmin)? n3: nmin;
		emax = (e3 > emax)? e3: emax;
		emin = (e3 < emin)? e3: emin;
	}
	if(nlegs == 4) {
		nmax = (n4 > nmax)? n4: nmax;
		nmin = (n4 < nmin)? n4: nmin;
		emax = (e4 > emax)? e4: emax;
		emin = (e4 < emin)? e4: emin;
	}
	Ht = nmax - nmin;
	Wh = emax - emin;
	nmax = nmax - Ht / 16;
	nmin = nmin + Ht / 16;
	emax = emax - Wh / 16;
	emin = emin + Wh / 16;
	return(true);
}



/// Draw all turningpoints in their right places.
bool DrawTps(cairo_t *cr)
{
char *Get_Tri(int i);
double Get_Lat(int i);
double Get_Long(int i);

	int i;
	char Tri[16]; // = (char *)"___";
	int x, y;
	double North=53.0, East=1.0;

	for(i=1; i<2000; i++) {

		/// Get Trigraph.
		strncpy(Tri, tps[i].code, 15);
		if(Tri[0] == '\0') return true;
		if(strcmp(Tri, "") == 0) return true;

		/// Get turning point latitude and longitude.
		North = tps[i].latitude;
		East = tps[i].longitude;

		/// Don't continue if out of screen area.
		if(North < lat1 - 1.0) continue;
		if(North > lat2 + 1.0) continue;
		if(East < long1 - 1.0) continue;
		if(East > long2 + 1.0) continue;

///  Find where that is on the window, and place it.
		if(MapToScreen(East, North, x, y)) {
			cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
			cairo_move_to (cr, x+4, y);
			cairo_arc(cr, x, y, 4, 0, 2.0*M_PI);
			cairo_move_to (cr, x+6, y);
			cairo_show_text (cr, Tri);
		}
	}
	return false;
}



/** Processes one line of data from the coast file, drawing a line from the previous point to the new one
unless starting a new shape. */
void ProcessCoastData(cairo_t *cr, char *pstr)
{
	double lat1, long1;
	static int start_x, start_y, end_x, end_y;
	int contiguous;

	cairo_set_source_rgb (cr, 0.5, 0.5, 1.0);

	sscanf(pstr, "%lf,%lf, %d", &long1, &lat1, &contiguous);

	MapToScreen(long1, lat1, end_x, end_y);
	if(contiguous) {
		cairo_line_to(cr, end_x, end_y);
	} else {
		cairo_move_to(cr, end_x, end_y);
	}
	start_x = end_x;
	start_y = end_y;
}



/** Draw coastline/borders from region ".coast" file. */
bool DrawCoast(cairo_t *cr)
{
void Mapping(void);
	int coastfile;
	int res;
	gchar file_name[256];
	char line[256], *pstr=NULL;
	size_t size=128;
	strncpy(file_name, region, 120);
	strcat(file_name, ".coast");
	if((coastfile=open(file_name, O_RDONLY)) >= 0) {
		pstr = line;
		/// Get a line
		do {
			while(1) {
				res = rd_line(coastfile, line, 255);
//				getline((char **)&pstr, (size_t *)&size, coastfile );
				if(*pstr != '*') break;
			}
			/// Now have a non-comment line call for it to be processed.
			ProcessCoastData(cr, pstr);
			*pstr = '\0';
		} while(res != EOF);
		/// File finished, close it.
		close(coastfile);
	} else {
		return(FALSE);
	}
	return(TRUE);
}



/** Processes one line of data from the coast file, drawing a line from the previous point to the new one
unless starting a new shape. */
void ProcessTownData(cairo_t *cr, char *pstr)
{
	double lat1, long1;
	static int start_x, start_y, end_x, end_y;
	int contiguous;

	cairo_set_source_rgb (cr, 0.75, 0.75, 0.75);
//	gdk_gc_set_line_attributes (gc2, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);

	sscanf(pstr, "%lf,%lf, %d", &long1, &lat1, &contiguous);

	MapToScreen(long1, lat1, end_x, end_y);
	if(contiguous) {
		cairo_line_to(cr, end_x, end_y);
	} else {
		cairo_fill (cr);
		cairo_move_to(cr, end_x, end_y);
	}
	start_x = end_x;
	start_y = end_y;
}



/** Draw coastline/borders from region ".coast" file. */
bool DrawTown(cairo_t *cr)
{
void Mapping(void);
	int townfile;
	int res;
	char file_name[256];
	char line[256];
	char *pstr=line;
	size_t size=128;
	strncpy(file_name, region, 120);
	strcat(file_name, ".town");
	if((townfile=open(file_name, O_RDONLY)) >= 0) {
		/// Get a line
		do {
			while(1) {
				res = rd_line(townfile, line, 255);
//				getline((char **)&pstr, (size_t *)&size, townfile );
				if(*pstr != '*') break;
			}
			/// Now have a non-comment line call for it to be processed.
			ProcessTownData(cr, pstr);
			*pstr = '\0';
		} while(res != EOF);
		/// File finished, close it.
		close(townfile);
	} else {
		return(FALSE);
	}
	return(TRUE);
}



static bool newshape;
/** Draw the airspace described in the ".air" file. Types include Restricted, Danger, 
Prohibited, ClassA, ClassB, ClassC, ClassD, ClassF, ClassG, NoGlider, Ctr and WaveBox */
bool ProcessAirData(cairo_t *cr, char *pstr)
{
	static enum AirspaceType at;
	char *ptr = pstr;
	static char name[64];
	int len = strlen(pstr);
	double frad;
	int x, y;
	int low;
	static int start_x, centre_x, start_y, centre_y, end_x, end_y;	// start_x, start_y define the last position, from which the next line is drawn... I think!
	static double lat0, long0, lat1, long1, lat2, long2;
	double alpha, beta, d_angle;
	static int rad;
	static int direction = CLOCKWISE;
	int temp;

	*(pstr + len-1) = '\0';

	//* ******** Is it an AC command? ("Airspace Class") *********
	if(strncmp(pstr, "AC", 2) == 0) {
		cairo_stroke (cr);
		ptr += 3;
		newshape = true;
		direction = CLOCKWISE;
		if(strncmp(ptr, "R", 1) == 0) {
			at = Restricted;				/// R --> Purple
			cairo_set_source_rgb (cr, 1.0, 0.0, 1.0);
		}
		else if(strncmp(ptr, "GP", 2) == 0) {
			at = NoGlider;					/// GP --> Purple
			cairo_set_source_rgb (cr, 1.0, 0.0, 1.0);
		}
		else if(strncmp(ptr, "CTR", 3) == 0) {
			at = Ctr;						/// CTR --> Purple
			cairo_set_source_rgb (cr, 1.0, 0.0, 1.0);
		}
		else if(strncmp(ptr, "Q", 1) == 0) {
			at = Danger;					/// Q --> Purple
			cairo_set_source_rgb (cr, 1.0, 0.0, 1.0);
		}
		else if(strncmp(ptr, "P", 1) == 0) {
			at = Prohibited;				/// P --> Purple
			cairo_set_source_rgb (cr, 1.0, 0.0, 1.0);
		}
		else if(strncmp(ptr, "A", 1) == 0) { 
			at = ClassA; 					/// A --> Purple
			cairo_set_source_rgb (cr, 1.0, 0.0, 1.0);
		}
		else if(strncmp(ptr, "B", 1) == 0) {
			at = ClassB; 					/// B --> Purple
			cairo_set_source_rgb (cr, 1.0, 0.0, 1.0);
		}
		else if(strncmp(ptr, "C", 1) == 0) {
			at = ClassC; 					/// C --> Purple
			cairo_set_source_rgb (cr, 1.0, 0.0, 1.0);
		}
		else if(strncmp(ptr, "D", 1) == 0) {
			at = ClassD; 					/// D --> Purple
			cairo_set_source_rgb (cr, 1.0, 0.0, 1.0);
		}
		else if(strncmp(ptr, "E", 1) == 0) {
			at = ClassD; 					/// E --> Purple
			cairo_set_source_rgb (cr, 1.0, 0.0, 1.0);
		}
		else if(strncmp(ptr, "W", 1) == 0) {
			at = WaveBox;					/// W --> Pale green
			cairo_set_source_rgb (cr, 0.5, 1.0, 0.5);
		}
		else if(strncmp(ptr, "F", 1) == 0) {
			at = Uncontrolled;				/// F --> Pale green
			cairo_set_source_rgb (cr, 0.5, 1.0, 0.5);
		}
		else if(strncmp(ptr, "G", 1) == 0) {
			at = Uncontrolled;				/// G --> Pale green
			cairo_set_source_rgb (cr, 0.5, 1.0, 0.5);
		}
		else {
			at = Unknown;
			cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
			return(FALSE);
		}

	} else if(strncmp(pstr, "AN ", 3) == 0) {
		/// AN: Airspace name.
		ptr += 3;
		strncpy(name, ptr, 63);
		name[strlen(ptr)] = '\0';
		// ***********
		newshape = true;
	} else if(strncmp(pstr, "AT ", 3) == 0) {
		/// AN: Show name on map at location
		ptr += 3;
		int n0, n1, n2, e0, e1, e2;
		char c1, c2;
		double n, e;
		int nn, ee;
		if(name[0]) {
			sscanf(ptr, "%d:%d:%d %c %d:%d:%d %c", &n0, &n1, &n2, &c1, &e0, &e1, &e2, &c2);
			n = n0 + (n1 / 60.0) + (n2 / 3600.0);
			e = e0 + (e1 / 60.0) + (e2 / 3600.0);
			if(c1 == 'S') n = -n;
			if(c2 == 'W') e = -e;
//			MapToScreen(widget, e, n, ee, nn);
//			gdk_draw_string(widget->window, Font1, widget->style->black_gc, ee, nn, name);
			name[0] = '\0';
		}

	} else if(strncmp(pstr, "AL ", 3) == 0) {
		// ********* AL: Airspace base. *********
		ptr += 3;
		if(strncmp(ptr, "SFC", 3) == 0) low = 0;
		else if(strncmp(ptr, "FL", 2) == 0) {
			ptr += 2;
			low = atoi(ptr) * 100;	
// Yes, yes... I know. I'm ignoring QNH. I can't
// know what it will be when you use the map!
		} else low = atoi(ptr);
		if(low < THRESHOLD_HEIGHT) switch(at) {
			case Restricted:
			case Danger:
			case Prohibited:
			case ClassA:
			case ClassB:
			case ClassC:
			case ClassD:
			case NoGlider:
			case Ctr:
			cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
			break;
			case Uncontrolled:
			cairo_set_source_rgb (cr, 0.5, 1.0, 0.5);
			default: break;
		}
	} else if(strncmp(pstr, "AH ", 3) == 0) {
		/// ********* AH: Airspace top. *********
		ptr += 3;
	} else if(strncmp(pstr, "V ", 2) == 0) {
		/// ********* V: Variable assignment. *********
		ptr += 2;
		if(strncmp(ptr, "X", 1) == 0) {
			/// X: Variable is X, centre for an arc or circle.
			ptr +=2;
			// Get position
			ptr = dmsN(ptr, lat0);
			ptr = dmsE(ptr, long0);
			MapToScreen(long0, lat0, centre_x, centre_y);
			int n0, n1, n2, e0, e1, e2;
			char c1, c2;
			double n, e;
			int nn, ee;
		if(name[0]) {
			sscanf(ptr, "%d:%d:%d %c %d:%d:%d %c", &n0, &n1, &n2, &c1, &e0, &e1, &e2, &c2);
			n = n0 + (n1 / 60.0) + (n2 / 3600.0);
			e = e0 + (e1 / 60.0) + (e2 / 3600.0);
			if(c1 == 'S') n = -n;
			if(c2 == 'W') e = -e;
			cairo_move_to(cr, centre_x, centre_y);
			cairo_show_text (cr, name);
			cairo_stroke (cr);
//			MapToScreen(widget, e, n, ee, nn);
//			gdk_draw_string(widget->window, Font1, widget->style->black_gc, ee, nn, name);
			name[0] = '\0';
		}
		} else if(strncmp(ptr, "D", 1) == 0) {
			/// Variable is D, direction of arc.
			ptr +=2;
			if(*ptr == '+') direction = CLOCKWISE;
			if(*ptr == '-') direction = ANTICLOCKWISE;
		}

	} else if(strncmp(pstr, "DP ", 3) == 0) {
		/// ********* DP: Polygon data point. *********
		ptr += 3;
		ptr = dmsN(ptr, lat0);
		ptr = dmsE(ptr, long0);
		MapToScreen(long0, lat0, end_x, end_y);
		if(newshape) {
			// Move to first point.
			newshape = false;
			cairo_move_to(cr, end_x, end_y);
		} else {
			// Draw line to next point.
			cairo_line_to(cr, end_x, end_y);
		}
		start_x = end_x, start_y = end_y;
	} else if(strncmp(pstr, "DA ", 3) == 0) {
		/// ********* DA: Add an arc. *********
		// DA radius, start_angle, endangle.
		ptr += 3;
		ptr = dmsN(ptr, lat0);
		ptr = dmsE(ptr, long0);
		MapToScreen(long0, lat0, end_x, end_y);
	} else if(strncmp(pstr, "DB ", 3) == 0) {
		/// ********* DB: Add an arc. *********
		// DB lat_start, long_start, lat_end, long_end.
		ptr += 3;
		ptr = dmsN(ptr, lat1);
		ptr = dmsE(ptr, long1);
		ptr++;
		while(*ptr == ' ') ptr++;
		ptr = dmsN(ptr, lat2);
		ptr = dmsE(ptr, long2);

		/// Draw line to next point.
		MapToScreen(long1, lat1, start_x, start_y);
		MapToScreen(long2, lat2, end_x, end_y);
		rad = (int)(radius(start_x-centre_x, start_y-centre_y)+radius(end_x-centre_x, end_y-centre_y))/2;

		cairo_move_to(cr, start_x, start_y);

/// Calculate start and end angles for the arc.
		alpha = angle(start_x-centre_x, start_y-centre_y);
		beta = angle(end_x-centre_x, end_y-centre_y);


/// Swap start and end positions.
		if(direction == CLOCKWISE) {
			cairo_arc(cr, centre_x, centre_y, rad, alpha, beta);
		} else {
			cairo_arc_negative(cr, centre_x, centre_y, rad, alpha, beta);
		}

#if SHOW_CHORD
		cairo_move_to(cr, start_x, start_y);
		cairo_line_to(cr, end_x, end_y);
#endif
			start_x = end_x, start_y = end_y;
																
	} else if(strncmp(pstr, "DC ", 3) == 0) {
		/// ********* DC: Draw circle. *********
		ptr += 3;
		/// What is the radius?
		ToRad(ptr, frad);
		rad = (int)RadiusOnScreen(frad);
		/// What is the centre?
		MapToScreen(long0, lat0, x, y);
		// Draw the circle.
		cairo_new_path(cr);
		cairo_arc (cr, x, y, rad, 0.0, 2.0*M_PI);
		newshape = true;
	} else if(strncmp(pstr, "DY ", 3) == 0) {
		/// ********* DY: Add airway segment. *********
		ptr += 3;
	}
}



/** Read airspace file and pass the data to ProcessAirData to draw. */
bool DrawAirspace(cairo_t *cr)
{
	int airfile;
	int res;
	char line[256];
	char *pstr=line;
	size_t size=128;

	char filename[128];
	strcpy(filename, region);
	strcat(filename, ".air");
	if((airfile=open(filename, O_RDONLY)) >= 0) {
		/// Get a line
		do {
			while(1) {
				res = rd_line(airfile, line, 255);
//				getline((char **)&pstr, (size_t *)&size, airfile );
				if(*pstr != '*') break;
			}
			/// Now have a non-comment line; process it.
			ProcessAirData(cr, pstr);
//			cairo_stroke (cr);
			*pstr = '\0';
		} while(res != EOF);
		/// File finished, close it.
		close(airfile);
	} else {
		return(FALSE);
	}
	return(TRUE);
}



/// Draw the task route.
bool DrawTask(cairo_t *cr)
{
//char *Get_Tri(int i);
//double Get_Lat(int i);
//double Get_Long(int i);
bool MapToScreen(double, double, int &, int &);

//GdkColor colour;
char t1[16], t2[16], t3[16];	// Turning point trigraphs
char t4[16];
double n1, e1, n2, e2, n3, e3, n4, e4;	// Northing and Eastings for tps

	int x1, y1, x2, y2, x3, y3, x4, y4;

	n1 = tps[tpi[0]].latitude;
	n2 = tps[tpi[1]].latitude;
	n3 = tps[tpi[2]].latitude;
	n4 = tps[tpi[3]].latitude;
	e1 = tps[tpi[0]].longitude;
	e2 = tps[tpi[1]].longitude;
	e3 = tps[tpi[2]].longitude;
	e4 = tps[tpi[3]].longitude;

	cairo_set_source_rgb (cr, 0.0, 0.75, 0.0);

	MapToScreen(e1, n1, x1, y1);
	MapToScreen(e2, n2, x2, y2);
	MapToScreen(e3, n3, x3, y3);
	MapToScreen(e4, n4, x4, y4);
	cairo_move_to(cr, x1, y1);
	cairo_line_to(cr, x2, y2);
	
	switch(nlegs) {
		case 2: break;
		case 3: cairo_line_to(cr, x3, y3);
		 break;
		case 4: 
		 cairo_line_to(cr, x3, y3);
		 cairo_line_to(cr, x4, y4);
		 break;
	}
	cairo_line_to(cr, x1, y1);
	cairo_stroke(cr);

	return true;
}



/// Create a '.png' file showing a map of coasts, borders, towns, turnpoints and airspace.
bool DrawMap(char *Img_name)
{
	char Img[64];
	char str1[96], str2[64];
	cairo_text_extents_t extents;
	strncpy(Img, Img_name, 60);
//	strcat(Img, ".png");
	/// Create the surface we will draw on. Make it 1840 by 960 pixels.
	cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, IMG_WIDTH, IMG_HEIGHT);
	cairo_t *cr = cairo_create(surface);

	// Set surface to white.
	cairo_set_source_rgb (cr, 0.99, 0.99, 0.99);
	cairo_paint (cr);

	/// Select surface colour to white.
	cairo_set_source_rgb (cr, 0.99, 0.99, 0.99);
	cairo_set_line_width (cr, 2);

	/// Draw coastline
	cairo_set_source_rgb (cr, 0.75, 0.75, 0);
	DrawCoast(cr);
	cairo_stroke (cr);

	/// Draw towns
	cairo_set_source_rgb (cr, 0.75, 0.75, 0);
	DrawTown(cr);
	cairo_stroke (cr);

	/// Draw airspace
	DrawAirspace(cr);
	cairo_stroke (cr);

	/// Draw task
	DrawTask(cr);
	cairo_stroke (cr);

/// Draw turnpoints.
	DrawTps(cr);
	cairo_stroke (cr);

	/// Simple method of drawing text. (Probably not right way for actual use!)
	cairo_select_font_face (cr, "san serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (cr, 20.0);

	// Select surface colour to blue.
	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);

	/// Write copyright message.
	strcpy(str1, "Copyright (c) 2019 Roger Burghall. Issued under GPL v3.0 - No liability accepted.");
	cairo_move_to (cr, 10.0, IMG_HEIGHT - 40);
	cairo_text_extents(cr, str1, &extents);
	cairo_move_to(cr, 10, IMG_HEIGHT - 20);
	cairo_line_to(cr, 10 + extents.width, IMG_HEIGHT - 20);
	cairo_line_to(cr, 10 + extents.width, IMG_HEIGHT - 20 - extents.height);
	cairo_line_to(cr, 10, IMG_HEIGHT - 20 - extents.height);
	cairo_line_to(cr, 10, IMG_HEIGHT - 20);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.75);
	cairo_fill(cr);
	cairo_move_to (cr, 10.0, IMG_HEIGHT - 20);
	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	cairo_show_text (cr, str1);
	cairo_stroke(cr);

	cairo_move_to (cr, 10.0, 20.0);
	switch(nlegs) {
		case 2: sprintf(str1, "%s,%s,%s", tp[1], tp[2], tp[1]); break;
		case 3: sprintf(str1, "%s,%s,%s,%s", tp[1], tp[2], tp[3], tp[1]); break;
		case 4: sprintf(str1, "%s,%s,%s,%s,%s", tp[1], tp[2], tp[3], tp[4], tp[1]); break;
		default: exit(-1);
	}

	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
//	cairo_move_to(cr, 10, IMG_HEIGHT - 20);
//	cairo_show_text (cr, str1);
//	cairo_stroke(cr);

	/// Write task summary.
	switch(nlegs) {
		case 2: sprintf(str2, "  (%s,%s)", tps[tpi[0]].name, tps[tpi[1]].name); break;
		case 3: sprintf(str2, "  (%s,%s,%s)", tps[tpi[0]].name, tps[tpi[1]].name, tps[tpi[2]].name); break;
		case 4: sprintf(str2, "  (%s,%s,%s,%s)", tps[tpi[0]].name, tps[tpi[1]].name, tps[tpi[2]].name, tps[tpi[3]].name); break;
		default: exit(-1);
	}

	strncat(str1, str2, 64);

	sprintf(str2, " %0.2f km.", task_distance);
	strncat(str1, str2, 16);

	cairo_text_extents(cr, str1, &extents);
	cairo_move_to(cr, 10, 20);
	cairo_line_to(cr, 10+extents.width, 20);
	cairo_line_to(cr, 10+extents.width, 20-extents.height);
	cairo_line_to(cr, 10, 20-extents.height);
	cairo_line_to(cr, 10, 20);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.75);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	cairo_move_to(cr, 10, 20);
	cairo_show_text (cr, str1);
	cairo_stroke(cr);

	/// Done with cr.
	cairo_destroy (cr);
	/// Draw surface to a .png file.
	cairo_surface_write_to_png (surface, Img);
	cairo_surface_destroy (surface);
}



void Mapping(void)
{
	char str[129];
	/** Determine the latitude and longitude ranges needed. */
	if(!GetTaskLimits(lat1, long1, lat2, long2)) lat1=53.00, lat2=54.5, long1=-2.0, long2=0.0;
	// Let's see the task extrema...
/** 'cosine1' and 'cosine2' show how much closer together the lines of longitude are than lines of latitude
	at the top and bottom of the task area.
*/
	cosine1 = fabs(cos(M_PI * lat1 / 180.0));
	cosine2 = fabs(cos(M_PI * lat2 / 180.0));
	cosine = (cosine1 + cosine2) / 2.0;

/// Calculate how many pixels per degree.
	scalex = width / (long2 - long1);
	scaley = height / (lat2 - lat1);
	scale = (scalex > scaley)? scaley: scalex;
	scale *= 0.8;

	xmid = width / 2;
	ymid = height /2;
	lat_mid = (lat2 + lat1) / 2.0;
	long_mid = (long2 + long1) / 2.0;

}



int main(int argc, char **argv)
{
	char task[64], *ptr, *ptr1, str1[96];
	int i=0, j=0;
	char *lock_file_name, image_file_name[64];
	char unique_file[64] = "xcplan_XXXXXX";
//	double nmin, emin, nmax, emax;
	string tp_file;
	int reterr;

	strncpy(task, argv[1], 63);
	ptr = task;
	
	while(*ptr) {
		if(*ptr == ',') {
			tp[i][j] = '\0';
			i++;
			j = 0;
			ptr++;
		}
		tp[i][j++] = *ptr++;
	}
	tp[i][j] = '\0';
	j = 0;
	nlegs = i;

	// Read the turning point file for region tp[0].
	tp_file = tp[0];
	strncpy(region, tp[0], 56);
	ReadTPs(tp_file + ".tps");

	// Find indices for the turning points of the task,
	for(j = 0; j < i; j++) {
		tpi[j] = find_tp(tp[j+1]);
	}

	task_distance = Vincenty(ToRad(tps[tpi[0]].latitude), ToRad(tps[tpi[0]].longitude), ToRad(tps[tpi[1]].latitude), ToRad(tps[tpi[1]].longitude));
	switch(nlegs) {
		case 2: task_distance *= 2.0;
		 break;	
		case 3: task_distance += Vincenty(ToRad(tps[tpi[1]].latitude), ToRad(tps[tpi[1]].longitude), ToRad(tps[tpi[2]].latitude), ToRad(tps[tpi[2]].longitude));
		 task_distance += Vincenty(ToRad(tps[tpi[2]].latitude), ToRad(tps[tpi[2]].longitude), ToRad(tps[tpi[0]].latitude), ToRad(tps[tpi[0]].longitude));
		 break;	
		case 4: task_distance += Vincenty(ToRad(tps[tpi[1]].latitude), ToRad(tps[tpi[1]].longitude), ToRad(tps[tpi[2]].latitude), ToRad(tps[tpi[2]].longitude));
		 task_distance += Vincenty(ToRad(tps[tpi[2]].latitude), ToRad(tps[tpi[2]].longitude), ToRad(tps[tpi[3]].latitude), ToRad(tps[tpi[3]].longitude));
		 task_distance += Vincenty(ToRad(tps[tpi[3]].latitude), ToRad(tps[tpi[3]].longitude), ToRad(tps[tpi[0]].latitude), ToRad(tps[tpi[0]].longitude));
		 break;	
		default: 
		break;
	}

	// Find map limits for the task
	Mapping( );
//	GetTaskLimits(lat1, long1, lat2, long2);
	/// Select 'Images' directory.

	char cwd[128], *pcwd;
	pcwd = getcwd(cwd, 127);
//	int res = chdir("./Images");

	/// Get a unique file name for locking purposes; the image file will have ".png" appended.
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	lock_file_name = mktemp((char *)unique_file);
	int lock_file = open(lock_file_name, O_RDWR | O_CREAT, mode);

	if(lock_file) {
		close(lock_file);
		if(chmod(lock_file_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
	}

	/// Now we need the corresponding image file name.
	if(isspace(lock_file_name[strlen(lock_file_name)-1])) lock_file_name[strlen(lock_file_name)-1] = '\0';
	strcpy(image_file_name, lock_file_name);
	strcat(image_file_name, ".png");

	// Draw map in file
	DrawMap(image_file_name);

///	Send image_file_name to GUI.
	write(1, image_file_name, strlen(image_file_name));

	strcpy(str1, "/home/roger/CppTuition/24 Gnome/TurnPoint/Web_xcplan/Test/XC-cleaner");
//	strcat(str1, ".png");
	chmod(str1, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	pid_t pID = fork();
	if(pID == 0) {
		/// If fork returns 0 this is the child process...
		/// ...so we substitute "./child" image for "./fork"
//		reterr = execl("./XC-cleaner", "XC-cleaner", str1, lock_file_name, NULL);

		return(0);
	} else if(pID < 0) {
	} else {
		/// This is the parent process. The child's i.d. is pID.
	}

//	res = chdir(cwd);
	return(0);
}


