#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "TString.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TSpline.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TStyle.h"
#include "TAxis.h"
#include "TSystem.h"
#include "TClonesArray.h"

//input data is in text format, output data is in root format:
void MirrorTestAnalysis( const char* infilename, const char* outfilename ){

  //  gStyle->SetMarkerStyle(20);
  gStyle->SetPadLeftMargin(.22);
  gStyle->SetPadBottomMargin(.22);
  gStyle->SetPadRightMargin(.04);
  gStyle->SetPadTopMargin(.04);
  gStyle->SetTitleSize(.08,"XYZ");
  gStyle->SetLabelSize(.08,"XYZ");
  gStyle->SetNdivisions(505,"XYZ");
  gStyle->SetTitleOffset(1.4,"Y");
  gStyle->SetTitleOffset(1.2,"X");
  gStyle->SetTitleX(.7);
  gStyle->SetTitleY(.45);
  gStyle->SetTitleW(.35);
  gStyle->SetTitleH(.2);
  gStyle->SetPadTickX(1);
  gStyle->SetPadTickY(1);
  gStyle->SetPadGridX(true);
  gStyle->SetPadGridY(true);

  ifstream inputfile(infilename);
  TFile* fout = new TFile(outfilename,"RECREATE");

  TString currentline;

  //  TString testmirrorID = "TestMirror";
  //TString ctrlmirrorID = "10Z40AL.2";

  int nscans=0;
  int nmirrortests=0;
  int ncalibrations=0;
  vector<TString> MirrorIDs;
  vector<int> MirrorTestCalibIDs; //goes with MirrorIDs
  vector<vector<double> > MirrorTest_I_i, MirrorTest_I_r, MirrorTest_Ratio;
  vector<vector<double> > BScalib_I_R, BScalib_I_T, BScalib_Ratio;
  vector<int> BScalib_iscan; //Scan ID for BS calibration
  vector<int> MirrorTest_iscan; //Scan ID for Mirror test calibration
  vector<double> lambda;

  //In this version of the program, the data are formatted in columns as:
  // lambda I1A I1B I2A I2B ... where A is always the current measured by the photodiode viewing the incident light (reflected from beamsplitter)
  // and B is always the current measured by the second photodiode (transmitted by beamsplitter and/or reflected by mirror)
  //We need to establish the "dictionary" for interpreting the data columns. 
 

  //  int ncolumns=0;

  while( currentline.ReadLine(inputfile) && !currentline.BeginsWith("startdata") ){
    TObjArray* currentline_tokens = (TObjArray*) currentline.Tokenize(" ");

    int ntokens = currentline_tokens->GetEntries();
    
    if( ntokens > 0 && !currentline.BeginsWith("#") ){

      TString sfirst = ( (TObjString*) (*currentline_tokens)[0] )->GetString();
      
      if( sfirst == "nsamples" && ntokens >= 2 ){
	TString ssecond = ( ( TObjString*) (*currentline_tokens)[1] )->GetString();
	nmirrortests = ssecond.Atoi();
	
	MirrorIDs.resize( nmirrortests );
	MirrorTestCalibIDs.resize( nmirrortests );
	MirrorTest_I_i.resize( nmirrortests );
	MirrorTest_I_r.resize( nmirrortests );
	MirrorTest_Ratio.resize( nmirrortests );
	MirrorTest_iscan.resize( nmirrortests );

	cout << "Number of mirror tests = " << nmirrortests << endl;

      }
      
      if( sfirst == "ncalib" && ntokens >= 2 ){
	TString ssecond = ( ( TObjString*) (*currentline_tokens)[1] )->GetString();
	ncalibrations = ssecond.Atoi();

	BScalib_I_R.resize( ncalibrations );
	BScalib_I_T.resize( ncalibrations );
	BScalib_Ratio.resize( ncalibrations );
	BScalib_iscan.resize( ncalibrations );

	cout << "Number of calibrations = " << ncalibrations << endl;

      }

      if( sfirst == "TestSampleName" && nmirrortests > 0 && ntokens >= nmirrortests + 1 ){
	for( int i=1; i<=nmirrortests; i++){
	  TString sname = ( (TObjString*) (*currentline_tokens)[i] )->GetString();
	  MirrorIDs[i-1] = sname;

	  cout << "Sample " << i << " name=" << MirrorIDs[i-1].Data() << endl;
	}
      }

      if( sfirst == "TestSample_iscan" && nmirrortests > 0 && ntokens >= nmirrortests + 1 ){
	for( int i=1; i<=nmirrortests; i++){
	  TString sscan = ( (TObjString*) (*currentline_tokens)[i] )->GetString();
	  MirrorTest_iscan[i-1] = sscan.Atoi();

	  cout << "Test number " << i << " scan index = " << MirrorTest_iscan[i-1] << endl;
	}
      }
      
      if( sfirst == "TestSample_icalib" && nmirrortests > 0 && ntokens >= nmirrortests + 1 ){
	for( int i=1; i<=nmirrortests; i++){
	  TString scalib = ( (TObjString*) (*currentline_tokens)[i] )->GetString();
	  MirrorTestCalibIDs[i-1] = scalib.Atoi();
	  cout << "Test number " << i << " calibration index = " << MirrorTestCalibIDs[i-1] << endl;
	}
      }

      if( sfirst == "BScalib_iscan" && ncalibrations > 0 && ntokens >= ncalibrations + 1 ){
	for(int i=1; i<=ncalibrations; i++){
	  TString sscan = ( (TObjString*) (*currentline_tokens)[i] )->GetString();
	  BScalib_iscan[i-1] = sscan.Atoi();
	  cout << "Calibration number " << i << " scan index = " << BScalib_iscan[i-1] << endl;
	}
      }

    }
  }

  bool goodconfig = nmirrortests > 0 && ncalibrations > 0 && MirrorIDs.size() == nmirrortests && BScalib_iscan.size() == ncalibrations ;

  char delim = 9;

  nscans = nmirrortests + ncalibrations;

  while( goodconfig && currentline.ReadLine(inputfile) ){
    TObjArray* currentline_tokens = (TObjArray*) currentline.Tokenize(delim);
    
    int ntokens = currentline_tokens->GetEntries();
    
    if( !currentline.BeginsWith("#") && ntokens == 2 * nscans + 1 ){ //good data read:
      TString slambda = ( (TObjString*) (*currentline_tokens)[0] )->GetString();
      lambda.push_back( slambda.Atof() );
 
      for(int i=0; i<nmirrortests; i++){ //get all data for mirror tests:
	int iscan = MirrorTest_iscan[i];

	if( iscan >= 0 && iscan < nscans ){
	
	  double I_i, I_r;
	  TString sI_i, sI_r;
	  sI_i = ( (TObjString*) (*currentline_tokens)[2*iscan+1] )->GetString();
	  I_i = sI_i.Atof();
	  MirrorTest_I_i[i].push_back( I_i );
	  sI_r = ( (TObjString*) (*currentline_tokens)[2*iscan+2] )->GetString();
	  I_r = sI_r.Atof();
	  MirrorTest_I_r[i].push_back( I_r );
	  MirrorTest_Ratio[i].push_back( I_r/I_i );

	  cout << "Mirror test " << i+1 << " scan ID = " << iscan << " (lambda, I_i, I_r)=(" << lambda[lambda.size()-1] << ", " << I_i << ", " << I_r << ")" << endl;
	}
      }

      for(int i=0; i<ncalibrations; i++){
	int iscan = BScalib_iscan[i];
	
	if( iscan >= 0 && iscan < nscans ){
	  double I_R, I_T;
	  TString sI_R, sI_T;
	  sI_R = ( (TObjString*) (*currentline_tokens)[2*iscan+1] )->GetString();
	  sI_T = ( (TObjString*) (*currentline_tokens)[2*iscan+2] )->GetString();
	  
	  I_R = sI_R.Atof();
	  I_T = sI_T.Atof();

	  BScalib_I_R[i].push_back( I_R );
	  BScalib_I_T[i].push_back( I_T );
	  BScalib_Ratio[i].push_back( I_T/I_R );
	  
	  cout << "Beam splitter calibration " << i+1 << " scan ID = " << iscan << " (lambda, I_R, I_T)=(" << lambda[lambda.size()-1] << ", " << I_R << ", " << I_T << ")" << endl;
	}
      }
    }
  }
  
  TClonesArray *graphs = new TClonesArray("TGraph", nmirrortests );
  
  TCanvas* c1 = new TCanvas("c1","c1",600,600);
  //  TCanvas* c2 = new TCanvas("c2","c2",900,600);
  
  for(int i=0; i<nmirrortests; i++){
    vector<double> TestRatio = MirrorTest_Ratio[i];
    vector<double> CalibRatio = BScalib_Ratio[MirrorTestCalibIDs[i]];
    vector<double> Reflectivity;
    
    for(int j=0; j<TestRatio.size(); j++){
      Reflectivity.push_back( TestRatio[j]/CalibRatio[j] );
    }

    new( (*graphs)[i] ) TGraph( lambda.size(), &(lambda[0]), &(Reflectivity[0]) );

    MirrorIDs[i].ReplaceAll("-","_");

    ( (TGraph*) (*graphs)[i] )->SetName( MirrorIDs[i].Data() );
    ( (TGraph*) (*graphs)[i] )->SetTitle( MirrorIDs[i].Data() );
    ( (TGraph*) (*graphs)[i] )->SetMarkerStyle(20);
    
    c1->Clear();
    
    ( (TGraph*) (*graphs)[i] )->Draw("AP");
    ( (TGraph*) (*graphs)[i] )->GetYaxis()->SetRangeUser( 0.0, 1.0 );
    ( (TGraph*) (*graphs)[i] )->GetYaxis()->SetTitle("Reflectivity");
    ( (TGraph*) (*graphs)[i] )->GetXaxis()->SetTitle("Wavelength (nm)");
    
    c1->Update();
    cout << "CR = next:";
    TString reply;
    reply.ReadLine(cin,kFALSE);
  }
    
 
  graphs->Write();
  fout->Close();
}
