#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <time.h>

#include <ugpio/ugpio.h>

//Stats Over The Interval (raw data)
struct StatsOverInterval
{
	int numCars;
	float timeInterval;
	float cps;
};

//Stats Over The Simulation (stats)
struct StatsOverSimulation
{
	int totalCars;
	float totalTime;
	int maxCars;
	int minCars;
	float averageCars;
	float averageTime;
	int modeCars[10000];
	int numModes;
	float maxCPS;
	float minCPS;
	float avgCPS;
	float medianCPS;
	float popStdDevCPS;
	float smplStdDevCPS;
	float timeSaved;
};

//Constants
const unsigned int GRN_N = 18;          //gpio slot of green led for north
const unsigned int RED_N = 46;          //gpio slot of red led for north
const unsigned int GRN_W = 3;           //gpio slot of green led for west
const unsigned int RED_W = 1;           //gpio slot of red led for west
const unsigned int SENS_N_IN = 2;       //input (echo) gpio slot of sensor for north
const unsigned int SENS_N_OUT = 19;     //output (trigger) gpio slot of sensor for north
const unsigned int SENS_W_IN = 0;       //input (echo) gpio slot of sensor for west
const unsigned int SENS_W_OUT = 11;     // output (trigger) gpio slot of sensor for west

const float defaultTimeInterval = 30;   //default time interval to switch from green to red
const float defaultThreshold = 0.3;		//threshold for the sensor

//Logging parameters
int logDegree = 0;					//degree of logging; passed by user through argv, default is 0
char date[80];

//LED functions
bool light_on (const unsigned int port);
bool light_off (const unsigned int port);

//Time functions
int deltaTime (const int oldTime);
int timeUpdate();
int minutesToSeconds (int minutes);

//Sensor functions
float readSensor (const unsigned int gpioIn, const unsigned int gpioOut);
bool carPassed(const unsigned int gpioIn, const unsigned int gpioOut, const float threshold);

//Statistic Functions
float calcCarsPerSecond (struct StatsOverInterval intervalStats);
int calcTotalCars (struct StatsOverInterval statsInterval[], int sizeStats);
float calcTotalTime (struct StatsOverInterval statsInterval[], int sizeStats);
int calcMaxCars (struct StatsOverInterval statsInterval[], int sizeStats);
int calcMinCars (struct StatsOverInterval statsInterval[], int sizeStats);
float calcAvgCars (struct StatsOverInterval statsInterval[], int sizeStats);
float calcAvgTime (struct StatsOverInterval statsInterval[], int sizeStats);
int calcModeCars (struct StatsOverInterval statsInterval[], int sizeStats, int modes[]); 
bool sortInt(int dataset[], const int size);
bool selectionInt(int dataset[], const int size, int index);
bool sortFloat(float dataset[], const int size);
bool selectionFloat(float dataset[], const int size, int index);
float calcMaxCPS (struct StatsOverInterval statsInterval[], int sizeStats);
float calcMinCPS (struct StatsOverInterval statsInterval[], int sizeStats);
float calcAverageCPS (struct StatsOverInterval statsInterval[], int sizeStats);
float calcMedianCPS (struct StatsOverInterval statsInterval[], int sizeStats);
float calcPopStdDevCPS(struct StatsOverInterval statsInterval[], int sizeStats);
float calcSmplStdDevCPS(struct StatsOverInterval statsInterval[], int sizeStats);
float calcTimeSaved (struct StatsOverInterval statsInterval[], int sizeStats, const float defaultIntersectionTime);
struct StatsOverSimulation computeStatsOverSimulation(struct StatsOverInterval intervalStats[], int sizeStats);

//Filewriting functions
bool writeStatsToFile (char filename[], struct StatsOverInterval statsIntervalNorth[], int sizeNorth, 
			struct StatsOverInterval statsIntervalWest[], int sizeWest, 
			struct StatsOverSimulation statsSimNorth, struct StatsOverSimulation statsSimWest);
bool writeToLog(char filename[], int degreeLogging, int logMessageNumber, char tag[], float value);

int main(int argc, char **argv, char **envp)
{

	int simulationTime;			//time of simulation; passed by user through argv, default is 5 min
	int simulationTimer; 		//timer to keep track of when simulation should end
	
	char northTag[] = "North";	//Tag for logging purposes
	char westTag[] = "West";	//Tag for logging purposes
	
	//Get Date and Time of Simulation (used for name of output files)
  	struct timeval tv;
  	time_t curtime;
 	gettimeofday(&tv, NULL); 
    curtime = tv.tv_sec;
    strftime (date,80,"%F_%I:%M%p",localtime(&curtime));
	
	if (argc < 2)
	{
		logDegree = 0;
		simulationTime = 300;
	}
	else if (argc == 2)
	{
		logDegree = 0;
		simulationTime = minutesToSeconds(atoi(argv[1]));
	}
	else if (argc > 2)
	{
		logDegree = atoi(argv[2]);
		simulationTime = minutesToSeconds(atoi(argv[1]));
	}
	
	writeToLog(date, logDegree, 0, 0, 0);
	
	char tag1[] = "Simulation time";
	writeToLog(date, logDegree, 13, tag1, simulationTime);
	
	//request gpios and direction setup
	//NORTH
	gpio_request(SENS_N_OUT, NULL);
	gpio_direction_output(SENS_N_OUT, 0);
	gpio_request(SENS_N_IN, NULL);
	gpio_direction_input(SENS_N_IN);
	gpio_request(GRN_N, NULL);
	gpio_direction_output(GRN_N, 0);
	gpio_request(RED_N, NULL);
	gpio_direction_output(RED_N, 0); 
	
	//Log appropriate north pins
	char ntag1[] = "SENS_N_OUT";
	char ntag2[] = "SENS_N_IN";
	char ntag3[] = "GRN_N";
	char ntag4[] = "RED_N";
	writeToLog(date, logDegree, 1, ntag1, SENS_N_OUT);
	writeToLog(date, logDegree, 1, ntag2, SENS_N_IN);
	writeToLog(date, logDegree, 2, ntag4, RED_N);
	writeToLog(date, logDegree, 3, ntag3, GRN_N);
	
	
	//WEST
	gpio_request(SENS_W_OUT, NULL);
	gpio_direction_output(SENS_W_OUT, 0);
	gpio_request(SENS_W_IN, NULL);
	gpio_direction_input(SENS_W_IN);
	gpio_request(GRN_W, NULL);
	gpio_direction_output(GRN_W, 0);
	gpio_request(RED_W, NULL);
	gpio_direction_output(RED_W, 0);	
    
	//Log appropriate west pins
	char wtag1[] = "SENS_W_OUT";
	char wtag2[] = "SENS_W_IN";
	char wtag3[] = "GRN_W";
	char wtag4[] = "RED_W";
	writeToLog(date, logDegree, 1, ntag1, SENS_W_OUT);
	writeToLog(date, logDegree, 1, ntag2, SENS_W_IN);
	writeToLog(date, logDegree, 2, ntag4, RED_W);
	writeToLog(date, logDegree, 3, ntag3, GRN_W);
	
    //Set Simulation Timer
	gettimeofday(&tv, NULL);
	simulationTimer = tv.tv_sec;
	
	//set up state of program
	char currentState = 'n'; //north is green
	
	//declare arrays to store raw data for each intersection direction
	struct StatsOverInterval north[10000];
	int sizeNorth = 0;
	struct StatsOverInterval west[10000];
	int sizeWest = 0;
	
	//state machine variables
	bool done = false;
	int timerMain;
	int timerOpti;
	int carCounter = 0;
	struct StatsOverInterval intervalStat;
	

	//Initialising intersection lights
	light_on(GRN_W);
	sleep(1);
	light_off(GRN_W);
	sleep(1);
	light_on(RED_W);
	sleep(1);
	light_off(RED_W);
	sleep(1);
	light_on(GRN_N);
	sleep(1);
	light_off(GRN_N);
	sleep(1);
	light_on(RED_N);
	sleep(1);
	light_off(RED_N);
	sleep(1);

	//state machine
	while (deltaTime(simulationTimer) < simulationTime)
	{
		switch (currentState)
		{
			case 'n':
			
				light_on(GRN_N);
				light_off(RED_N);
				light_on(RED_W);
				light_off(GRN_W);
				
				done = false;
				timerMain = timeUpdate();
				timerOpti = timeUpdate();
				carCounter = 0;
				
				while (!done)
				{
					if (carPassed(SENS_N_IN, SENS_N_OUT, defaultThreshold))
					{
						carCounter++;
						writeToLog(date, logDegree, 8, 0, 0);
						timerOpti = timeUpdate();
					}
					if (deltaTime(timerMain) > defaultTimeInterval)
					{
						done = true;
						intervalStat.numCars = carCounter;
						intervalStat.timeInterval = defaultTimeInterval;
						intervalStat.cps = calcCarsPerSecond(intervalStat);
						north[sizeNorth] = intervalStat;
						sizeNorth++;
						
						//Log appropriate interval information
						writeToLog(date, logDegree, 4, northTag, 0);
						writeToLog(date, logDegree, 5, northTag, defaultTimeInterval);
						writeToLog(date, logDegree, 14, northTag, carCounter);
						writeToLog(date, logDegree, 6, northTag, intervalStat.cps);

					}
					else if (deltaTime(timerOpti) > 10)
					{
						done = true;
						intervalStat.numCars = carCounter;
						intervalStat.timeInterval = deltaTime(timerMain);
						intervalStat.cps = calcCarsPerSecond(intervalStat);
						north[sizeNorth] = intervalStat;
						sizeNorth++;
						
						//Log appropriate interval information
						writeToLog(date, logDegree, 4, northTag, defaultTimeInterval - intervalStat.timeInterval);
						writeToLog(date, logDegree, 5, northTag, intervalStat.timeInterval);
						writeToLog(date, logDegree, 14, northTag, carCounter);
						writeToLog(date, logDegree, 6, northTag, intervalStat.cps);
					}
					
					usleep(100000);
				}
				
				currentState = 'w';
				
				break;
			
			case 'w':
			
				light_on(GRN_W);
				light_off(RED_W);
				light_on(RED_N);
				light_off(GRN_N);
				
				done = false;
				timerMain = timeUpdate();
				timerOpti = timeUpdate();
				carCounter = 0;
				
				while (!done)
				{
					if (carPassed(SENS_W_IN, SENS_W_OUT, defaultThreshold))
					{
						carCounter++;
						writeToLog(date, logDegree, 8, 0, 0);
						timerOpti = timeUpdate();
					}
					if (deltaTime(timerMain) > defaultTimeInterval)
					{
						done = true;
						intervalStat.numCars = carCounter;
						intervalStat.timeInterval = defaultTimeInterval;
						intervalStat.cps = calcCarsPerSecond(intervalStat);
						west[sizeWest] = intervalStat;
						sizeWest++;
						
						//Log appropriate interval information
						writeToLog(date, logDegree, 4, westTag, 0);
						writeToLog(date, logDegree, 5, westTag, defaultTimeInterval);
						writeToLog(date, logDegree, 14, westTag, carCounter);
						writeToLog(date, logDegree, 6, westTag, intervalStat.cps);

					}
					else if (deltaTime(timerOpti) > 10)
					{
						done = true;
						intervalStat.numCars = carCounter;
						intervalStat.timeInterval = deltaTime(timerMain);
						intervalStat.cps = calcCarsPerSecond(intervalStat);
						west[sizeWest] = intervalStat;
						sizeWest++;
						
						//Log appropriate interval information
						writeToLog(date, logDegree, 4, westTag, defaultTimeInterval - intervalStat.timeInterval);
						writeToLog(date, logDegree, 5, westTag, intervalStat.timeInterval);
						writeToLog(date, logDegree, 14, westTag, carCounter);
						writeToLog(date, logDegree, 6, westTag, intervalStat.cps);
					}
					
					usleep(100000);
				}
				
				currentState = 'n';
				
				break;
		}
	}
	

	light_off(GRN_W);
	light_off(RED_W);
	light_off(RED_N);
	light_off(GRN_N);

	//compute statistics
	struct StatsOverSimulation simNorth = computeStatsOverSimulation(north, sizeNorth);
	struct StatsOverSimulation simWest = computeStatsOverSimulation(west, sizeWest);
	
	//write stats to file
	writeStatsToFile (date, north, sizeNorth, west, sizeWest, simNorth, simWest);
	
	writeToLog(date, logDegree, 12, 0, 0);
	return 0;

}

bool light_on (const unsigned int port)
{
	gpio_set_value(port, 1);  	//set gpio value to high
	
	return true;
}

bool light_off (const unsigned int port)
{
	gpio_set_value(port, 0); 	//set gpio value to low
	
	return true;
}

int deltaTime(const int oldTime)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);  		//get current time

	int currentTime = tv.tv_sec; 		//take the time in seconds
	int change = currentTime - oldTime; 	//calculate the change in time
	
	return change;
}

int timeUpdate()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);	//get current time

	int newTime = tv.tv_sec;
	return newTime;			//return the current time
}

int minutesToSeconds(int minutes)
{
	return minutes*60;
}

float readSensor (const unsigned int gpioIn, const unsigned int gpioOut)
{
	
	float detectedRange = 0;
	
	//gpioIn reads data from the sensor (Echo)
	gpio_request(gpioIn, NULL);
	gpio_direction_input(gpioIn);
	
	//gpioOut requests data from the sensor (Trig)
	gpio_request(gpioOut, NULL);
	gpio_direction_output(gpioOut, 0);
	
	//Trigger Sensor
	gpio_set_value(gpioOut, 1);
	usleep(15);
	gpio_set_value(gpioOut, 0);
	
	//Wait for echo
	int echo = gpio_get_value(gpioIn);
	int iterations = 0;
	
	float microsecondCounter = 0;
	
	while ((echo==0) && (iterations < 5000))
	{
		usleep(1);
		echo = gpio_get_value(gpioIn);
		iterations++;
	}
	
	//sensor didn't work
	if (iterations >= 5000)
	{
		detectedRange = -2;
	}
	//calculate range
	else
	{
		microsecondCounter = 0;
		
		echo = gpio_get_value(gpioIn);
		
		while ((echo != 0) && (microsecondCounter < 32000))
		{
			usleep(10);
			echo = gpio_get_value(gpioIn);
			microsecondCounter += 10;
		}
		
		//detected nothing
		if (microsecondCounter >= 32000)
		{
			detectedRange = -1;
		}
		//find the range in centimeters
		else
		{
			detectedRange = microsecondCounter/58;
		}
	}
	

	return detectedRange;

}

bool carPassed(const unsigned int gpioIn, const unsigned int gpioOut, const float threshold)
{
	if (readSensor(gpioIn, gpioOut) <= threshold) //if sensor output is less than threshold
	{
		return true;	//car has passed
	}
	else
	{
		return false;	//no car
	}
}

float calcCarsPerSecond (struct StatsOverInterval intervalStats)
{
	float cps = intervalStats.numCars/intervalStats.timeInterval;
	return cps;
}

float calcTotalTime (struct StatsOverInterval statsInterval[ ], int sizeStats)
{
	float total = 0;
	for (int i = 0; i < sizeStats; i++)
	{
		total += statsInterval[i].timeInterval;
	}
	return total;
}

int calcTotalCars (struct StatsOverInterval statsInterval[ ], int sizeStats)
{
	int total = 0;
	for (int i = 0; i < sizeStats; i++)
	{
		total += statsInterval[i].numCars;
	}
	return total;
}

int calcMaxCars (struct StatsOverInterval statsInterval[ ], int sizeStats)
{
	int max = statsInterval[0].numCars;
	for (int i = 0; i < sizeStats ; i++)
	{
		if (statsInterval[i].numCars > max)
		{
			max = statsInterval[i].numCars;
		}
	}
	return max;
}

int calcMinCars (struct StatsOverInterval statsInterval[ ], int sizeStats)
{
	int min = statsInterval[0].numCars;
	for (int i = 0; i < sizeStats; i++)
	{
		if (statsInterval[i].numCars < min)
		{
			min = statsInterval[i].numCars;
		}
	}
	return min;
}

float calcAvgCars (struct StatsOverInterval statsInterval[ ], int sizeStats)
{
	float total = 0;
	for (int i = 0; i < sizeStats; i++)
	{
		total += statsInterval[i].numCars;
	}
	return total/sizeStats;
}

float calcAvgTime (struct StatsOverInterval statsInterval[ ], int sizeStats)
{
	float total = 0;
	for (int i = 0; i < sizeStats; i++)
	{
		total += statsInterval[i].timeInterval;
	}
	return total/sizeStats;
}

int calcModeCars (struct StatsOverInterval statsInterval[], int sizeStats, int modes[])
{
	int numModes = 0;
	
	int newData[sizeStats];
	
	for (int i = 0; i < sizeStats; i++)
	{
		newData[i] = statsInterval[i].numCars;
	}
	
	sortInt (newData,sizeStats);
	
	int sizeMode = 0;
	int indexMode = 0;
	int maxCount = 1;
	int modeCount = 1;
	bool firstTime = true;
	
	for (int i = 0; i <= sizeStats-1; i++)
	{
		if (newData[i] == newData[i+1])
		{
			modeCount++;
			
			if (modeCount == maxCount)
			{
				modes[indexMode] = newData[i];
				indexMode++;
				sizeMode++;
			}
			if (modeCount > maxCount)
			{
				maxCount = modeCount;
				
				if (firstTime)
				{
					indexMode = 0;
					sizeMode = 0;
					modes[indexMode] = newData[i];
					sizeMode++;
					indexMode++;
					
					firstTime = false;
				}
			}
		}
		else
		{
			if (maxCount == 1)
			{
				modes[indexMode] = newData[i];
				indexMode++;
				sizeMode++;
				firstTime = true;
			}
			else
			{
				modeCount = 1;
				firstTime = true;
			}
		}
			
	}
	
	numModes = sizeMode;

	return numModes;	
}

bool sortInt(int dataset[], const int size)
{
	return selectionInt (dataset, size, 0);
}

bool selectionInt(int dataset[], const int size, int index)
{
	if (index == size)
	{
		return true;
	}
	else
	{
		int min = dataset[index];
		int posMin = index;
		
		for (int i = index; i < size; i++)
		{
			if (dataset[i] < min)
			{
				min = dataset[i];
				posMin = i;
			}
		}
		
		int temp = dataset[index];
		dataset[index] = dataset[posMin];
		dataset[posMin] = temp;
		
		return selectionInt(dataset, size, index+1);
	}
}

bool sortFloat(float dataset[], const int size)
{
	return selectionFloat (dataset, size, 0);
}

bool selectionFloat(float dataset[], const int size, int index)
{
	if (index == size)
	{
		return true;
	}
	else
	{
		float min = dataset[index];
		int posMin = index;
		
		for (int i = index; i < size; i++)
		{
			if (dataset[i] < min)
			{
				min = dataset[i];
				posMin = i;
			}
		}
		
		float temp = dataset[index];
		dataset[index] = dataset[posMin];
		dataset[posMin] = temp;
		
		return selectionFloat(dataset, size, index+1);
	}
}

float calcMaxCPS (struct StatsOverInterval statsInterval[ ], int sizeStats)
{
	float max = statsInterval[0].cps;
	for (int i = 0; i < sizeStats ; i++)
	{
		if (statsInterval[i].cps > max)
		{
			max = statsInterval[i].cps;
		}
	}
	return max;
}

float calcMinCPS (struct StatsOverInterval statsInterval[ ], int sizeStats)
{
	float min = statsInterval[0].cps;
	for (int i = 0; i < sizeStats; i++)
	{
		if (statsInterval[i].cps < min)
		{
			min = statsInterval[i].cps;
		}
	}
	return min;
}

float calcAverageCPS (struct StatsOverInterval statsInterval[ ], int sizeStats)
{
	float total = 0;
	for (int i = 0; i < sizeStats; i++)
	{
		total += statsInterval[i].cps;
	}
	return total/sizeStats;
}

float calcMedianCPS (struct StatsOverInterval statsInterval[ ], int sizeStats)
{
	float median;
	
	int j = 0;
	int i = 0;
	float set[sizeStats];

	while (j < sizeStats)
	{
		set[j] = statsInterval[j].cps;
		j++;
	}
	
	sortFloat(set,sizeStats);
	
	if (sizeStats % 2 == 0)
	{
		median = (set[sizeStats/2] + set[(sizeStats/2)-1])/2;
	}
	else
	{
		median = set[(sizeStats-1)/2];
	}
		
	return median;
}

float calcPopStdDevCPS(struct StatsOverInterval statsInterval[], int sizeStats) 
{
	float devset[sizeStats];
	int j = 0;
	float popdev = 0;
	while (j < sizeStats)
	{
		devset[j] = (statsInterval[j].cps-calcAverageCPS(statsInterval, sizeStats))*(statsInterval[j].cps-calcAverageCPS(statsInterval, sizeStats));
		j++;
	}
	
	j = 0;
	while (j < sizeStats)
	{
		popdev += devset[j];
			
		j++;
	}
	popdev = sqrt(popdev/sizeStats);
	
	if (sizeStats <= 0)
	{
		return -1;
	}
	
	return popdev;
}

float calcSmplStdDevCPS(struct StatsOverInterval statsInterval[ ], int sizeStats) 
{
	float devset[sizeStats];
	int j = 0;
	float smpldev = 0;
	while (j < sizeStats)
	{
		devset[j] = (statsInterval[j].cps-calcAverageCPS(statsInterval, sizeStats))*(statsInterval[j].cps-calcAverageCPS(statsInterval, sizeStats));
		j++;
	}
	
	j = 0;
	while (j < sizeStats)
	{
		smpldev += devset[j];
			
		j++;
	}
	
	if (sizeStats == 1)
	{
		return -1;
	}
	else
	{
		smpldev = sqrt(smpldev/(sizeStats-1));
	}
	
	return smpldev;
}

float calcTimeSaved (struct StatsOverInterval statsInterval[ ], int sizeStats, const float defaultIntersectionTime)
{
	float total = 0;
	for (int i = 0; i < sizeStats; i++)
	{
		total += statsInterval[i].timeInterval;
	}
	return (sizeStats*defaultIntersectionTime - total);
}

struct StatsOverSimulation computeStatsOverSimulation(struct StatsOverInterval intervalStats[], int sizeStats)
{
	char funcTag[] = "StatsOverSimulation";
	writeToLog(date, logDegree, 9, funcTag, 0);
	
	struct StatsOverSimulation statsSim;
	statsSim.totalCars = calcTotalCars(intervalStats, sizeStats);
	statsSim.totalTime = calcTotalTime(intervalStats, sizeStats);
	statsSim.maxCars = calcMaxCars(intervalStats, sizeStats);
	statsSim.minCars = calcMinCars(intervalStats, sizeStats);
	statsSim.averageCars = calcAvgCars (intervalStats, sizeStats);
	statsSim.averageTime = calcAvgTime (intervalStats, sizeStats);
	statsSim.numModes = calcModeCars(intervalStats, sizeStats, statsSim.modeCars);
	statsSim.maxCPS = calcMaxCPS(intervalStats, sizeStats);
	statsSim.minCPS = calcMinCPS(intervalStats, sizeStats);
	statsSim.avgCPS = calcAverageCPS(intervalStats, sizeStats);
	statsSim.medianCPS = calcMedianCPS(intervalStats, sizeStats);
	statsSim.popStdDevCPS = calcPopStdDevCPS(intervalStats, sizeStats);
	statsSim.smplStdDevCPS = calcSmplStdDevCPS(intervalStats, sizeStats);
	statsSim.timeSaved = calcTimeSaved(intervalStats, sizeStats, 30);
	
	writeToLog(date, logDegree, 10, funcTag, 0);
	return statsSim;
	
}

bool writeStatsToFile (char filename[], struct StatsOverInterval statsIntervalNorth[], int sizeNorth, 
						struct StatsOverInterval statsIntervalWest[], int sizeWest, 
						struct StatsOverSimulation statsSimNorth, struct StatsOverSimulation statsSimWest)
{
	char funcTag[] = "writeStatsToFile";
	writeToLog(date, logDegree, 9, funcTag, 0);
	
	FILE* fptr;
	
	//North Raw Data
	char fullFilenameRawNorth[100];
	strcpy(fullFilenameRawNorth, filename);
	char extensionRawNorth[] = "_NORTH_RAW.rawstat";
	strcat(fullFilenameRawNorth, extensionRawNorth);
	
	fptr = fopen(fullFilenameRawNorth, "w");
	fprintf(fptr, "Raw Data for North Direction\r\nx--------x--------x-------x--------x\r\n\r\n");
	for (int i = 0; i < sizeNorth; i++)
	{
		fprintf(fptr, "Time Interval #%d: \r\nNumber of Cars: %d\r\nTime Interval Length: %f s\r\nCars Per Second: %f cps\r\n\r\n", i+1, statsIntervalNorth[i].numCars, statsIntervalNorth[i].timeInterval, statsIntervalNorth[i].cps);
	}
	writeToLog(date, logDegree, 11, fullFilenameRawNorth, 0);
	fclose(fptr);
	
	//West Raw Data
	char fullFilenameRawWest[100];
	strcpy(fullFilenameRawWest, filename);
	char extensionRawWest[] = "_WEST_RAW.rawstat";
	strcat(fullFilenameRawWest, extensionRawWest);
	
	fptr = fopen(fullFilenameRawWest, "w");
	fprintf(fptr, "Raw Data for West Direction\r\nx--------x--------x-------x--------x\r\n\r\n");
	for (int i = 0; i < sizeWest; i++)
	{
		fprintf(fptr, "Time Interval #%d: \r\nNumber of Cars: %d\r\nTime Interval Length: %f s\r\nCars Per Second: %f cps\r\n\r\n", i+1, statsIntervalWest[i].numCars, statsIntervalWest[i].timeInterval, statsIntervalWest[i].cps);
	}
	writeToLog(date, logDegree, 11, fullFilenameRawWest, 0);
	fclose(fptr);
	
	//North Simulation Stats
	char fullFilenameStatNorth[100];
	strcpy(fullFilenameStatNorth, filename);
	char extensionStatNorth[] = "_NORTH_SIM.stat";
	strcat(fullFilenameStatNorth, extensionStatNorth);
	
	fptr = fopen(fullFilenameStatNorth, "w");
	fprintf(fptr, "Simulation Statistics for North Direction\r\nx--------x--------x-------x--------x\r\n\r\n");
	fprintf(fptr, "Total Cars: %d\r\n", statsSimNorth.totalCars);
	fprintf(fptr, "Total Time: %f s\r\n", statsSimNorth.totalTime);
	fprintf(fptr, "Max Cars: %d\r\n", statsSimNorth.maxCars);
	fprintf(fptr, "Min Cars: %d\r\n", statsSimNorth.minCars);
	fprintf(fptr, "Average Cars: %f\r\n", statsSimNorth.averageCars);
	fprintf(fptr, "Average Time: %f s\r\n", statsSimNorth.averageTime);
	fprintf(fptr, "Mode(s) # of Cars: ");
	for (int i = 0; i < statsSimNorth.numModes; i++)
	{
		fprintf(fptr, "%d, ", statsSimNorth.modeCars[i]);
	}
	fprintf(fptr, "\r\n");
	fprintf(fptr, "Maximum Cars Per Second: %f cps\r\n", statsSimNorth.maxCPS);
	fprintf(fptr, "Minimim Cars Per Second: %f cps\r\n", statsSimNorth.minCPS);
	fprintf(fptr, "Average Cars Per Second: %f cps\r\n", statsSimNorth.avgCPS);
	fprintf(fptr, "Median Cars Per Second: %f cps\r\n", statsSimNorth.medianCPS);;
	fprintf(fptr, "Population Standard Deviation Cars Per Second: %f cps\r\n", statsSimNorth.popStdDevCPS);
	fprintf(fptr, "Sample Standard Deviation Cars Per Second: %f cps\r\n", statsSimNorth.smplStdDevCPS);
	fprintf(fptr, "Time Saved: %f s\r\n\r\n", statsSimNorth.timeSaved);
	
	fprintf(fptr, "         _______\r\n       //  ||  \\\\\r\n _____//___||__\\ \\___\r\n )  _    HIIIII-5 _    \\\r\n |_/  \\_________ /  \\___|\r\n___ \\_/_________ \\_/______\r\n");
	
	writeToLog(date, logDegree, 11, fullFilenameStatNorth, 0);
	fclose(fptr);
	
	//West Simulation Stats
	char fullFilenameStatWest[100];
	strcpy(fullFilenameStatWest, filename);
	char extensionStatWest[] = "_WEST_SIM.stat";
	strcat(fullFilenameStatWest, extensionStatWest);
	
	fptr = fopen(fullFilenameStatWest, "w");
	fprintf(fptr, "Simulation Statistics for West Direction\r\nx--------x--------x-------x--------x\r\n\r\n");
	fprintf(fptr, "Total Cars: %d\r\n", statsSimWest.totalCars);
	fprintf(fptr, "Total Time: %f s\r\n", statsSimWest.totalTime);
	fprintf(fptr, "Max Cars: %d\r\n", statsSimWest.maxCars);
	fprintf(fptr, "Min Cars: %d\r\n", statsSimWest.minCars);
	fprintf(fptr, "Average Cars: %f\r\n", statsSimWest.averageCars);
	fprintf(fptr, "Average Time: %f s\r\n", statsSimWest.averageTime);
	fprintf(fptr, "Mode(s) # of Cars: ");
	for (int i = 0; i < statsSimWest.numModes; i++)
	{
		fprintf(fptr, "%d, ", statsSimWest.modeCars[i]);
	}
	fprintf(fptr, "\r\n");
	fprintf(fptr, "Maximum Cars Per Second: %f cps\r\n", statsSimWest.maxCPS);
	fprintf(fptr, "Minimim Cars Per Second: %f cps\r\n", statsSimWest.minCPS);
	fprintf(fptr, "Average Cars Per Second: %f cps\r\n", statsSimWest.avgCPS);
	fprintf(fptr, "Median Cars Per Second: %f cps\r\n", statsSimWest.medianCPS);;
	fprintf(fptr, "Population Standard Deviation Cars Per Second: %f cps\r\n", statsSimWest.popStdDevCPS);
	fprintf(fptr, "Sample Standard Deviation Cars Per Second: %f cps\r\n", statsSimWest.smplStdDevCPS);
	fprintf(fptr, "Time Saved: %f s\r\n\r\n", statsSimWest.timeSaved);
	
	fprintf(fptr, "         _______\r\n       //  ||  \\\\\r\n _____//___||__\\ \\___\r\n )  _    HIIIII-5 _    \\\r\n |_/  \\_________ /  \\___|\r\n___ \\_/_________ \\_/______\r\n");
	
	writeToLog(date, logDegree, 11, fullFilenameStatWest, 0);
	fclose(fptr);
	
	writeToLog(date, logDegree, 10, funcTag, 0);
	return true;
	
}

bool writeToLog(char filename[], int degreeLogging, int logMessageNumber, char tag[], float value)
{
	int nameLength = 0;
	
	while(filename[nameLength] != 0)
	{
		nameLength++;
	}
	
	char logName[nameLength+5];
	char extension[] = ".log";
	
	strcpy(logName, filename);
	strcat(logName, extension);
	
	FILE* fptr;
	
	fptr = fopen(logName, "a");
	
	switch(logMessageNumber)
	{
		case 0:
		
			if(degreeLogging > 0)
			{
				fprintf(fptr, "Welcome to the log file!\r\n");
			}
			
			break;
			
		case 1:
			
			if(degreeLogging > 0)
			{
				fprintf(fptr, "Sensor at port: %f.\r\n", value);
			}
			
			break;
		
		case 2:
		
			if(degreeLogging > 0)
			{
				fprintf(fptr, "Red light at port: %f.\r\n", value);
			}
			
			break;
			
		case 3:
			
			if(degreeLogging > 0)
			{
				fprintf(fptr, "Green light at port: %f.\r\n", value);
			}
			
			break;
			
		case 4:
		
			if(degreeLogging > 5)
			{
				fprintf(fptr, "Time Saved: %f seconds, Tag: %s.\r\n", value, tag); 
			}
			
			break;
			
		case 5:
		
			if(degreeLogging > 5)
			{
				fprintf(fptr, "Time interval for green light: %f seconds, Tag: %s.\r\n", value, tag);
			}
			
			break;
			
		case 6:
		
			if(degreeLogging > 5)
			{
				fprintf(fptr, "Cars per second during 1 green light: %f, Tag: %s.\r\n", value, tag);
			}
			
			break;
			
		case 7:
		
			if(degreeLogging > 9)
			{
				fprintf(fptr, "Sensor value: %f, Tag: %s.\r\n", value, tag);
			}
			
			break;
			
		case 8:
		
			if(degreeLogging > 9)
			{
				fprintf(fptr, "Car passed.\r\n");
			}
			
			break;
			
		case 9:
		
			if(degreeLogging > 0)
			{
				fprintf(fptr, "Currently in function %s.\r\n", tag);
			}
			
			break;
			
		case 10:
		
			if(degreeLogging > 0)
			{
				fprintf(fptr, "Exiting function %s.\r\n", tag);
			}
			
			break;
			
		case 11:
		
			if(degreeLogging > 0)
			{
				fprintf(fptr, "Successfully written statistics file %s.\r\n", tag);
			}
			
			break;
			
		case 12:
		
			if(degreeLogging > 0)
			{
				fprintf(fptr, "Simulation Terminated.\r\n");
			}
			
			break;
			
		case 13:
		
			if(degreeLogging > 0)
			{
				fprintf(fptr, "Value of %s: %f\r\n", tag, value);
			}
			
			break;
			
		case 14:
		
			if(degreeLogging > 5)
			{
				fprintf(fptr, "Number of cars in interval: %f, Tag: %s.\r\n", value, tag);
			}
			
			break;
	}
			
	fclose(fptr);
	
	return true;
}

