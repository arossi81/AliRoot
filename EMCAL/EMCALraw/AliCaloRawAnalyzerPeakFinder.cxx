// -*- mode: c++ -*-
/**************************************************************************
 * This file is property of and copyright by                              *
 * the Relativistic Heavy Ion Group (RHIG), Yale University, US, 2009     *
 *                                                                        *
 * Primary Author: Per Thomas Hille  <perthomas.hille@yale.edu>           *
 *                                                                        *
 * Contributors are mentioned in the code where appropriate.              *
 * Please report bugs to   perthomas.hille@yale.edu                       *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

// EMCal system
#include "AliCaloRawAnalyzerPeakFinder.h"
#include "AliCaloPeakFinderVectors.h"
#include "AliCaloBunchInfo.h"
#include "AliCaloFitResults.h"

// AliRoot system
#include "AliCDBEntry.h"
#include "AliCDBManager.h"
#include "AliLog.h"

// ROOT system
#include "TMath.h"
#include "TFile.h"


#include <iostream>

using namespace std;

/// \cond CLASSIMP
ClassImp( AliCaloRawAnalyzerPeakFinder ) ;
/// \endcond

///
/// Constructor
//_______________________________________________________________________
AliCaloRawAnalyzerPeakFinder::AliCaloRawAnalyzerPeakFinder() :AliCaloRawAnalyzer("Peak-Finder", "PF"),  
							      fPeakFinderVectors(0),
							      fRunOnAlien(false),
							      fIsInitialized(false)
{
  fAlgo= Algo::kPeakFinder;
  
  fPeakFinderVectors = new AliCaloPeakFinderVectors() ;
  
  ResetVectors();
  
  LoadVectorsOCDB();
}

///
/// Reset vectors
//_______________________________________________________________________
void  
AliCaloRawAnalyzerPeakFinder::ResetVectors()
{
  for(int i=0; i < PF::MAXSTART; i++)
  {
    for(int j=0; j < PF::SAMPLERANGE; j++ )
    {
      for(int k=0; k < 100; k++ )
      {
        fPFAmpVectors[i][j][k] = 0; 
        fPFTofVectors[i][j][k] = 0;
        fPFAmpVectorsCoarse[i][j][k] = 0;
        fPFTofVectorsCoarse[i][j][k] = 0; 
      }
    }
  }
}

///
/// \return First (coarse) estimate of Amplitude using the Peak-Finder.
///
/// The output of the first iteration is sued to select vectors 
/// for the second iteration.
//_______________________________________________________________________
Double_t  
AliCaloRawAnalyzerPeakFinder::ScanCoarse(const Double_t *const array, const int length ) const
{
  Double_t tmpTof = 0;
  Double_t tmpAmp = 0;
  
  for(int i=0; i < length; i++)
  {
    tmpTof += fPFTofVectorsCoarse[0][length][i]*array[i]; 
    tmpAmp += fPFAmpVectorsCoarse[0][length][i]*array[i]; 
  }
  
  tmpTof = tmpTof / tmpAmp ;
  return tmpTof;
}

///
/// Evaluation of amplitude and TOF
///
/// Extracting the amplitude using the Peak-Finder algorithm
/// The amplitude is a weighted sum of the samples using 
/// optimum weights.
//_______________________________________________________________________
AliCaloFitResults 
AliCaloRawAnalyzerPeakFinder::Evaluate( const vector<AliCaloBunchInfo> &bunchvector, const UInt_t altrocfg1,  const UInt_t altrocfg2 )
{
  if( fIsInitialized == false )
  {
    cout << __FILE__ << ":" << __LINE__ << "ERROR, peakfinder vectors not loaded" << endl;
    return  AliCaloFitResults(kInvalid, kInvalid);
  }
    
  short maxampindex; //index of maximum amplitude
  short maxamp; //Maximum amplitude
  fAmp = 0;
  int index = SelectBunch( bunchvector,  &maxampindex,  &maxamp );
  
  if( index >= 0)
  {
    Float_t ped = ReverseAndSubtractPed( &(bunchvector.at(index))  ,  altrocfg1, altrocfg2, fReversed  );
    Float_t maxf = TMath::MaxElement(   bunchvector.at(index).GetLength(),  fReversed );
    short timebinOffset = maxampindex - (bunchvector.at( index ).GetLength()-1); 
    Float_t time = (timebinOffset*TIMEBINWITH)-fL1Phase;
    
    if(  maxf < fAmpCut  ||  maxamp > fOverflowCut  ) // (maxamp - ped) > fOverflowCut = Close to saturation (use low gain then)
    {
      return  AliCaloFitResults( maxamp, ped, Ret::kCrude, maxf, time, (int)time, 0, 0, Ret::kDummy);
    }            
    else if ( maxf >= fAmpCut )
    {
      int first = 0;
      int last = 0;
      short maxrev = maxampindex  -  bunchvector.at(index).GetStartBin();	  
      SelectSubarray( fReversed,  bunchvector.at(index).GetLength(), maxrev, &first, &last, fFitArrayCut);
      int nsamples =  last - first;
      
      if( ( nsamples  )  >= fNsampleCut ) // no if statement needed really; keep for readability
      {
        int startbin = bunchvector.at(index).GetStartBin();  
        int n = last - first;  
        int pfindex = n - fNsampleCut; 
        pfindex = pfindex > PF::SAMPLERANGE ? PF::SAMPLERANGE : pfindex;
        int dt =  maxampindex - startbin -2; 
        int tmpindex = 0;
        Float_t tmptof = ScanCoarse( &fReversed[dt] , n );
        
        if( tmptof < -1 )
        {
          tmpindex = 0;
        }
        else
          if( tmptof  > -1 && tmptof < 100 )
          {
            tmpindex = 1;
          }
          else
          {
            tmpindex = 2;
          }
        
        double tof = 0;
        for(int k=0; k < PF::SAMPLERANGE; k++   )
        {
          tof +=  fPFTofVectors[0][pfindex][k]*fReversed[ dt  +k + tmpindex -1 ];   
        }
        for( int i=0; i < PF::SAMPLERANGE; i++ )
        {
          {
            
            fAmp += fPFAmpVectors[0][pfindex][i]*fReversed[ dt  +i  +tmpindex -1 ];
          }
        }
        
        if( TMath::Abs(  (maxf - fAmp  )/maxf )  >   0.1 )
        {
          fAmp = maxf;
        }
        
        tof = timebinOffset - 0.01*tof/fAmp - fL1Phase/TIMEBINWITH; // clock
        Float_t chi2 = CalculateChi2(fAmp, tof-timebinOffset+maxrev, first, last);
        Int_t ndf = last - first - 1; // nsamples - 2
        Float_t toftime = (tof*TIMEBINWITH)-fL1Phase;
        
        return AliCaloFitResults( maxamp, ped , Ret::kFitPar, fAmp, toftime,
                                 (int)toftime, chi2, ndf,
                                 Ret::kDummy, AliCaloFitSubarray(index, maxrev, first, last) );  
      }
      else
      {
        Float_t chi2 = CalculateChi2(maxf, maxrev, first, last);
        Int_t ndf = last - first - 1; // nsamples - 2
        
        return AliCaloFitResults( maxamp, ped , Ret::kCrude, maxf, time,
                                 (int)time, chi2, ndf, Ret::kDummy, AliCaloFitSubarray(index, maxrev, first, last) );
      }
    } // ampcut
  }
  
  return  AliCaloFitResults(kInvalid, kInvalid);
}

///
/// Copy vectors
//_______________________________________________________________________
void   
AliCaloRawAnalyzerPeakFinder::CopyVectors( const AliCaloPeakFinderVectors *const pfv )
{
  if ( pfv != 0)
  {
    for(int i = 0;  i < PF::MAXSTART ; i++)
    {
      for( int j=0; j < PF::SAMPLERANGE; j++)  
      {
        pfv->GetVector( i, j, fPFAmpVectors[i][j] ,  fPFTofVectors[i][j],    
                       fPFAmpVectorsCoarse[i][j] , fPFTofVectorsCoarse[i][j]  ); 
        
        fPeakFinderVectors->SetVector( i, j, fPFAmpVectors[i][j], fPFTofVectors[i][j],    
                                      fPFAmpVectorsCoarse[i][j], fPFTofVectorsCoarse[i][j] );   
      }
    }
  }
  else
  {
    AliFatal( "pfv = ZERO !!!!!!!");
  } 
}

///
/// Loading of Peak-Finder  vectors from the 
/// Offline Condition Database  (OCDB)
//_______________________________________________________________________
void   
AliCaloRawAnalyzerPeakFinder::LoadVectorsOCDB()
{
  AliCDBEntry* entry = AliCDBManager::Instance()->Get("EMCAL/Calib/PeakFinder/");
  
  if( entry != 0 )
  {
    //cout << __FILE__ << ":" << __LINE__ << ": Printing metadata !! " << endl;
    //entry->PrintMetaData();
    AliCaloPeakFinderVectors  *pfv = (AliCaloPeakFinderVectors *)entry->GetObject(); 
    if( pfv == 0 )
    {
      cout << __FILE__ << ":" << __LINE__ << "_ ERRROR " << endl;
    }
    CopyVectors( pfv );
    
    if( pfv != 0 )
    {
      fIsInitialized = true;
    }
  }
}

///
/// Utility function to write Peak-Finder vectors to an root file
/// The output is used to create an OCDB entry.
//_______________________________________________________________________
void   
AliCaloRawAnalyzerPeakFinder::WriteRootFile() const
{ 
  fPeakFinderVectors->PrintVectors();
  
  TFile *f = new TFile("peakfindervectors2.root",  "recreate" );
  
  fPeakFinderVectors->Write();
  
  f->Close();
  
  delete f;
}

//
/// Utility function to write Peak-Finder vectors. 
//_______________________________________________________________________
void 
AliCaloRawAnalyzerPeakFinder::PrintVectors()
{ 
  for(int i=0; i < 20; i++)
  {
    for( int j = 0; j < PF::MAXSTART; j ++ )
    {
      for( int k=0; k < PF::SAMPLERANGE; k++ )
      {
        cout << fPFAmpVectors[j][k][i] << "\t" ;
      }
    }
    cout << endl;
  }
  cout << __FILE__ << ":" << __LINE__ << ":.... DONE !!" << endl;
}
