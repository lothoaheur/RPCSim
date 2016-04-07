/*
 * TAvalanche1D.hpp
 * 
 * Copyright 2016 Vincent Français <francais@clermont.in2p3.fr>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */
 
 
#pragma once

#include <vector>
#include <utility>
#include <map>
#include <chrono>

#include "RngStream.hpp"
#include "TRandomEngineSFMT.hpp"
#include "TRandomEngineMRG.hpp"
#include "TRandomEngineMT.hpp"
#include "TRandomEngine.hpp"

#include "MediumMagboltz.hh"
#include "SolidBox.hh"
#include "GeometrySimple.hh"
#include "ComponentConstant.hh"
#include "Sensor.hh"
#include "TrackHeed.hh"

#include "TAvalanche.hpp"
#include "TDetector.hpp"
#include "TResult.hpp"

#if defined( _DEBUG ) || defined( DEBUG ) || defined (__DEBUG__)
#   ifndef DEBUG
#       define DEBUG
#   endif
#endif



#if defined(DEBUG)
#	include <assert.h>
#	define DEBUGASSERT                assert
#else
#   define DEBUGASSERT( x )               {}
#endif

using namespace std;

enum EAvalancheStatus{
	AVAL_SUCCESS,
	AVAL_NO_ERROR,
	
	AVAL_CLT_FAIL,
	AVAL_MULT_FAIL,
	AVAL_ERROR_GRID_NOT_EMPTY,
	AVAL_ERROR_TIMESTEP_EXCEEDING_LIMIT,
	AVAL_EXPLOSIVE_BEHAVIOR
};

class TAvalanche1D : public TAvalanche {
	
	public:
	//TAvalanche1D(TDetector* det, bool const& randomSeed=false);
	TAvalanche1D(TDetector* det, sfmt_t sfmt, bool const& randomSeed=false);
	
	~TAvalanche1D();
	
	void initialiseTrackHeed(const string& particleName, const double& momentum, const double& x0, const double& theta);
	void initialiseSingleCluster(const double& x0, const double& n0=1);
		
	void simulateEvent();
	void computeInducedCharges();
	void computeInducedSignal();
	void computeInducedSignal2();
	void computeChargeSpectra(const double& x0, const double& angle, const int& nEvents);
	void computeClusterDensity(const string& particleName, const double& Pmin, const double& Pmax, const int& steps);
	void computeElectronsProduction(const string& particleName, const double& P, const int& nTracks);
	void checkDetectorGrid();
	bool propagate();
	void computeLongitudinalDiffusion();
	bool avalanche();
	inline double n_moy(const double& x);
	double multiplicationRiegler(const double& x, const double& s);
	double electronMultiplication(const double& n);
	double multiplicationCLT(const double& x, const double& n);
	void makeResultFile();
	void computeSCEffect();
	double interpolateEbar(const double& z, const double& zp, const double& l);
	
	void makeSnapshot();
	
	void disableSpaceChargeEffect() {bComputeSpaceChargeEffet = false;}
	void enableSpaceChargeEffect() {bComputeSpaceChargeEffet = true;}
	
	void setElectronMultiplicationThreshold();
	
	vector<double> getInducedSignal() const {return fSignal;}
	vector<double> getInducedCharges() const {return fCharges;}
	TResult getResultFile() const {return fResult;}
	
	void testInterpolation();
	size_t getIndex3D(const size_t& i, const size_t& j, const size_t& k);
	
	
	
	private:
	uint tId;
	int Id;
	
	TDetector* fDet;
	DetectorGeometry fGeometry;
	
	int iNstep;
	double fGapWidth;
	const double* fResistiveLayersWidth;
	double fDt;
	double fDx;
	
	int iEbarTableSize;
	vector<double> fEbarTable;
	string fEbarTableHexName;
	vector<double> fEbarZarray;
	vector<double> fEbarZparray;
	vector<double> fEbarLarray;
	
	int iCurrentDetectorStep;
	int iTimeStep;
	
	int iThrCrossTimeStep;
	double fChargeThres;
	
	int iNElectronsSize;
	vector<double> fElecDetectorGrid;
	vector<double> fPosIonDetectorGrid;
	vector<double> fNegIonDetectorGrid;
	
	double fNelecAnode;
	vector< pair<double,double> > fElecOnAnode;
	
	vector<double> fNElectrons;
	vector<double> fSignal;
	vector<double> fCharges;
	vector<double> fTotalCharges;
	
	double fDiffL;
	double fDiffT;
	vector<double> fVx;
	vector<double> fAlpha;
	vector<double> fEta;
	vector<double> fE;
	double fEini;
	
	double fClusterDensity;
	map<double,int> fClustersX, fClustersY, fClustersZ;
	
	double fLongiDiffSigma;
	
	double fThrCLT;
	double fSpaceChargeLimit;
	double fLongiDiffComputeLimit;
	
	bool bVerbose;
	bool bAvalancheInitialised;
	bool bEbarComputed;
	bool bSnapshots;
	bool bThrCrossTime;
	bool bComputeSpaceChargeEffet;
	bool bHasReachSpaceChargeLimit;
	
	EAvalancheStatus eAvalStatus;
	
	RngStream* fRandRng;
	TRandomEngineMRG* fRandRngCLT;
	RngStream* fRandRngLongiDiff;

	TRandomEngine* fRNG;
	
	long double fRandomNumberGenerated;
	
	TResult fResult;
	
	//TTimer fTimer;
};
