/* MPU9250 Basic Example Code
 by: Kris Winer
 date: April 1, 2014
 license: Beerware - Use this code however you'd like. If you
 find it useful you can buy me a beer some time.
 Modified by Brent Wilkins July 19, 2016

 Demonstrate basic MPU-9250 functionality including parameterizing the register
 addresses, initializing the sensor, getting properly scaled accelerometer,
 gyroscope, and magnetometer data out. Added display functions to allow display
 to on breadboard monitor. Addition of 9 DoF sensor fusion using open source
 Madgwick and Mahony filter algorithms. Sketch runs on the 3.3 V 8 MHz Pro Mini
 and the Teensy 3.1.
 */

#include <M5Stack.h>
#include "utility/MPU9250.h"
#include "utility/quaternionFilters.h"

#define processing_out false
#define AHRS true         // Set to false for basic data read
#define SerialDebug true  // Set to true to get Serial output for debugging
#define LCD

MPU9250 IMU;
// Kalman kalmanX, kalmanY, kalmanZ; // Create the Kalman instances

void setup()
{
  M5.begin();
  Wire.begin();
  IMU.initMPU9250();
  IMU.initAK8963(IMU.magCalibration);
}

void loop()
{
  // If intPin goes high, all data registers have new data
  // On interrupt, check if data ready interrupt
  if (IMU.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)
  {  
    IMU.readAccelData(IMU.accelCount);  // Read the x/y/z adc values
    IMU.getAres();

    // Now we'll calculate the accleration value into actual g's
    // This depends on scale being set
    IMU.ax = (float)IMU.accelCount[0]*IMU.aRes; // - accelBias[0];
    IMU.ay = (float)IMU.accelCount[1]*IMU.aRes; // - accelBias[1];
    IMU.az = (float)IMU.accelCount[2]*IMU.aRes; // - accelBias[2];

    IMU.readGyroData(IMU.gyroCount);  // Read the x/y/z adc values
    IMU.getGres();

    // Calculate the gyro value into actual degrees per second
    // This depends on scale being set
    IMU.gx = (float)IMU.gyroCount[0]*IMU.gRes;
    IMU.gy = (float)IMU.gyroCount[1]*IMU.gRes;
    IMU.gz = (float)IMU.gyroCount[2]*IMU.gRes;

    IMU.readMagData(IMU.magCount);  // Read the x/y/z adc values
    IMU.getMres();
    // User environmental x-axis correction in milliGauss, should be
    // automatically calculated
    IMU.magbias[0] = +470.;
    // User environmental x-axis correction in milliGauss TODO axis??
    IMU.magbias[1] = +120.;
    // User environmental x-axis correction in milliGauss
    IMU.magbias[2] = +125.;

    // Calculate the magnetometer values in milliGauss
    // Include factory calibration per data sheet and user environmental
    // corrections
    // Get actual magnetometer value, this depends on scale being set
    IMU.mx = (float)IMU.magCount[0]*IMU.mRes*IMU.magCalibration[0] -
               IMU.magbias[0];
    IMU.my = (float)IMU.magCount[1]*IMU.mRes*IMU.magCalibration[1] -
               IMU.magbias[1];
    IMU.mz = (float)IMU.magCount[2]*IMU.mRes*IMU.magCalibration[2] -
               IMU.magbias[2];
  } // if (readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)

  // Must be called before updating quaternions!
  IMU.updateTime();

  // Sensors x (y)-axis of the accelerometer is aligned with the y (x)-axis of
  // the magnetometer; the magnetometer z-axis (+ down) is opposite to z-axis
  // (+ up) of accelerometer and gyro! We have to make some allowance for this
  // orientationmismatch in feeding the output to the quaternion filter. For the
  // MPU-9250, we have chosen a magnetic rotation that keeps the sensor forward
  // along the x-axis just like in the LSM9DS0 sensor. This rotation can be
  // modified to allow any convenient orientation convention. This is ok by
  // aircraft orientation standards! Pass gyro rate as rad/s
//  MadgwickQuaternionUpdate(ax, ay, az, gx*PI/180.0f, gy*PI/180.0f, gz*PI/180.0f,  my,  mx, mz);
  MahonyQuaternionUpdate(IMU.ax, IMU.ay, IMU.az, IMU.gx*DEG_TO_RAD,
                         IMU.gy*DEG_TO_RAD, IMU.gz*DEG_TO_RAD, IMU.my,
                         IMU.mx, IMU.mz, IMU.deltat);

  if (!AHRS)
  {
    IMU.delt_t = millis() - IMU.count;
    if (IMU.delt_t > 500)
    {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextColor(GREEN ,BLACK);
      M5.Lcd.setCursor(0, 0); M5.Lcd.print("MPU9250/AK8963");
      M5.Lcd.setCursor(0, 32); M5.Lcd.print(" x   y   z  ");

      M5.Lcd.setCursor(0,  48); M5.Lcd.print((int)(1000*IMU.ax));
      M5.Lcd.setCursor(32, 48); M5.Lcd.print((int)(1000*IMU.ay));
      M5.Lcd.setCursor(64, 48); M5.Lcd.print((int)(1000*IMU.az));
      M5.Lcd.setCursor(96, 48); M5.Lcd.print("mg");

      M5.Lcd.setCursor(0,  64); M5.Lcd.print((int)(IMU.gx));
      M5.Lcd.setCursor(32, 64); M5.Lcd.print((int)(IMU.gy));
      M5.Lcd.setCursor(64, 64); M5.Lcd.print((int)(IMU.gz));
      M5.Lcd.setCursor(96, 64); M5.Lcd.print("o/s");

      M5.Lcd.setCursor(0,  96); M5.Lcd.print((int)(IMU.mx));
      M5.Lcd.setCursor(32, 96); M5.Lcd.print((int)(IMU.my));
      M5.Lcd.setCursor(64, 96); M5.Lcd.print((int)(IMU.mz));
      M5.Lcd.setCursor(96, 96); M5.Lcd.print("mG");

      M5.Lcd.setCursor(0,  128); M5.Lcd.print("Gyro T ");
      M5.Lcd.setCursor(50,  128); M5.Lcd.print(IMU.temperature, 1);
      M5.Lcd.print(" C");

      IMU.count = millis();
    }
  }
  else
  {
    // Serial print and/or display at 0.5 s rate independent of data rates
    IMU.delt_t = millis() - IMU.count;

    // update LCD once per half-second independent of read rate
    // if (IMU.delt_t > 500)
    if (IMU.delt_t > 100)
    {
      if(SerialDebug)
      {
        Serial.print("ax = "); Serial.print((int)1000*IMU.ax);
        Serial.print(" ay = "); Serial.print((int)1000*IMU.ay);
        Serial.print(" az = "); Serial.print((int)1000*IMU.az);
        Serial.println(" mg");

        Serial.print("gx = "); Serial.print( IMU.gx, 2);
        Serial.print(" gy = "); Serial.print( IMU.gy, 2);
        Serial.print(" gz = "); Serial.print( IMU.gz, 2);
        Serial.println(" deg/s");

        Serial.print("mx = "); Serial.print( (int)IMU.mx );
        Serial.print(" my = "); Serial.print( (int)IMU.my );
        Serial.print(" mz = "); Serial.print( (int)IMU.mz );
        Serial.println(" mG");

        Serial.print("q0 = "); Serial.print(*getQ());
        Serial.print(" qx = "); Serial.print(*(getQ() + 1));
        Serial.print(" qy = "); Serial.print(*(getQ() + 2));
        Serial.print(" qz = "); Serial.println(*(getQ() + 3));
      }

// Define output variables from updated quaternion---these are Tait-Bryan
// angles, commonly used in aircraft orientation. In this coordinate system,
// the positive z-axis is down toward Earth. Yaw is the angle between Sensor
// x-axis and Earth magnetic North (or true North if corrected for local
// declination, looking down on the sensor positive yaw is counterclockwise.
// Pitch is angle between sensor x-axis and Earth ground plane, toward the
// Earth is positive, up toward the sky is negative. Roll is angle between
// sensor y-axis and Earth ground plane, y-axis up is positive roll. These
// arise from the definition of the homogeneous rotation matrix constructed
// from quaternions. Tait-Bryan angles as well as Euler angles are
// non-commutative; that is, the get the correct orientation the rotations
// must be applied in the correct order which for this configuration is yaw,
// pitch, and then roll.
// For more see
// http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
// which has additional links.
      IMU.yaw   = atan2(2.0f * (*(getQ()+1) * *(getQ()+2) + *getQ() *
                    *(getQ()+3)), *getQ() * *getQ() + *(getQ()+1) * *(getQ()+1)
                    - *(getQ()+2) * *(getQ()+2) - *(getQ()+3) * *(getQ()+3));
      IMU.pitch = -asin(2.0f * (*(getQ()+1) * *(getQ()+3) - *getQ() *
                    *(getQ()+2)));
      IMU.roll  = atan2(2.0f * (*getQ() * *(getQ()+1) + *(getQ()+2) *
                    *(getQ()+3)), *getQ() * *getQ() - *(getQ()+1) * *(getQ()+1)
                    - *(getQ()+2) * *(getQ()+2) + *(getQ()+3) * *(getQ()+3));
      IMU.pitch *= RAD_TO_DEG;
      IMU.yaw   *= RAD_TO_DEG;
      // Declination of SparkFun Electronics (40°05'26.6"N 105°11'05.9"W) is
      // 	8° 30' E  ± 0° 21' (or 8.5°) on 2016-07-19
      // - http://www.ngdc.noaa.gov/geomag-web/#declination
      IMU.yaw   -= 8.5;
      IMU.roll  *= RAD_TO_DEG;

      if(SerialDebug)
      {
        Serial.print("Yaw, Pitch, Roll: ");
        Serial.print(IMU.yaw, 2);
        Serial.print(", ");
        Serial.print(IMU.pitch, 2);
        Serial.print(", ");
        Serial.println(IMU.roll, 2);

        Serial.print("rate = ");
        Serial.print((float)IMU.sumCount/IMU.sum, 2);
        Serial.println(" Hz");
        Serial.println("");
      }

#ifdef LCD
      // M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextFont(2);

      M5.Lcd.setCursor(0, 0); M5.Lcd.print("     x       y       z ");
      M5.Lcd.setCursor(0,  24);
      M5.Lcd.printf("% 6d  % 6d  % 6d     mg   \r\n",  (int)(1000*IMU.ax), (int)(1000*IMU.ay), (int)(1000*IMU.az));
      M5.Lcd.setCursor(0,  44);
      M5.Lcd.printf("% 6d  % 6d  % 6d      o/s  \r\n", (int)(IMU.gx), (int)(IMU.gy), (int)(IMU.gz));
      M5.Lcd.setCursor(0,  64);
      M5.Lcd.printf("% 6d  % 6d  % 6d     mG    \r\n",  (int)(IMU.mx), (int)(IMU.my), (int)(IMU.mz));
  
      M5.Lcd.setCursor(0,  100);
      M5.Lcd.printf("  yaw: % 5.2f    pitch: % 5.2f    roll: % 5.2f   \r\n",(IMU.yaw), (IMU.pitch), (IMU.roll));

    // With these settings the filter is updating at a ~145 Hz rate using the
    // Madgwick scheme and >200 Hz using the Mahony scheme even though the
    // display refreshes at only 2 Hz. The filter update rate is determined
    // mostly by the mathematical steps in the respective algorithms, the
    // processor speed (8 MHz for the 3.3V Pro Mini), and the magnetometer ODR:
    // an ODR of 10 Hz for the magnetometer produce the above rates, maximum
    // magnetometer ODR of 100 Hz produces filter update rates of 36 - 145 and
    // ~38 Hz for the Madgwick and Mahony schemes, respectively. This is
    // presumably because the magnetometer read takes longer than the gyro or
    // accelerometer reads. This filter update rate should be fast enough to
    // maintain accurate platform orientation for stabilization control of a
    // fast-moving robot or quadcopter. Compare to the update rate of 200 Hz
    // produced by the on-board Digital Motion Processor of Invensense's MPU6050
    // 6 DoF and MPU9150 9DoF sensors. The 3.3 V 8 MHz Pro Mini is doing pretty
    // well!

      // M5.Lcd.setCursor(0, 60);
      // M5.Lcd.printf("yaw:%6.2f   pitch:%6.2f   roll:%6.2f  ypr \r\n",(IMU.yaw), (IMU.pitch), (IMU.roll));
      M5.Lcd.setCursor(12, 144); 
      M5.Lcd.print("rt: ");
      M5.Lcd.print((float) IMU.sumCount / IMU.sum, 2);
      M5.Lcd.print(" Hz");
#endif // LCD

      IMU.count = millis();
      IMU.sumCount = 0;
      IMU.sum = 0;

#if(processing_out)

      Serial.print(((IMU.yaw)));    Serial.print(";");
      Serial.print(((IMU.pitch))); Serial.print(";");
      Serial.print(((IMU.roll)));   Serial.print(";");
      Serial.print(26.5);    Serial.print(";");
      Serial.print(0.01);    Serial.print(";");
      Serial.print(0.02);    Serial.println();
#endif
    } // if (IMU.delt_t > 500)
  } // if (AHRS)
}
