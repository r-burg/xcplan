
<?php
# Invoke by entering 'localhost/gui4.php' in a browser.
# Version 0.51 2019-03-29 RB	
# Version 0.52 2019-03-31 RB	Add "next" and "prev" buttons.
# Version 0.53 2019-04-01 RB	Change to "POST". Use a hidden input to remember the task index number.
# Version 0.54 2019-05-25 RB	Add "FAI" and "25/45%" checkboxes.
# Version 0.55 2019-05-26 RB	Passing flags to XC-plan.
# Version 0.56 2019-06-03 RB	Use a hidden field to pass the task list.

$page=$_SERVER['REQUEST_URI'];

function choice ($reg, $sel) {
	if($sel) echo "<option value=\"$reg\" selected> $reg </option>";
	else echo "<option> $reg </option>";
}


function populate($file, $start) {
#	print("[File: $file]");
	echo "<td> <select name=\"startend\">";
#	$filename = $region;
	$file = "$file.tps";
	
	$fhandle = fopen($file, "r");

	if($fhandle != false) {
#		echo "File opened. <br>";
		# Read TP file line by line.
		while(($tp = fgets($fhandle, 1024)) !== false) {
			# Extract the tp code.
			$sep = strpos($tp, "\",");
#				print "$sep ";
			if($tp[0] == "\"") {
				# BGA format process (all items in quotes).
				$sep = strpos($tp, ",");
				$tp = substr($tp, $sep+1);
				$sep = strpos($tp, "\",");
				$tp = substr($tp, 0, $sep+1);
				$sep = strpos($tp, "\"");
				$tp = substr($tp, 1, -1);
			} else {
				# US format (no quotes).
				$sep = strpos($tp, ",");
				$tp = substr($tp, $sep+1);
				$sep = strpos($tp, ",");
				$tp = substr($tp, 0, $sep);
				$sep = strpos($tp, "\"");
			}
			# Add TP to Start/End box.
			echo "<$tp>";
			if($tp == $start) choice($tp, true);
			else if(strlen($tp) > 0) choice($tp, false);
		}
		fclose($fhandle);
		echo "</select> </td> </tr> <br>";
	} else print"Can't open file<br>";
}



function item0($str1, $sep) {
	$n=strpos($str1, $sep);
#	print "[n=$n]";
	if($n==FALSE) {
#		print " item = $str1; ";
		return($str1);
	}
	$item=substr($str1, 0, $n);
#	print " item = $item; ";
	return($item);
}


function remainder($str1, $sep) {
	$n=strpos($str1, $sep);
	if($n==FALSE) return("");
	$m=strlen($str1);
	$remains=substr($str1, $n+1, $m-$n-1);
	return($remains);
}



function parse($line) {
	$i=0;
	$tp = [];
	while(strlen($line)>0) {
		$tp[$i++]=item0($line, ",");
		$line=remainder($line, ",");
	}
	return($tp);
}


#### Main program begins. ####

	$version = "0.6";

# Display a title.
	echo "<html>";
		echo "<head>";
		echo "<META HTTP-EQUIV=\"submit\" CONTENT=\"2;URL=gui4.php\">";
		print "<title> xcplan Cross-country planner</title>" ;
#		print "Copyright (c) R. Burghall 2018-2019;  Licenced under GPL v3.0";
		echo "<style>";
		echo "table tr td { margin-top:1px; margin-bottom:1px; margin-right:1px; margin-left:1px; padding: 1px 1px 1px 1px; }";
		echo "body { padding: 1px 1px 1px 1px; }";
		echo "</style>";
		echo "</head>";
	echo "</html>";

#	$region = "usa";
	$comment = false;
	$tri_type = "all";
	$flags = '0';
	$result1 = [];
	$result = [];
	$dist = 300.0;
	$legs = 3;
	$region = "uk";
	$startend = "SUT";

	$indx=(isset($_POST['indx'])) ? $_POST['indx']: 0;

# If 'distance' is defined in the 'POST' parameters, record its value.
	if(isset($_POST['distance'])) {
		$dist = $_POST['distance'];
	}
#	echo "<br>";
	if(is_null($dist)) $dist=100.0;

	if(isset($_POST['region'])) {
		$region = $_POST['region'];
	}

	if(isset($_POST['startend'])) {
		$startend = $_POST['startend'];
	}

	if(isset($_POST['legs'])) {
		$legs = $_POST['legs'];
	}
	if(is_null($legs)) $legs=3;

	if(isset($_POST['tri_type'])) {
		$tri_type = $_POST['tri_type'];
	}
	if($comment) print " tri_type = $tri_type ";
	if($tri_type == 'none') $flags = '0';
	else if($tri_type == 'fai') $flags = '1';
	else if($tri_type == 't25') $flags = '2';

	if(isset($_POST['result1'])) {
		$result1 = $_POST['result1'];
	}

	if(isset($_POST['indx'])) {
		$indx = $_POST['indx'];
	}

	if(isset($_POST['next'])) {
		$indx = $indx+1;
	}

	if(isset($_POST['prev'])) {
		$indx = $indx-1;
		if($indx < 0) $indx = 0;
	}

	if(isset($_POST['first'])) {
		$indx=0;
	}
	if(is_null($indx)) $indx=0;

	if($comment) print_r($_POST);
	if($comment) print "Distance = $dist, Region = $region, Start/End = $startend, Legs = $legs\n";

	# Delete any old maps and their lock-files. Mod RB 2021-10-29 v0.6
	# echo "Delete old maps.";
	# $result1=shell_exec("find /var/www/xcplan -mmin +2 -iname xcplan*.png -exec /var/www/xcplan/del_image {} \;");
	# if($result1) echo " del_image failed. ";
	
	if($region == "") $region = "uk";
	
# Delete all old maps and their lock files
	shell_exec("/var/www/xcplan/Remove_old 2>&1");

# Use  "shell_exec ( string $command [, array &$output [, int &$return_var ]] ) : string" to call C++ engine.
#	print exec("cd /home/Roger& ls\n");
	if($indx == 0) {
		$result1=shell_exec("/var/www/xcplan/XC-plan $region $startend $dist $legs $flags 0 2>&1");
#		$result1=shell_exec("ls /var/www/xcplan 2>&1");
		if($comment) print "[/var/www/xcplan/XC-plan $region $startend $dist $legs $flags 0] ";
		if($comment) print "<$result1>\n";
	}
	
	$file = 'guilog.txt';
	if($handle = fopen($file, 'c')) {
	    fwrite($handle, $result1);
	    fclose($handle);
	    shell_exec('chmod 777');
	} else {
	    echo "Could not open \'planlog.txt\' for writing.";
	}
	
	$tasks = explode("|", $result1);

#	print "Called \'list_tasks\': $result1<br>";
#	$result=parse($result1);
#	if($comment) print ", task list=$result1 ";

# Plot what we have been passed 
	$image=shell_exec("/var/www/xcplan/XC-map $region,$tasks[$indx]\n2>&1");
	if($comment) print "/var/www/xcplan/XC-map $region,$tasks[$indx]:  $image\n";
	$image1 = $image;

# Create a selection of regions on the page.
	echo "<html>";
		print "Copyright (c) R. Burghall 2018-2019;  Licenced under GPL v3.0";
		echo "<table cellspacing=0 cellpadding=5>";
			echo "<form method=\"POST\" action=\"$_SERVER[PHP_SELF]\">";	
				echo "<input type=hidden name=result1 value=$result1>";
				echo "<tr> <td> <label for=\"region\"> Region:  </label> </td>";
				echo "<td> <select name=\"region\">";

# Identify each file in the current directory and see if it is a '.tps' file.
				$dir = ".";
				if($dh = opendir($dir)){
					while (($file = readdir($dh)) !== false) {
						$l = strlen($file);
						$p = strpos($file, ".tps");
						if($p >= 0) if($p == ($l - 4)) {
							echo "l = $l, p = $p<br>";
							$f = substr($file, 0, $p);
							print "Region: $f<br>";
							# The file is a turning point file. Add to Start/End box.
							if(($region == "") && ($f == "uk")) choice(substr($f, 0, $p), true);
							else if($region == $f) choice(substr($f, 0, $p), true);
							else choice(substr($f, 0, $p), false);
						}
					}
					closedir($dh);
				}
#				echo "</select> </td> </tr> <br>";
				echo "</select> </td>";

#				echo "<td> <label for=\"legs\"> Legs: </label> </td> </tr>";
# Set the default value of the legs select box to what it was before...
#			 	echo "<td> <select name=\"legs\" value=$legs id=\"legs\">";
#				if($legs == 2) choice("2", true); else choice("2", false);
#				if($legs == 3) choice("3", true); else choice("3", false);
#				if($legs == 4) choice("4", true); else choice("4", false);

				$filename = $region;
				$filename = "$filename.tps";
				if($comment) echo "Filename = $filename<br>";

# Shall we restrict results to FAI triangles?
#				echo "<td> FAI only:";
				echo "<td>";
				if($tri_type == "fai") echo "<input type=\"radio\" id=\"fai\" name=\"tri_type\" value=\"fai\" checked> FAI</td>";
				else echo "<input type=\"radio\" id=\"fai\" name=\"tri_type\" value=\"fai\"> FAI</td>";

# Shall we restrict results to 25/45% triangles?
#				echo "<td>.   25/45% only:";
				echo "<td>";
				if($tri_type == "t25") echo "<input type=\"radio\" id=\"t25\" name=\"tri_type\" value=\"t25\" checked> 25/45%</td>";
				else echo "<input type=\"radio\" id=\"t25\" name=\"tri_type\" value=\"t25\"> 25/45%</td>";

# or include all results?
#				echo "<td>.   all:";
				echo "<td>";
				if($tri_type == "all") echo "<input type=\"radio\" id=\"all\" name=\"tri_type\" value=\"all\" checked> All</td>";
				else echo "<input type=\"radio\" id=\"all\" name=\"tri_type\" value=\"all\"> All</td>";

				echo "<tr> <td> <label for=\"startend\"> Start/end:  </label> </td>";
				if($startend == NULL) if($region == "uk") $startend = "SUT";
				populate($region, $startend);

				echo "<tr> <td> <label for=\"distance\"> Distance: </label> </td>";
# Set the default value of the distance input box to what it was before...
			 	echo "<td> <input type=\"text\" name=\"distance\" value=\"$dist\" size=\"8\" id=\"distance\"> </td> </tr><br>";

				echo "<tr> <td> <label for=\"legs\"> Legs: </label> </td>";
# Set the default value of the legs select box to what it was before...
			 	echo "<td> <select name=\"legs\" value=$legs id=\"legs\">";
				if($legs == 2) choice("2", true); else choice("2", false);
				if($legs == 3) choice("3", true); else choice("3", false);
				if($legs == 4) choice("4", true); else choice("4", false);

				echo "<input type=\"hidden\" id=\"indx\" name=\"indx\" value=$indx>";

# Submit the form... it's the only way to read the inputs.
				echo "<tr> <td colspan=2 align=center> <input type=\"submit\" name=\"first\" value=\"First\">";	
				echo " <input type=\"submit\" name=\"next\" value=\"Next\">";			
				echo " <input type=\"submit\" name=\"prev\" value=\"Prev\"> </td> </tr>";	
			echo "</form>";
		echo "</table>";

		echo "<img src=\"$image\" align=left>";	#*****

		if($comment) {
			if($image[0] == " ") print("Space detected!");
			else print("No space.");

#			if(file_exists($image)) $result = unlink($image);
#			else print("Image seems not to exist!");
#			print("[$command]");
		}
	echo "</html>";
?>

