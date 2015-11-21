#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <vector>
#include <utility>
#include <sys/stat.h>
#include <typeinfo>
#include <fcntl.h>
#include <cassert>

#include "MediumMagboltz.hh"
#include "SolidBox.hh"
#include "GeometrySimple.hh"
#include "ComponentConstant.hh"
#include "Sensor.hh"
#include "TrackHeed.hh"
#include "Plotting.hh"

//#include <boost/numeric/ublas/matrix.hpp>

//#include <TCanvas.h>
//#include <TGraph.h>
//#include <TROOT.h>
//#include <TApplication.h>
//#include <TH1F.h>

//#include "electron_avalanche.hpp"
//#include "cluster.hpp"
//#include "RPCSim.hpp"
#include "TDetector.hpp"
#include "TAvalanche.hpp"
#include "helper_functions.hpp"
#include "TThreadsFactory.h"
#include "TResult.hpp"

#define PATH_MAX 0x1000
#define PI 3.14159265358979323846

using namespace std;
using namespace Garfield;

typedef unsigned int uint;

pthread_mutex_t gPipeLock;
pthread_mutex_t gTrackLock;
int gPipe[2];
TResult gNullResult;

//struct data{
	//RPCSim sim;
	////string partName;
	////double partMomentum;
	////double x0;
	////double theta;
//};



void * wrapperFunction(void * Arg){
	
	assert(Arg != NULL);

	//struct data * d = reinterpret_cast<struct data *>(Arg);
	TDetector * detector = reinterpret_cast<TDetector *>(Arg);
	
	TResult result;
	
	TAvalanche avalanche(detector);
	
	sem_post(TThreadsFactory::GetInstance()->GetInitLock());
	
	pthread_mutex_lock(&gTrackLock);
	//avalanche.initialiseTrackHeed(detector,"muon",5.e9,0.,0.);
	avalanche.initialiseSingleCluster(0);
	pthread_mutex_unlock(&gTrackLock);
	
	avalanche.simulateEvent();
	
	//result.fInducedCharge = avalanche.getInducedCharges().back();
	
	result = avalanche.getResultFile();
	
	pthread_mutex_lock(&gPipeLock);
    write(gPipe[1], &result, sizeof(result));
    pthread_mutex_unlock(&gPipeLock);
	
	return 0;
}

void * WriteResults(void * Arg)
{
    TResult result;
    char * outputFile = reinterpret_cast<char *>(Arg);
    //string outFileName = reinterpret_cast<string>(Arg);
    int outFD;
	ofstream outFile(outputFile, ios::out | ios::trunc);
	
    /* Open the output file */
    outFD = open("out/out.dat", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    assert(outFD != -1);

    /* Read the incoming event */
    while (read(gPipe[0], &result, sizeof(TResult)) > 0)
    {
        /* End signal => stop dealing with data */
        if (memcmp(&result, &gNullResult, sizeof(TResult)) == 0)
            break;

        /* Write the event to the output file */
        write(outFD, &result, sizeof(TResult));
        outFile << result.fInducedCharge << endl;
    }

    close(outFD);
    outFile.close();

    return 0;
}

int main(int argc, char** argv) {
	unsigned int nThreads = 1;
    unsigned long nEvents = 1;
    
    char outputFile[PATH_MAX];
    if(argc > 2)
		strncpy(outputFile, argv[2], PATH_MAX - 1);
	else
		strncpy(outputFile, "out/out2.dat", PATH_MAX - 1);
    outputFile[PATH_MAX - 1] = '\0';
    
    pthread_t writingThread;
    void * ret;
    
    /* Initialize our pipe lock */
    pthread_mutex_init(&gPipeLock, 0);
    pthread_mutex_init(&gTrackLock, 0);

    /* Start our threads factory */
    TThreadsFactory::GetInstance()->SetMaxThreads(nThreads);
    
    /* Init our null event */
    memset(&gNullResult, 0, sizeof(TResult));
    
    /* Init our detector */
	MediumMagboltz* gas = new MediumMagboltz();
	gas->SetComposition("c2h2f4",96.7,"ic4h10",3.,"sf6",0.3);
	gas->SetTemperature(293.15);
	gas->SetPressure(760.);
	
	double HV = 50000.;
	if(argc > 1)	HV = atof(argv[1])*1000.;
	
	DetectorGeometry geom;
	geom.gapWidth = 0.2;
	geom.resistiveLayersWidth[0] = 0.2;
	geom.resistiveLayersWidth[1] = 0.2;
	geom.relativePermittivity[0] = 10.;
	geom.relativePermittivity[1] = 10.;
	TDetector* detector = new TDetector(geom,500);
	detector->setGasMixture(gas);
	detector->setElectricField(HV,0.,0.);
	detector->initialiseDetector();
	
	//TAvalanche* avalanche = new TAvalanche(detector);
	//avalanche->computeClusterDensity(detector, "pion", 6.e7, 15.e9, 600);
	//delete avalanche;

	
    /* Open the communication pipe */
    if (pipe(gPipe) == -1){
		goto end;
    }

    /* Start our background writing thread */
    if (pthread_create(&writingThread, 0, WriteResults, outputFile) != 0){
		goto end2;
    }

    /* Hot loop, the simulation happens here */
    for (unsigned long i = 0; i < nEvents; ++i){
		TThreadsFactory::GetInstance()->CreateThread(wrapperFunction, detector);
    }

    /* Wait for all the propagations to finish */
    TThreadsFactory::GetInstance()->WaitForAllThreads();

    /* Send the end signal to writer */
    write(gPipe[1], &gNullResult, sizeof(TResult));

    /* Wait for the end of the writer */
    pthread_join(writingThread, &ret);

end2:
    close(gPipe[0]);
    close(gPipe[1]);
end:
    pthread_mutex_destroy(&gPipeLock);
    pthread_mutex_destroy(&gTrackLock);
    delete detector;

    return 0;
}