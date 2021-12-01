/****************************************************************

			list_tasks.cc

	Version 0.1 RB	30/04/2019

****************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
// #include <cairo.h>
#include <cmath>
#include <cstdlib>
#include "list_tasks.h"

#define VERSION 0.1

#define TEST1	false
#define SHOW	false


#define MAX_LAT		57.0
#define MIN_LAT		49.0
#define MAX_LONG	4.0
#define MIN_LONG	-11.0


char region[256];
char countries[32][256];
int count = 0;
int legs = 3;
double tdistance = 300.0;					/// Desired minimum task distance, defaults to 300.0.
char start[16];
bool fai_flag = false, t25_45_flag = false;
int flags = 15;
static int selected = 0;
task tasks[256]; 						/// Array of possible tasks.
task *ptask = tasks;
tp tps[2500];							/// Array of turning points.
int n_tps;							/// Number of turning points in 'tps[]'.
tp etps[2000];							/// Array of turning points in range
int en_tps;							/// Number of turning points in 'etps[]'.
char filename[256];
int tasks_found = 0;
int n = 0;

bool task2(char *, double);					/// Trawl for O/R tasks meeting the requirements, sorted by distance.
bool task3(void);
bool task3(char *, double);					/// Trawl for triangular tasks meeting the requirements, sorted by distance.
bool task4(char *, double);					/// Trawl for butterfly tasks meeting the requirements, sorted by distance.



void planlog(string str)
{
#if 0
	int file;
	char buffer[256];
	strncpy(buffer, str.c_str(), 255);
	buffer[255] = '\0';
	file = open("/var/www/xcplan/planlog.txt", O_RDWR | O_CREAT | O_APPEND, S_IWOTH);
	if(file) {
		write(file, buffer, strlen(buffer));
		close(file);
	}
#else
	std::ofstream file;
	file.open("/var/www/xcplan/planlog.txt", std::ofstream::out | std::ofstream::app);
	if(file.is_open( )) {
		file << str << "\n";
		file.close( );
	}
#endif
}



double toRad(double degrees)
{
  return(degrees * M_PI / 180.0);
}



void ToRad(char *ptr, double &frad)
{
	char *p = ptr;
	frad = atof(ptr);
}



/** The minus operator returns the distance between two points. */
double position::operator- (position& pos1)
{
	double Vincenty(double lat1, double lon1, double lat2, double lon2);
	double lat1, long1, lat2, long2;

	lat1 = this->lat;
	lat2 = pos1.lat;
	long1 = this->lon;
	long2 = pos1.lon;

	l = Vincenty(toRad(lat1), toRad(long1), toRad(lat2), toRad(long2));

	return(l);
}



/// Clear the turning point combobox.
void clear_tps(void)
{
#if false
	int i;
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox_tp), -1);
	for(i = 0; i <= n_tps; i++) gtk_combo_box_text_remove(combobox_tp, 0);
//	n_tps = 1;
#endif
}



/// Parse a turning point line into name, code and location strings.
bool parse(string line, string &name, string &code, string &location)
{
	size_t i, j;					/// If line is "abc,def, , ghi"
	string dump;
	if(line.length( ) == 0) return(false);
	i = line.find(",");				/// then i = 3
	if(i < 0) return(false);
	name = line.substr(0, i);			/// name = "abc"
	j = i;
	i = line.find(",", i+1);			/// i = 7, j = 3
	if(i < 0) return(false);
	code = line.substr(j+1, i-j-1);			/// code = "def"
	j = i;
	i = line.find(",", i+1);
	if(i < 0) return(false);
	dump = line.substr(j+1, i-j-1);			/// 
	j = i;
	i = line.find(",", i+1);
	if(i < 0) return(false);
	dump = line.substr(j+1, i-j-1);			/// 
	j = i;
	i = line.find(",", i+1);
	if(i < 0) return(false);
	dump = line.substr(j+1, i-j-1);			/// 
	j = i;
	location = line.substr(j+1);			/// location = "ghi"
	return(true);
}



/** Turn the position string into two 'doubles', for latitude and longitude. */
bool LatLong(char *pos, double &Lat, double &Long)
{
	int i, j, l, signN, signE;
	char *str1, *degreesN, *minutesN, *degreesE, *minutesE;
	char *ptr1, *ptr2;

#if COMMENTS
	cout << "tp::LatLong(" << pos << ")\n";
#endif

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
		cout << "WARNING! Format not implemented.\n";
		return(false);
	}
	/// Check signs to be applied.
	signN = (strchr(minutesN, 'S')? -1: 1);
	signE = (strchr(minutesE, 'W')? -1: 1);
#ifdef SHOW_READ
	cout << "<" << degreesN << " " << minutesN << "; " << degreesE << " " << minutesE << ">\n";
#endif
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



/** Read TP file and store TPs in 'tps[]'. */
void ReadTPs(string filename)
{
	string dequote(string str);
	bool LatLong(char *pos, double &Lat, double &Long);

#if COMMENTS
	cout << "ReadTPs\n";
#endif
// exit(0);

	planlog("Read TPs "+ filename);

	int i;
	char loc[64];
	string line, name, code, location;
	ifstream infile;
	infile.open(filename.c_str( ));
	if(infile) {
		i = 0;
		while(!infile.eof()) {
			getline(infile, line);
			if(parse(line, name, code, location)) {
#if COMMENTS
				cout << "name: " << name << ", code: " << code << ", location: " << location << "\n";
#endif
				location = dequote(location);
				code = dequote(code);
//				if(code == "SUT") {
//					strcpy(start, code.c_str( ));
//				}
				strncpy(tps[i].name, name.c_str( ), 31); strncpy(tps[i].code, code.c_str( ), 15); strncpy(tps[i].location, location.c_str( ), 63);
				strcpy(loc, location.c_str( ));
				LatLong(loc, tps[i].latitude, tps[i].longitude);
				i++;
			}
		}
		n_tps = i;
		if(i == 100) planlog("100th TP");

	}
	infile.close( );
}



/** Copy TP codes from 'tps[]' to Start/End combobox. */
void CopyToSE(void)
{
#if false
	int i, se = 1;
	for(i=0; i<n_tps; i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(combobox_tp), tps[i].code);
	}
#endif
}



/// Compare the tasks for distance; used to sort tasks.
int compare_tasks(const void *p1, const void *p2)
{
	double l1, l2;
	task *pt1, *pt2;
	pt1 = (task *)p1, pt2 = (task *)p2;
	l1 = pt1->distance;
	l2 = pt2->distance;
	return((l1 > l2)? 1: (l1 == l2)? 0: -1);
}



/** Search the turn point array for the specified TP code. */
int get_tpi(tp *ptps, int ntps, char *str1)
{
	int i;
	for(i=0; i<ntps; i++) if(strcmp(ptps[i].code,  str1) == 0) return(i);
	return(-1);
}



bool plan( )
{
	bool Flag = false;
	char region1[256];
	strncpy(region1, region, 250);
	strcat(region1, ".tps");
	string filename(region1);
	clear_tps( );									/// Erase old TPs
//	strncpy(filename1, region, 250);
//	strcat(filename1, ".tps");
//	strcpy(filename, filename1);
	planlog("Read TPs");

	ReadTPs(filename);								/// Read new TP file to 'tps[]'.
	planlog("TPs read");

	if(tdistance > 1500.0) return(false);
	switch(legs) {
		case 2: if(task2(start, tdistance)) Flag = true; break;
		case 3: if(task3(start, tdistance)) Flag = true; break;
		case 4: if(task4(start, tdistance)) Flag = true; break;
		default: cout << "Don't understand legs = " << legs << "!\n";
	}
	return(Flag);
}



double square(double x)
{
  return(x * x);
}



/** Accurate but complex recursive method of computing the distance between points
 on an ellipsoid.
*/
double Vincenty(double lat1, double lon1, double lat2, double lon2 /*, double &bearing*/)
{
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



/** Rather than search every TP, make a list of those near enough to be useful. */
bool edit_tps(char *StartFinish, double distance, int legs)
{
	int i;
	double l, lat1, lon1;
	double edistance;

	i = get_tpi(tps, n_tps, StartFinish);
	double lat0 = tps[i].latitude;
	double lon0 = tps[i].longitude;

/** If there are 'en_tps' turning points within (tdistance * MAX_DIST_FACT / 2) kilometres perhaps there are 150 within
	(tdistance * MAX_DIST_FACT / 2) * sqrt(150.0 / en_tps)?
150 / 
*/
	en_tps = 0;
	switch(legs) {
		case 4:
		/// For four legs, apply a limit to the distance so as to favour "butterfly" tasks...
		for(i = 0; i < n_tps; i++ ) {
			l = Vincenty(toRad(lat0), toRad(lon0), toRad(tps[i].latitude), toRad(tps[i].longitude));
			if(l <= (tdistance * MAX_DIST_FACT * 0.5)) en_tps++;
		}
#if COMMENTS
		cout << "There are " << en_tps << " TPs within " << (tdistance * MAX_DIST_FACT * 0.5) << " kms\n";
#endif

		edistance = (tdistance * MAX_DIST_FACT * 0.5) * sqrt(100.0 / en_tps);
#if COMMENTS
		cout << "Use TPs within " << edistance << " kms.\n";
#endif

		en_tps = 0;
		for(i=0; i<n_tps; i++) {
			l = Vincenty(toRad(lat0), toRad(lon0), toRad(tps[i].latitude), toRad(tps[i].longitude));
			if((l < edistance) && (l > edistance / 8.0) || (l < 0.01)) {
				etps[en_tps] = tps[i];
				en_tps++;
			}
		}
#if COMMENTS
		cout << "Using nearest " << en_tps << " TPs\n";
#endif
		break;
		case 3:
		case 2:
		/// For out-and-returns and triangles, examine only TPs tdistance * MAX_DIST_FACT / 2.
		for(i=0; i<n_tps; i++) {
			lat1 = tps[i].latitude;
			lon1 = tps[i].longitude;
			l = Vincenty(toRad(lat0), toRad(lon0), toRad(lat1), toRad(lon1));
			if(l < distance * MAX_DIST_FACT * 0.5) {
				etps[en_tps] = tps[i];
				en_tps++;
			}
		}
	}
	return(en_tps > 0);
}



/** search for reasonable Out-and-Return tasks. */
bool task2(char *StartFinish, double distance)
{
	int i, j, k, n, t = 0;
	double l;
	double lat, lon;
	position p1;
	position p2;
	position p3;
	tp tp1;
	tp tp2;
	tp tp3;
	task tx;
	task *pt=tasks;

	planlog("task2");
	if(!StartFinish[0]) {
		cout << "No start/end point.\n";
		return(false);
	}

	tasks_found = 0;

/// Find position of start/finish tp.
	i = get_tpi(tps, n_tps, StartFinish);

	if((i == -1)) {
		cout << "Start/End error\n";
		strcpy(StartFinish, tps[n_tps / 2].code);
		i = get_tpi(tps, n_tps, StartFinish);
	}

	lat = tps[i].latitude;
	lon = tps[i].longitude;
	p1.lat = lat;
	p1.lon = lon;

	tp1 = tps[i];
	ptask = tasks;

	/// Search all combinations of 2 tps for those of sensible length.
	for(j=1, t=0; j<n_tps; j++) {
		if(i == j) continue;

#if SHOW_TASK
		cout << "j=" << j << ", tasks=" << tasks_found << "\n";
#endif
		p2.lat = tps[j].latitude;
		p2.lon = tps[j].longitude;

		l = (p1 - p2) * 2.0;
		if(isnan(l)) continue;
/// If the task length is too short, drop it...
		if(l < distance) continue;
/// likewise if it is excessively long.
		if(l > (distance * MAX_DIST_FACT)) continue;

#if COMMENTS
		cout << "tp1 = " << tps[i].code << ", tp2 = " << tps[j].code << ", tp3 = " << tps[k].code << ": l = " << l << "\n";
#endif
		tasks[tasks_found].tp1 = &tps[i];
		tasks[tasks_found].tp2 = &tps[j];
		tasks[tasks_found].tp3 = &tps[i];
		tasks[tasks_found].distance = l;
		tasks[tasks_found++].legs = 2;

/// When we have 200 candidates, sort them (and keep the best 100).
		if(tasks_found >= 200) {
			qsort(tasks, 200, sizeof(task), compare_tasks);
			tasks_found = 100; 
		}
	}

/// All candidates processed; sort them.
	if(tasks_found) qsort(tasks, tasks_found-1, sizeof(task), compare_tasks);
	else return(false);

#if SHOW_TASK
	show_task(0);
	show_task(1);
	show_task(2);
#endif
	return(true);
}
 


/** Search for suitable triangular tasks. */
bool task3(char *StartFinish, double distance)
{
	int i, j, k, n, t = 0;
	double l;
	double lat, lon;
	position p1;
	position p2;
	position p3;
	tp tp1;
	tp tp2;
	tp tp3;
	task tx;
	task *pt=tasks;
	planlog("task3");

	if(!StartFinish[0]) {
		cout << "No start/end point.\n";
		return(false);
	}

	edit_tps(StartFinish, distance, 3);

	tasks_found = 0;

/// Find position of start/finish tp.
	i = get_tpi(etps, en_tps, StartFinish);

	if((i == -1)) {
		cout << "Start/End error in 'task3'\n";
		strcpy(StartFinish, etps[n_tps / 2].code);
		i = get_tpi(etps, en_tps, StartFinish);
	}

	lat = etps[i].latitude;
	lon = etps[i].longitude;
	p1.lat = lat;
	p1.lon = lon;

	tp1 = etps[i];

	l = p1 - p2;

	ptask = tasks;
	/// Search all combinations of 2 tps for those of sensible length.
	for(j=1, t=0; j<en_tps; j++) {
		for(k=j+1; k<en_tps; k++) {
			if(i == j) continue;
			if(i == k) continue;
			if(j == k) continue;

#if SHOW_TASK
			cout << "j=" << j << ", k=" << k << ", tasks=" << tasks_found << "\n";
#endif
			p2.lat = etps[j].latitude;
			p2.lon = etps[j].longitude;

			p3.lat = etps[k].latitude;
			p3.lon = etps[k].longitude;
			l = ((p1 - p2) + (p2 - p3) + (p3 - p1));
			if(isnan(l)) continue;
/// If the task length is too short, drop it...
			if(l < distance) continue;
/// likewise if it is excessively long.
			if(l > (distance * MAX_DIST_FACT)) continue;

			if(fai_flag) {
				if((p1 - p2) < l * 0.28) continue;
				if((p2 - p3) < l * 0.28) continue;
				if((p3 - p1) < l * 0.28) continue;
			}
			if(t25_45_flag) {
				if((p1 - p2) < l * 0.25) continue;
				if((p2 - p3) < l * 0.25) continue;
				if((p3 - p1) < l * 0.25) continue;
				if((p1 - p2) > l * 0.45) continue;
				if((p2 - p3) > l * 0.45) continue;
				if((p3 - p1) > l * 0.45) continue;
			}

#if COMMENTS
			cout << "tp1 = " << etps[i].code << ", tp2 = " << etps[j].code << ", tp3 = " << etps[k].code << ": l = " << l << "\n";
#endif
			tasks[tasks_found].tp1 = &etps[i];
			tasks[tasks_found].tp2 = &etps[j];
			tasks[tasks_found].tp3 = &etps[k];
			tasks[tasks_found].distance = l;
			tasks[tasks_found++].legs = 3;

/// When we have 200 candidates, sort them (and keep the best 100).
			if(tasks_found >= 200) {
				qsort(tasks, 200, sizeof(task), compare_tasks);
				tasks_found = 100; 
			}
		}
	}

/// All candidates processed; sort them.
	if(tasks_found) qsort(tasks, tasks_found-1, sizeof(task), compare_tasks);
	else return(false);

#if SHOW_TASK
	show_task(0);
	show_task(1);
	show_task(2);
#endif
	return(true);
}
 


/** Using a subset of the TP list, search for 4-leg tasks. */
bool task4(char *StartFinish, double distance)
{
	int i, j, k, m, n, t = 0;
	int mid;
	double l;
	double lat, lon;
	position p1;
	position p2;
	position p3;
	position p4;
	tp tp1;
	tp tp2;
	tp tp3;
	tp tp4;
	task tx;
	task *pt=tasks;
	planlog("task4");

	if(!StartFinish[0]) {
		cout << "No start/end point.\n";
		return(false);
	}

	edit_tps(StartFinish, distance, 4);

	tasks_found = 0;

/// Find position of start/finish tp.
	i = get_tpi(etps, en_tps, StartFinish);

	if((i == -1)) {
		cout << "Start/End <" << StartFinish << "> error\n";
		strcpy(StartFinish, etps[en_tps / 2].code);
		i = get_tpi(etps, en_tps, StartFinish);
	}

	lat = etps[i].latitude;
	lon = etps[i].longitude;
	p1.lat = lat;
	p1.lon = lon;

	tp1 = etps[i];

	mid = en_tps / 2;

	ptask = tasks;
	/// Search all combinations of 2 tps for those of sensible length.
	for(j=0, t=0; j < mid; j++) {
		for(k=0; k < en_tps; k++) {
			for(m = mid + 1; m < en_tps; m++) {
				if(i == j) continue;
				if(i == k) continue;
				if(j == k) continue;
				if(i == m) continue;
				if(j == m) continue;
				if(k == m) continue;

#if SHOW_TASK
				cout << "j=" << j << ", k=" << k << ", tasks=" << tasks_found << "\n";
#endif
				p2.lat = etps[j].latitude;
				p2.lon = etps[j].longitude;

				p3.lat = etps[k].latitude;
				p3.lon = etps[k].longitude;

				p4.lat = etps[m].latitude;
				p4.lon = etps[m].longitude;

				l = ((p1 - p2) + (p2 - p3) + (p3 - p4) + (p4 - p1));
				if(isnan(l)) continue;
	/// If the task length is too short, drop it...
				if(l < distance) continue;
	/// likewise if it is excessively long.
				if(l > (distance * MAX_DIST_FACT)) continue;

				if(REJECT_THIN_TRIANGLES) {
					if((p1 - p2) < l * MIN_LEG) continue;
					if((p2 - p3) < l * MIN_LEG) continue;
					if((p3 - p4) < l * MIN_LEG) continue;
					if((p4 - p1) < l * MIN_LEG) continue;
				}

#if COMMENTS
				cout << "tp1 = " << tps[i].code << ", tp2 = " << tps[j].code << ", tp3 = " << tps[k].code << ", tp4 = " << tps[m].code << ": l = " << l << "\n";
#endif
				tasks[tasks_found].tp1 = &etps[i];
				tasks[tasks_found].tp2 = &etps[j];
				tasks[tasks_found].tp3 = &etps[k];
				tasks[tasks_found].tp4 = &etps[m];
				tasks[tasks_found].distance = l;
				tasks[tasks_found++].legs = 4;

	/// When we have 200 candidates, sort them (and keep the best 100).
				if(tasks_found >= 200) {
					qsort(tasks, 200, sizeof(task), compare_tasks);
					tasks_found = 100; 
				}
			}
		}
	}

/// All candidates processed; sort them.
	if(tasks_found) qsort(tasks, tasks_found-1, sizeof(task), compare_tasks);
	else return(false);

//	configure_event(NULL, NULL);

#if SHOW_TASK
	show_task(0);
	show_task(1);
	show_task(2);
#endif
	return(true);
}
 


int main(int argc, char **argv) {
	char file_name[128];
	char disttext[32], legstext[32], flagstext[32], ntext[32];
	int i;
#if TEST1
	cout << "SUT,CHP,SSO;SUT,BLN,TOR;SUT,ACC,MAT;SUT,ACC,ULL;SUT,BAK,SKS;SUT,PRN,SEW;SUT,ACC,PEI;SUT,BLN,CHP;SUT,DER,RTH;SUT,HEX,WDM;SUT,CHP,CP1;SUT,HSG,THG;SUT,APP,LYD;SUT,ACC,SHI;SUT,CFB,MIT;SUT,BLI,THG;SUT,ACC,MLM;SUT,CHP,DER;SUT,ACC,CCR;SUT,CHP,SFE;SUT,LYD,SSO;SUT,BLN,LOW;SUT,KLO,SEW;SUT,BUX,KIR;SUT,KLO,ROT;SUT,MUT,RBY;SUT,BUH,PRN;SUT,DER,TOR;SUT,MSE,THG;SUT,ACC,KIR;SUT,BGG,BUX;SUT,SHE,SPU;SUT,HOW,MIT;SUT,BEL,SED;SUT,CHO,ROT;SUT,LOU,SHE;SUT,KLO,PEV;SUT,MOP,PIE;SUT,SFE,SPU;SUT,ACC,ALO;SUT,ALO,CHP;SUT,MIT,SHE;SUT,MOP,TEB;SUT,HSG,PRN;SUT,SWM,THG;SUT,BUH,DFU;SUT,SEW,WRA;SUT,CHO,SSO;SUT,CPH,PRN;SUT,LYD,SFE;\n";
	exit(0);
#endif

#if SHOW
	cout << "list_tasks: " << argc << " arguments\n";
#endif
	planlog("xcplan");

	switch(argc) {
		case 1: planlog("1 arg");
		strcpy(region, "uk");
		strcpy(start, "SUT");
		tdistance = 100.0;
		legs = 3;
		flags = 1;
		n = 0;
		break;
		case 7:
		planlog("7 args");
		strncpy(region, argv[1], 32);
		strncpy(start, argv[2], 32);
		strncpy(disttext, argv[3], 32);
		strncpy(legstext, argv[4], 32);
		strncpy(flagstext, argv[5], 32);
		strncpy(ntext, argv[6], 32);

		sscanf(disttext, "%lf", &tdistance);
		sscanf(legstext, "%d", &legs);
		sscanf(flagstext, "%d", &flags);
		sscanf(ntext, "%d", &n);
		break;
		default: planlog("? args"); 
		cout << "Invalid number of arguments!\n";
		return(0);
	}

	fai_flag = flags & 1;
	t25_45_flag = flags & 2;

	planlog("ready to plan");
	plan( );
// OK to here!
	planlog("planned");
	for(i=0; i<((tasks_found < MAX_SOLUTIONS)? tasks_found: MAX_SOLUTIONS); i++) {
		switch(legs) {
			case 2: cout << tasks[i].tp1->code << "," << tasks[i].tp2->code << "|"; break;
			case 3: cout << tasks[i].tp1->code << "," << tasks[i].tp2->code << "," << tasks[i].tp3->code << "|"; break;
			case 4: cout << tasks[i].tp1->code << "," << tasks[i].tp2->code << "," << tasks[i].tp3->code << "," << tasks[i].tp4->code << "|"; break;
		}
	}
#if COMMENTS
	puts("Done!\n");
#endif
exit(0); 
}




