#!/usr/bin/php -q
<?
function cmps12UnisgnedToSigned($in) {
	if ( $in > 32767 ) 
		$in -= 65536;

	return $in;
}

function rdLogger_CMPS_process_file($infile,$outfile) {
	$fp['in']=fopen($infile,'r');

	$fp['out']=fopen($outfile,'w');
//	$fp['out']=fopen('php://stdout','w');



/*
(
    [0] => 2018-11-01 19:49:55
    [1] => 15226
    [2] => 1168
    [3] => 0
    [4] => 326
    [5] => 0
    [6] => 34
    [7] => R1024
    [8] => 141
    [9] => 506
    [10] => 103
    [11] => -60
    [12] => 64837
    [13] => 65412
    [14] => 65296
    [15] => 709
    [16] => 401
    [17] => 65412
    [18] => 1232
    [19] => 63984
    [20] => 243
    [21] => 1091
    [22] => 9
    [23] => -103
    [24] => 247
)

*/

	$csvHeader=1;
	$lineN=0;

	while ( $csv=fgetcsv($fp['in'],1024) ) {
		if ( count($csv) < 25 ) {
			printf("# not enough columns on line %d. Dumping CSV parsed line:\n",$lineN);
			print_r($csv);
		}


		$r['PACKET DATE']=$csv[0];
		$r['PULSE PERIOD']=$csv[1];
		$r['PULSE MIN PERIOD']=$csv[2];
		$r['PULSE COUNT']=$csv[3];
		$r['INPUT ADC']=$csv[4];
		$r['ANALOG0 ADC']=$csv[5];
		$r['UPTIME']=$csv[6];
		$r['SERIAL NUMBER']=$csv[7];

		/* CMPS12 */
		$r['CMPS12 WIND DIRECTION']=$csv[8];
		$r['CMPS12 BEARING']=$csv[9];
		$r['CMPS12 PITCH']=$csv[10];
		$r['CMPS12 ROLL']=$csv[11];
		$r['CMPS12 MAG X']=$csv[12];
		$r['CMPS12 MAG Y']=$csv[13];
		$r['CMPS12 MAG Z']=$csv[14];
		$r['CMPS12 ACCEL X']=$csv[15];
		$r['CMPS12 ACCEL Y']=$csv[16];
		$r['CMPS12 ACCEL Z']=$csv[17];
		$r['CMPS12 GYRO X']=$csv[18];
		$r['CMPS12 GYRO Y']=$csv[19];
		$r['CMPS12 GYRO Z']=$csv[20];
		$r['CMPS12 BN055 BEARING']=$csv[21];
		$r['CMPS12 TEMPERATURE']=$csv[22];
		$r['CMPS12 PITCH ANGLE']=$csv[23];
		$r['CMPS12 CALIBRATION STATE']=$csv[24];

		/* do some calculations */

		/* BN055 sensor XYZ is signed */
		$r['(CALC) CMPS12 MAG X']   = cmps12UnisgnedToSigned($r['CMPS12 MAG X']);
		$r['(CALC) CMPS12 MAG Y']   = cmps12UnisgnedToSigned($r['CMPS12 MAG Y']);
		$r['(CALC) CMPS12 MAG Z']   = cmps12UnisgnedToSigned($r['CMPS12 MAG Z']);
		$r['(CALC) CMPS12 ACCEL X'] = cmps12UnisgnedToSigned($r['CMPS12 ACCEL X']);
		$r['(CALC) CMPS12 ACCEL Y'] = cmps12UnisgnedToSigned($r['CMPS12 ACCEL Y']);
		$r['(CALC) CMPS12 ACCEL Z'] = cmps12UnisgnedToSigned($r['CMPS12 ACCEL Z']);
		$r['(CALC) CMPS12 GYRO X']  = cmps12UnisgnedToSigned($r['CMPS12 GYRO X']);
		$r['(CALC) CMPS12 GYRO Y']  = cmps12UnisgnedToSigned($r['CMPS12 GYRO Y']);
		$r['(CALC) CMPS12 GYRO Z']  = cmps12UnisgnedToSigned($r['CMPS12 GYRO Z']);

		/* CMPS12 is bearing degrees * 10 */
		$r['(CALC) CMPS12 BEARING DEGREES']=$r['CMPS12 BEARING']/10.0;

		/* BN055 is bearing degrees * 16 */
		$r['(CALC) CMPS12 BN055 BEARING DEGREES']=fmod( $r['CMPS12 BN055 BEARING']/16.0, 360.0);

	
		/* see how they compare */
		$r['(CALC) CMPS12 - BN055 DIFFERENCE']=$r['(CALC) CMPS12 BEARING DEGREES']-$r['(CALC) CMPS12 BN055 BEARING DEGREES'];

		/* calibration state */
		$r['(CALC) CMPS12 CALIBRATION SYSTEM']= ($r['CMPS12 CALIBRATION STATE'] >> 6) & 0x03;
		$r['(CALC) CMPS12 CALIBRATION GYRO']=   ($r['CMPS12 CALIBRATION STATE'] >> 4) & 0x03;
		$r['(CALC) CMPS12 CALIBRATION ACCEL']=  ($r['CMPS12 CALIBRATION STATE'] >> 2) & 0x03;
		$r['(CALC) CMPS12 CALIBRATION MAG']=    ($r['CMPS12 CALIBRATION STATE'] >> 0) & 0x03;


		/* output to CSV */
		/* print header if this is our first time through */
		if ( $csvHeader ) {
			fputcsv($fp['out'],array_keys($r));
			$csvHeader=0;
		}
	
		/* print line */
		fputcsv($fp['out'],$r);

		$lineN++;
	}
}



if ( $_SERVER['argc'] < 2  ) {
	printf("# first argument must be an existing rdLoggerUniversal log file. Aborting.\n");
	return;
}

for ( $i=1 ; $i<$_SERVER['argc'] ; $i++ ) {
	$infile=$_SERVER['argv'][$i];

	if ( ! file_exists($infile) ) {
		printf("# '%s' does not exist. Skipping.\n",$infile);
	}

	if ( FALSE !== strpos($infile,'processed_') ) {
		printf("# skipping '%s' because it already appears to be processed.\n",$infile);
		continue;
	}

	$outfile=dirname($infile) . '/processed_' . basename($infile);

	printf("# processing '%s' to '%s' ... ",$infile,$outfile);


	rdLogger_CMPS_process_file($_SERVER['argv'][$i],$outfile);

	printf("done.\n");


}

?>
