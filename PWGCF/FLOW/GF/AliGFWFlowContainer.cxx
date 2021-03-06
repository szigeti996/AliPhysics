#include "AliGFWFlowContainer.h"

AliGFWFlowContainer::AliGFWFlowContainer():
  TNamed("",""),
  fProf(0),
  fProfRand(0),
  fNRandom(0),
  fIDName("MidV")
{
};
AliGFWFlowContainer::AliGFWFlowContainer(const char *name):
  TNamed(name,name),
  fProf(0),
  fProfRand(0),
  fNRandom(0),
  fIDName("MidV")
{
};
AliGFWFlowContainer::~AliGFWFlowContainer() {
  delete fProf;
  delete fProfRand;
};
void AliGFWFlowContainer::Initialize(TObjArray *inputList, Int_t nMultiBins, Double_t *multiBins, Int_t nRandom) {
  if(!inputList) {
    printf("Input list not specified\n");
    return;
  };
  if(inputList->GetEntries()<1) {
    printf("Input list empty!\n");
    return;
  };
  fProf = new TProfile2D(Form("%s_CorrProfile",this->GetName()),"CorrProfile",nMultiBins, multiBins,inputList->GetEntries(),0.5,inputList->GetEntries()+0.5);
  for(Int_t i=0;i<inputList->GetEntries();i++)
    fProf->GetYaxis()->SetBinLabel(i+1,inputList->At(i)->GetName());
  if(nRandom) {
    fNRandom=nRandom;
    fProfRand = new TObjArray();
    fProfRand->SetOwner(kTRUE);
    for(Int_t i=0;i<nRandom;i++)
      fProfRand->Add((TProfile2D*)fProf->Clone(Form("%s_Rand_%i",fProf->GetName(),i)));
  };
};
void AliGFWFlowContainer::Initialize(TObjArray *inputList, Int_t nMultiBins, Double_t MultiMin, Double_t MultiMax, Int_t nRandom) {
  if(!inputList) {
    printf("Input list not specified\n");
    return;
  };
  if(inputList->GetEntries()<1) {
    printf("Input list empty!\n");
    return;
  };
  fProf = new TProfile2D(Form("%s_CorrProfile",this->GetName()),"CorrProfile",nMultiBins, MultiMin,MultiMax,inputList->GetEntries(),0.5,inputList->GetEntries()+0.5);
  fProf->SetDirectory(0);
  for(Int_t i=0;i<inputList->GetEntries();i++)
    fProf->GetYaxis()->SetBinLabel(i+1,inputList->At(i)->GetName());
  if(nRandom) {
    fNRandom=nRandom;
    fProfRand = new TObjArray();
    fProfRand->SetOwner(kTRUE);
    for(Int_t i=0;i<nRandom;i++)
      fProfRand->Add((TProfile2D*)fProf->Clone(Form("%s_Rand_%i",fProf->GetName(),i)));
  };

};

Int_t AliGFWFlowContainer::FillProfile(const char *hname, Double_t multi, Double_t corr, Double_t w, Double_t rn) {
  if(!fProf) return -1;
  Int_t yin = fProf->GetYaxis()->FindBin(hname);
  if(!yin) {
    printf("Could not find bin %s\n",hname);
    return -1;
  };
  fProf->Fill(multi,yin,corr,w);
  if(fNRandom) {
    Double_t rnind = rn*fNRandom;
    ((TProfile2D*)fProfRand->At((Int_t)rnind))->Fill(multi,yin,corr,w);
  };
  return 0;
};
Long64_t AliGFWFlowContainer::Merge(TCollection *collist) {
  Long64_t nmerged=0;
  AliGFWFlowContainer *l_FC = 0;
  TIter all_FC(collist);
  //TProfile2D *spro = lfc->GetProfile();
  while (l_FC = ((AliGFWFlowContainer*) all_FC())) {
    TProfile2D *tpro = GetProfile();
    TProfile2D *spro = l_FC->GetProfile();
    if(!tpro) {
      fProf = (TProfile2D*)spro->Clone(spro->GetName());
      fProf->SetDirectory(0);
    } else 
      tpro->Add(spro);
    nmerged++;
    TObjArray *tarr = l_FC->GetSubProfiles();
    if(!tarr)
      continue;
    if(!fProfRand) {
      fProfRand = new TObjArray();
      fProfRand->SetOwner(kTRUE);
    };
    for(Int_t i=0;i<tarr->GetEntries();i++) {
      if(!(fProfRand->FindObject(tarr->At(i)->GetName()))) {
	fProfRand->Add((TProfile2D*)tarr->At(i)->Clone(tarr->At(i)->GetName()));
	((TProfile2D*)fProfRand->At(fProfRand->GetEntries()-1))->SetDirectory(0);
      } else {
	((TProfile2D*)fProfRand->FindObject(tarr->At(i)->GetName()))->Add((TProfile2D*)tarr->At(i));
      };
    };
  };
  return nmerged;
};
void AliGFWFlowContainer::ReadAndMerge(const char *filelist) {
  FILE *flist = fopen(filelist,"r");
  char str[150];
  Int_t nFiles=0;
  while(fscanf(flist,"%s\n",str)==1) nFiles++;
  rewind(flist);
  if(nFiles==0) {
    printf("No files to read!\n");
    return;
  };
  for(Int_t i=0;i<nFiles;i++) {
    Int_t trash = fscanf(flist,"%s\n",str);
    TFile *tf = new TFile(str,"READ");
    if(tf->IsZombie()) {
      printf("Could not open file %s!\n",str);
      tf->Close();
      continue;
    };
    PickAndMerge(tf);
    tf->Close();
  };
};
void AliGFWFlowContainer::PickAndMerge(TFile *tfi) {
  AliGFWFlowContainer *lfc = (AliGFWFlowContainer*)tfi->Get(this->GetName());
  if(!lfc) {
    printf("Could not pick up the %s from %s\n",this->GetName(),tfi->GetName());
    return;
  };
  TProfile2D *spro = lfc->GetProfile();
  TProfile2D *tpro = GetProfile();
  if(!tpro) {
    fProf = (TProfile2D*)spro->Clone(spro->GetName());
    fProf->SetDirectory(0);
  } else 
    tpro->Add(spro);
  TObjArray *tarr = lfc->GetSubProfiles();
  if(!tarr) {
    //printf("Target %s does not have subprofiles!\n",lfc->GetName());
    return;
  };
  if(!fProfRand) {
    fProfRand = new TObjArray();
    fProfRand->SetOwner(kTRUE);
  };
  for(Int_t i=0;i<tarr->GetEntries();i++) {
    if(!(fProfRand->FindObject(tarr->At(i)->GetName()))) {
      //printf("Adding %s (%i)\n",tarr->At(i)->GetName(),i);
      fProfRand->Add((TProfile2D*)tarr->At(i)->Clone(tarr->At(i)->GetName()));
      ((TProfile2D*)fProfRand->At(fProfRand->GetEntries()-1))->SetDirectory(0);
    } else {
      ((TProfile2D*)fProfRand->FindObject(tarr->At(i)->GetName()))->Add((TProfile2D*)tarr->At(i));
    };
  };
  //printf("After merge: %i in target, %i in source\n",fProfRand->GetEntries(),tarr->GetEntries());
};
Bool_t AliGFWFlowContainer::OverrideMainWithSub(Int_t ind, Bool_t ExcludeChosen) {
  if(!fProfRand) {
    printf("Cannot override main profile with a randomized one. Random profile array does not exist.\n");
    return kFALSE;
  };
  if(!ExcludeChosen) {
    TProfile2D *tarprof = (TProfile2D*)fProfRand->At(ind);
    if(!tarprof) {
      printf("Target random histogram does not exist.\n");
      return kFALSE;
    };
    TString ts(fProf->GetName());
    delete fProf;
    fProf = (TProfile2D*)tarprof->Clone(ts.Data());
    return kTRUE;
  } else {
    TString ts(fProf->GetName());
    delete fProf;
    fProf=0;
    for(Int_t i=0;i<fProfRand->GetEntries();i++) {
      if(i==ind) continue;
      TProfile2D *tarprof = (TProfile2D*)fProfRand->At(ind);
      if(!fProf)
	fProf = (TProfile2D*)tarprof->Clone(ts.Data());
      else
	fProf->Add(tarprof);
    };
    return kTRUE;
  };
};
TProfile *AliGFWFlowContainer::GetCorrXXVsMulti(const char *order, Int_t l_pti) {
  const char *ptpf = l_pti>0?Form("_pt_%i",l_pti):"";
  const char *ybinlab = Form("%s%s%s",fIDName.Data(),order,ptpf);
  Int_t ybinno = fProf->GetYaxis()->FindBin(ybinlab);
  if(ybinno<0) {
    printf("Could not find %s!\n",ybinlab);
    return 0;
  };
  TProfile *rethist = (TProfile*)fProf->ProfileX(Form("corr_%s",order),ybinno,ybinno);
  rethist->SetTitle(Form(";multi.;#LT#LT%s#GT#GT",order));
  return rethist;
};
TProfile *AliGFWFlowContainer::GetCorrXXVsPt(const char *order, Double_t lminmulti, Double_t lmaxmulti) {
  Int_t minm = 1;
  Int_t maxm = fProf->GetXaxis()->GetNbins();
  //This is hardcoded for now. There must be a better (convenient) way to set this
  Int_t fNbinsPt=24;
  Double_t fbinsPt[] = {
    0.2, 0.4, 0.6, 0.8, 1.0,
    1.2, 1.4, 1.6, 1.8, 2.0,
    2.2, 2.4, 2.6, 3.0, 3.4,
    3.8, 4.2, 4.6, 5.2, 5.8,
    6.6, 8.0, 12.0, 16.0, 20.};

  if(lminmulti>0) {
    minm=fProf->GetXaxis()->FindBin(lminmulti);
    maxm=minm;
  };
  if(lmaxmulti>lminmulti) maxm=fProf->GetXaxis()->FindBin(lmaxmulti);
  TProfile *projection = (TProfile*)fProf->ProfileY("tempproj",minm,maxm);
  TProfile *rethist = new TProfile(Form("%s_MultiB_%i_%i",order,minm,maxm),Form("%s_MultiB_%i_%i",order,minm,maxm), fNbinsPt, fbinsPt);
  const char *ybl1 = Form("%s%s_pt_1",fIDName.Data(),order);
  const char *ybl2 = Form("%s%s_pt_%i",fIDName.Data(),order,fNbinsPt);
  Int_t ybn1 = fProf->GetYaxis()->FindBin(ybl1);
  Int_t ybn2 = fProf->GetYaxis()->FindBin(ybl2);
  AliProfileSubset *rhSubset = new AliProfileSubset(rethist);
  delete rethist;
  rhSubset->CopyFromProfile(ybn1,ybn2,projection);
  delete projection;
  return rhSubset;
};
TH1D *AliGFWFlowContainer::ProfToHist(TProfile *inpf) {
  Int_t nbins = inpf->GetNbinsX();
  Double_t *xbs = new Double_t[nbins+1];
  inpf->GetLowEdge(xbs);
  xbs[nbins] = xbs[nbins-1]+inpf->GetBinWidth(nbins);
  TH1D *rethist = new TH1D(Form("%s_hist",inpf->GetName()),inpf->GetTitle(),nbins,xbs);
  for(Int_t i=1;i<=rethist->GetNbinsX();i++) {
    if(inpf->GetBinContent(i)!=0) {
      rethist->SetBinContent(i,inpf->GetBinContent(i));
      rethist->SetBinError(i,inpf->GetBinError(i));
    };
  };
  return rethist;
};
TH1D *AliGFWFlowContainer::GetHistCorrXXVsMulti(const char *order, Int_t l_pti) {
  TProfile *tpf = GetCorrXXVsMulti(order,l_pti);
  TH1D *rethist = ProfToHist(tpf);
  delete tpf;
  return rethist;
};
TH1D *AliGFWFlowContainer::GetHistCorrXXVsPt(const char *order,  Double_t lminmulti, Double_t lmaxmulti) {
  TProfile *tpf = GetCorrXXVsPt(order,lminmulti,lmaxmulti);
  TH1D *rethist = ProfToHist(tpf);
  TProfile *refflow = GetRefFlowProfile(order,lminmulti,lmaxmulti);
  refflow->RebinX(refflow->GetNbinsX());
  rethist->SetBinContent(0,refflow->GetBinContent(1));
  rethist->SetBinError(0,refflow->GetBinError(1));
  delete refflow;
  delete tpf;
  return rethist;
};
TH1D *AliGFWFlowContainer::GetVN2(TH1D* cn2) {
  TH1D *rethist = (TH1D*)cn2->Clone(Form("vn2_%s",cn2->GetName()));
  rethist->Reset();
  Double_t rf2 = cn2->GetBinContent(0);
  Double_t rf2e= cn2->GetBinError(0);
  Bool_t OnPt = (!rf2==0);
  for(Int_t i=1;i<=rethist->GetNbinsX();i++) {
    Double_t d2 = cn2->GetBinContent(i);
    Double_t d2e = cn2->GetBinError(i);
    if(d2>0) {
      rethist->SetBinContent(i,OnPt?VDN2Value(d2,rf2):VN2Value(d2));
      rethist->SetBinError(i,OnPt?VDN2Error(d2,d2e,rf2,rf2e):VN2Error(d2,d2e));
    };
  };
  return rethist;
};
TH1D *AliGFWFlowContainer::GetCN2VsX(Int_t n, Bool_t onPt, Double_t arg1, Double_t arg2) {
  TH1D *corrN2;
  if(onPt)
    corrN2 = GetHistCorrXXVsPt(Form("%i2",n),arg1,arg2);
  else
    corrN2 = GetHistCorrXXVsMulti(Form("%i2",n),(Int_t)arg1);
  corrN2->SetName(Form("Corr_%s",corrN2->GetName()));
  TH1D *rethist = GetCN2(corrN2);
  TString *nam = new TString(corrN2->GetName());
  delete corrN2;
  rethist->SetName(nam->Data());
  if(onPt) {
    Int_t bins = fProf->GetXaxis()->FindBin(arg1);
    Int_t bins2 = fProf->GetXaxis()->FindBin(arg2);
    Double_t bv1 = fProf->GetXaxis()->GetBinLowEdge(bins);
    Double_t bv2 = fProf->GetXaxis()->GetBinUpEdge(bins2);
    rethist->SetTitle(Form("%2.0f - %4.0f;#it{p}_{T} (GeV/#it{c}); c_{%i}{2}",bv1,bv2,n));
  } else {
    rethist->SetTitle(Form(";#it{N}_{tr};c_{%i}{2}",n));
  };
  return rethist;
};

TH1D *AliGFWFlowContainer::GetVN2VsX(Int_t n, Bool_t onPt, Double_t arg1, Double_t arg2) {
  TH1D *corrh = GetCN2VsX(n,onPt,arg1,arg2);
  TString *nam = new TString(corrh->GetName());
  TH1D *rethist = GetVN2(corrh);
  delete corrh;
  rethist->SetName(nam->Data());
  if(onPt) {
    Int_t bins = fProf->GetXaxis()->FindBin(arg1);
    Int_t bins2 = fProf->GetXaxis()->FindBin(arg2);
    Double_t bv1 = fProf->GetXaxis()->GetBinCenter(bins);
    Double_t bv2 = fProf->GetXaxis()->GetBinCenter(bins2);
    rethist->SetTitle(Form("%2.0f - %4.0f;#it{p}_{T} (GeV/#it{c}); v_{%i}{2}",bv1,bv2,n));
  } else {
    rethist->SetTitle(Form(";#it{N}_{tr};v_{%i}{2}",n));
  };
  delete nam;
  return rethist;
};

TH1D *AliGFWFlowContainer::GetCN2(TH1D *corrN2) {
  Double_t rf2 = corrN2->GetBinContent(0);
  Double_t rf2e= corrN2->GetBinError(0);
  Bool_t OnPt=(rf2!=0); // This is not needed, as C2 = <2> anyway
  TH1D *rethist = (TH1D*)corrN2->Clone(Form("cN2_%s",corrN2->GetName()));
  rethist->Reset();
  for(Int_t i=1;i<=rethist->GetNbinsX();i++) {
    Double_t cor2v = corrN2->GetBinContent(i);
    Double_t cor2e = corrN2->GetBinError(i);
    rethist->SetBinContent(i,cor2v);
    rethist->SetBinError(i,cor2e);
  };
  if(OnPt) {
    rethist->SetBinContent(0,rf2);
    rethist->SetBinError(0,rf2e);
  } else {
    rethist->SetBinContent(0,0);
    rethist->SetBinError(0,0);
  };
  return rethist;
};


TH1D *AliGFWFlowContainer::GetCN4(TH1D *corrN4, TH1D *corrN2) {
  Double_t rf2 = corrN2->GetBinContent(0);
  Double_t rf2e= corrN2->GetBinError(0);
  Double_t rf4 = corrN4->GetBinContent(0);
  Double_t rf4e= corrN4->GetBinError(0);
  Bool_t OnPt=(rf2!=0);
  TH1D *rethist = (TH1D*)corrN4->Clone(Form("cN4_%s",corrN4->GetName()));
  rethist->Reset();
  for(Int_t i=1;i<=rethist->GetNbinsX();i++) {
    Double_t cor4v = corrN4->GetBinContent(i);
    Double_t cor4e = corrN4->GetBinError(i);
    Double_t cor2v = corrN2->GetBinContent(i);
    Double_t cor2e = corrN2->GetBinError(i);
    rethist->SetBinContent(i,OnPt?DN4Value(cor4v,cor2v,rf2):CN4Value(cor4v,cor2v));
    rethist->SetBinError(i,OnPt?DN4Error(cor4e,cor2v,cor2e,rf2,rf2e):CN4Error(cor4e,cor2v,cor2e));
  };
  if(OnPt) {
    rethist->SetBinContent(0,CN4Value(rf4,rf2));
    rethist->SetBinError(0,CN4Error(rf4e,rf2,rf2e));
  } else {
    rethist->SetBinContent(0,0);
    rethist->SetBinError(0,0);
  };
  return rethist;
};

TH1D *AliGFWFlowContainer::GetCN6(TH1D *corrN6, TH1D *corrN4, TH1D *corrN2) {
  TH1D *tn2 = (TH1D*)corrN2->Clone(Form("tn2_%s",corrN2->GetName()));
  TH1D *tn4 = (TH1D*)corrN4->Clone(Form("tn4_%s",corrN4->GetName()));
  TH1D *tn6 = (TH1D*)corrN6->Clone(Form("tn6_%s",corrN6->GetName()));

  Double_t rf2 = corrN2->GetBinContent(0);
  Double_t rf2e= corrN2->GetBinError(0);
  Double_t rf4 = corrN4->GetBinContent(0);
  Double_t rf4e= corrN4->GetBinError(0);
  Double_t rf6 = corrN6->GetBinContent(0);
  Double_t rf6e= corrN6->GetBinError(0);
  Bool_t OnPt=(rf2!=0);
  TH1D *rethist = (TH1D*)corrN6->Clone(Form("cN6_%s",corrN6->GetName()));
  rethist->Reset();
  for(Int_t i=1;i<=rethist->GetNbinsX();i++) {
    Double_t cor6v = corrN6->GetBinContent(i);
    Double_t cor6e = corrN6->GetBinError(i);
    Double_t cor4v = corrN4->GetBinContent(i);
    Double_t cor4e = corrN4->GetBinError(i);
    Double_t cor2v = corrN2->GetBinContent(i);
    Double_t cor2e = corrN2->GetBinError(i);
    rethist->SetBinContent(i,OnPt?DN6Value(cor6v,cor4v,cor2v,rf4,rf2):CN6Value(cor6v,cor4v,cor2v));
    rethist->SetBinError(i,OnPt?DN6Error(cor6e,cor4v,cor4e,cor2v,cor2e,rf4,rf4e,rf2,rf2e):CN6Error(cor6e, cor4v, cor4e, cor2v, cor2e));
  };
  if(OnPt) {
    rethist->SetBinContent(0,CN6Value(rf6,rf4,rf2));
    rethist->SetBinError(0,CN6Error(rf6e,rf4,rf4e,rf2,rf2e));
  } else {
    rethist->SetBinContent(0,0);
    rethist->SetBinError(0,0);
  };
  delete tn2;
  delete tn4;
  delete tn6;
  return rethist;
};
TH1D *AliGFWFlowContainer::GetCN8(TH1D *corrN8, TH1D *corrN6, TH1D *corrN4, TH1D *corrN2) {
  TH1D *tn2 = (TH1D*)corrN2->Clone(Form("tn2_%s",corrN2->GetName()));
  TH1D *tn4 = (TH1D*)corrN4->Clone(Form("tn4_%s",corrN4->GetName()));
  TH1D *tn6 = (TH1D*)corrN6->Clone(Form("tn6_%s",corrN6->GetName()));
  TH1D *tn8 = (TH1D*)corrN8->Clone(Form("tn8_%s",corrN6->GetName()));

  Double_t rf2 = corrN2->GetBinContent(0);
  Double_t rf2e= corrN2->GetBinError(0);
  Double_t rf4 = corrN4->GetBinContent(0);
  Double_t rf4e= corrN4->GetBinError(0);
  Double_t rf6 = corrN6->GetBinContent(0);
  Double_t rf6e= corrN6->GetBinError(0);
  Double_t rf8 = corrN8->GetBinContent(0);
  Double_t rf8e= corrN8->GetBinError(0);
  Bool_t OnPt=(rf2!=0);
  TH1D *rethist = (TH1D*)corrN8->Clone(Form("cN8_%s",corrN8->GetName()));
  rethist->Reset();
  for(Int_t i=1;i<=rethist->GetNbinsX();i++) {
    Double_t cor8v = corrN8->GetBinContent(i);
    Double_t cor8e = corrN8->GetBinError(i);
    Double_t cor6v = corrN6->GetBinContent(i);
    Double_t cor6e = corrN6->GetBinError(i);
    Double_t cor4v = corrN4->GetBinContent(i);
    Double_t cor4e = corrN4->GetBinError(i);
    Double_t cor2v = corrN2->GetBinContent(i);
    Double_t cor2e = corrN2->GetBinError(i);
    rethist->SetBinContent(i,OnPt?DN8Value(cor8v,cor6v,cor4v,cor2v,rf6,rf4,rf2):CN8Value(cor8v,cor6v,cor4v,cor2v));
    rethist->SetBinError(i,OnPt?DN8Error(cor8e,cor6v,cor6e,cor4v,cor4e,cor2v,cor2e,rf6,rf6e,rf4,rf4e,rf2,rf2e):CN8Error(cor8e,cor6v,cor6e, cor4v, cor4e, cor2v, cor2e));
  };
  if(OnPt) {
    rethist->SetBinContent(0,CN8Value(rf8,rf6,rf4,rf2));
    rethist->SetBinError(0,CN8Error(rf8e,rf6,rf6e,rf4,rf4e,rf2,rf2e));
  } else {
    rethist->SetBinContent(0,0);
    rethist->SetBinError(0,0);
  };
  delete tn2;
  delete tn4;
  delete tn6;
  delete tn8;
  return rethist;
};

TH1D *AliGFWFlowContainer::GetCN4VsX(Int_t n, Bool_t onPt, Double_t arg1, Double_t arg2) {
  TH1D *corrN2, *corrN4;
  if(onPt) {
    corrN2 = GetHistCorrXXVsPt(Form("%i2",n),arg1,arg2);
    corrN2->SetName(Form("Corr_%s",corrN2->GetName()));
    corrN4 = GetHistCorrXXVsPt(Form("%i4",n),arg1,arg2);
    corrN4->SetName(Form("Corr_%s",corrN4->GetName()));
  } else {
    corrN2 = GetHistCorrXXVsMulti(Form("%i2",n),(Int_t)arg1);
    corrN2->SetName(Form("Corr_%s",corrN2->GetName()));
    corrN4 = GetHistCorrXXVsMulti(Form("%i4",n),(Int_t)arg1);
    corrN4->SetName(Form("Corr_%s",corrN4->GetName()));
  };
  TH1D *rethist = GetCN4(corrN4,corrN2);
  TString *nam = new TString(corrN4->GetName());
  delete corrN2;
  delete corrN4;
  rethist->SetName(nam->Data());
  if(onPt) {
    Int_t bins = fProf->GetXaxis()->FindBin(arg1);
    Int_t bins2 = fProf->GetXaxis()->FindBin(arg2);
    Double_t bv1 = fProf->GetXaxis()->GetBinLowEdge(bins);
    Double_t bv2 = fProf->GetXaxis()->GetBinUpEdge(bins2);
    rethist->SetTitle(Form("%2.0f - %4.0f;#it{p}_{T} (GeV/#it{c}); c_{%i}{4}",bv1,bv2,n));
  } else {
    rethist->SetTitle(Form(";#it{N}_{tr};c_{%i}{4}",n));
  };
  return rethist;
};
TH1D *AliGFWFlowContainer::GetVN4(TH1D *inh) {
  TH1D *rethist = (TH1D*)inh->Clone(Form("v24_%s",inh->GetName()));
  Double_t c4=inh->GetBinContent(0);
  Double_t c4e=inh->GetBinError(0);
  Bool_t OnPt = (!c4==0);
  rethist->Reset();
  for(Int_t i=1;i<=rethist->GetNbinsX();i++) {
    Double_t d4=inh->GetBinContent(i);
    Double_t d4e=inh->GetBinError(i);
    if(OnPt&&c4>=0) continue;
    if(d4>=0) continue;
    rethist->SetBinContent(i,OnPt?VDN4Value(d4,c4):VN4Value(d4));
    rethist->SetBinError(i,OnPt?VDN4Error(d4,d4e,c4,c4e):VN4Error(d4,d4e));
  };
  return rethist;
};
TH1D *AliGFWFlowContainer::GetVN6(TH1D *inh) {
  TH1D *rethist = (TH1D*)inh->Clone(Form("v26_%s",inh->GetName()));
  Double_t c6=inh->GetBinContent(0);
  Double_t c6e=inh->GetBinError(0);
  Bool_t OnPt = (!c6==0);
  rethist->Reset();
  for(Int_t i=1;i<=rethist->GetNbinsX();i++) {
    Double_t d6=inh->GetBinContent(i);
    Double_t d6e=inh->GetBinError(i);
    if(OnPt && c6<=0) continue;
    if(d6<=0) continue;
    rethist->SetBinContent(i,OnPt?VDN6Value(d6,c6):VN6Value(d6));
    rethist->SetBinError(i,OnPt?VDN6Error(d6,d6e,c6,c6e):VN6Error(d6,d6e));
  };
  return rethist;
};
TH1D *AliGFWFlowContainer::GetVN8(TH1D *inh) {
  TH1D *rethist = (TH1D*)inh->Clone(Form("v28_%s",inh->GetName()));
  Double_t c8=inh->GetBinContent(0);
  Double_t c8e=inh->GetBinError(0);
  Bool_t OnPt = (!c8==0);
  rethist->Reset();
  for(Int_t i=1;i<=rethist->GetNbinsX();i++) {
    Double_t d8=inh->GetBinContent(i);
    Double_t d8e=inh->GetBinError(i);
    if(OnPt && c8>0) continue;
    if(d8>0) continue;
    rethist->SetBinContent(i,OnPt?VDN8Value(d8,c8):VN8Value(d8));
    rethist->SetBinError(i,OnPt?VDN8Error(d8,d8e,c8,c8e):VN8Error(d8,d8e));
  };
  return rethist;
};

TH1D *AliGFWFlowContainer::GetVN4VsX(Int_t n, Bool_t onPt, Double_t arg1, Double_t arg2) {
  TH1D *temph = GetCN4VsX(n,onPt,arg1,arg2);
  TH1D *rethist = GetVN4(temph);
  TString *nam = new TString(temph->GetName());
  delete temph;
  rethist->SetName(nam->Data());
  if(onPt) {
    Int_t bins = fProf->GetXaxis()->FindBin(arg1);
    Int_t bins2 = fProf->GetXaxis()->FindBin(arg2);
    Double_t bv1 = fProf->GetXaxis()->GetBinLowEdge(bins);
    Double_t bv2 = fProf->GetXaxis()->GetBinUpEdge(bins2);
    rethist->SetTitle(Form("%2.0f - %4.0f;#it{p}_{T} (GeV/#it{c}); v_{%i}{4}",bv1,bv2,n));
  } else {
    rethist->SetTitle(Form(";#it{N}_{tr};v_{%i}{4}",n));
  };
  return rethist;
};
TH1D *AliGFWFlowContainer::GetCN6VsX(Int_t n, Bool_t onPt, Double_t arg1, Double_t arg2) {
  TH1D *corrN2, *corrN4, *corrN6;
  if(onPt) {
    corrN2 = GetHistCorrXXVsPt(Form("%i2",n),arg1,arg2);
    corrN2->SetName(Form("Corr_%s",corrN2->GetName()));
    corrN4 = GetHistCorrXXVsPt(Form("%i4",n),arg1,arg2);
    corrN4->SetName(Form("Corr_%s",corrN4->GetName()));
    corrN6 = GetHistCorrXXVsPt(Form("%i6",n),arg1,arg2);
    corrN6->SetName(Form("Corr_%s",corrN6->GetName()));
  } else {
    corrN2 = GetHistCorrXXVsMulti(Form("%i2",n),(Int_t)arg1);
    corrN4 = GetHistCorrXXVsMulti(Form("%i4",n),(Int_t)arg1);
    corrN6 = GetHistCorrXXVsMulti(Form("%i6",n),(Int_t)arg1);
  };
  TH1D *rethist = GetCN6(corrN6,corrN4,corrN2);
  delete corrN2;
  delete corrN4;
  TString *nam = new TString(corrN6->GetName());
  delete corrN6;
  rethist->SetName(nam->Data());
  if(onPt) {
    Int_t bins = fProf->GetXaxis()->FindBin(arg1);
    Int_t bins2 = fProf->GetXaxis()->FindBin(arg2);
    Double_t bv1 = fProf->GetXaxis()->GetBinLowEdge(bins);
    Double_t bv2 = fProf->GetXaxis()->GetBinUpEdge(bins2);
    rethist->SetTitle(Form("%2.0f - %4.0f;#it{p}_{T} (GeV/#it{c}); c_{%i}{6}",bv1,bv2,n));
  } else {
    rethist->SetTitle(Form(";#it{N}_{tr};c_{%i}{6}",n));
  };
  return rethist;
};
TH1D *AliGFWFlowContainer::GetVN6VsX(Int_t n, Bool_t onPt, Double_t arg1, Double_t arg2) {
  TH1D *temph = GetCN6VsX(n,onPt,arg1,arg2);
  TH1D *rethist = GetVN6(temph);
  TString *nam = new TString(temph->GetName());
  delete temph;
  rethist->SetName(nam->Data());
  if(onPt) {
    Int_t bins = fProf->GetXaxis()->FindBin(arg1);
    Int_t bins2 = fProf->GetXaxis()->FindBin(arg2);
    Double_t bv1 = fProf->GetXaxis()->GetBinLowEdge(bins);
    Double_t bv2 = fProf->GetXaxis()->GetBinUpEdge(bins2);
    rethist->SetTitle(Form("%2.0f - %4.0f;#it{p}_{T} (GeV/#it{c}); v_{%i}{6}",bv1,bv2,n));
  } else {
    rethist->SetTitle(Form(";#it{N}_{tr};v_{%i}{6}",n));
  };
  return rethist;
};
TH1D *AliGFWFlowContainer::GetCN8VsX(Int_t n, Bool_t onPt, Double_t arg1, Double_t arg2) {
  TH1D *corrN2, *corrN4, *corrN6, *corrN8;
  if(onPt) {
    corrN2 = GetHistCorrXXVsPt(Form("%i2",n),arg1,arg2);
    corrN2->SetName(Form("Corr_%s",corrN2->GetName()));
    corrN4 = GetHistCorrXXVsPt(Form("%i4",n),arg1,arg2);
    corrN4->SetName(Form("Corr_%s",corrN4->GetName()));
    corrN6 = GetHistCorrXXVsPt(Form("%i6",n),arg1,arg2);
    corrN6->SetName(Form("Corr_%s",corrN6->GetName()));
    corrN8 = GetHistCorrXXVsPt(Form("%i8",n),arg1,arg2);
    corrN8->SetName(Form("Corr_%s",corrN8->GetName()));
  } else {
    corrN2 = GetHistCorrXXVsMulti(Form("%i2",n),(Int_t)arg1);
    corrN4 = GetHistCorrXXVsMulti(Form("%i4",n),(Int_t)arg1);
    corrN6 = GetHistCorrXXVsMulti(Form("%i6",n),(Int_t)arg1);
    corrN8 = GetHistCorrXXVsMulti(Form("%i8",n),(Int_t)arg1);
  };
  TH1D *rethist = GetCN8(corrN8,corrN6,corrN4,corrN2);
  delete corrN2;
  delete corrN4;
  delete corrN6;
  TString *nam = new TString(corrN8->GetName());
  delete corrN8;
  rethist->SetName(nam->Data());
  if(onPt) {
    Int_t bins = fProf->GetXaxis()->FindBin(arg1);
    Int_t bins2 = fProf->GetXaxis()->FindBin(arg2);
    Double_t bv1 = fProf->GetXaxis()->GetBinLowEdge(bins);
    Double_t bv2 = fProf->GetXaxis()->GetBinUpEdge(bins2);
    rethist->SetTitle(Form("%2.0f - %4.0f;#it{p}_{T} (GeV/#it{c}); c_{%i}{8}",bv1,bv2,n));
  } else {
    rethist->SetTitle(Form(";#it{N}_{tr};c_{%i}{8}",n));
  };
  return rethist;
};
TH1D *AliGFWFlowContainer::GetVN8VsX(Int_t n, Bool_t onPt, Double_t arg1, Double_t arg2) {
  TH1D *temph = GetCN8VsX(n,onPt,arg1,arg2);
  TH1D *rethist = GetVN8(temph);
  TString *nam = new TString(temph->GetName());
  delete temph;
  rethist->SetName(nam->Data());
  if(onPt) {
    Int_t bins = fProf->GetXaxis()->FindBin(arg1);
    Int_t bins2 = fProf->GetXaxis()->FindBin(arg2);
    Double_t bv1 = fProf->GetXaxis()->GetBinLowEdge(bins);
    Double_t bv2 = fProf->GetXaxis()->GetBinUpEdge(bins2);
    rethist->SetTitle(Form("%2.0f - %4.0f;#it{p}_{T} (GeV/#it{c}); v_{%i}{8}",bv1,bv2,n));
  } else {
    rethist->SetTitle(Form(";#it{N}_{tr};v_{%i}{8}",n));
  };
  return rethist;
};
TH1D *AliGFWFlowContainer::GetCNN(Int_t n, Int_t c, Bool_t onPt, Double_t arg1, Double_t arg2) {
  if(c==8) return GetCN8VsX(n,onPt,arg1,arg2);
  if(c==6) return GetCN6VsX(n,onPt,arg1,arg2);
  if(c==4) return GetCN4VsX(n,onPt,arg1,arg2);
  return GetCN2VsX(n,onPt,arg1,arg2);
};
TH1D *AliGFWFlowContainer::GetVNN(Int_t n, Int_t c, Bool_t onPt, Double_t arg1, Double_t arg2) {
  if(c==8) return GetVN8VsX(n,onPt,arg1,arg2);
  if(c==6) return GetVN6VsX(n,onPt,arg1,arg2);
  if(c==4) return GetVN4VsX(n,onPt,arg1,arg2);
  return GetVN2VsX(n,onPt,arg1,arg2);
};
TProfile *AliGFWFlowContainer::GetRefFlowProfile(const char *order, Double_t m1, Double_t m2) {
  TProfile *tpf = GetCorrXXVsMulti(order,0);
  Int_t b1 = tpf->FindBin(m1);
  Int_t b2 = tpf->FindBin(m2);
  if(b1<1) b1 = 1;
  if(b2<1) b2 = tpf->GetNbinsX();
  TProfile *tpf2 = new TProfile("verytempprof","vtpf",b2-b1+1,b1,b2);
  AliProfileSubset *tpf2SS = new AliProfileSubset(tpf2);
  delete tpf2;
  tpf2SS->CopyFromProfile(b1,b2,tpf);
  delete tpf;
  return tpf2SS;
};

//{2} particle correlations
Double_t AliGFWFlowContainer::CN2Value(Double_t cor2) {
  return cor2;
};
Double_t AliGFWFlowContainer::CN2Error(Double_t cor2e) {
  return cor2e;
};
Double_t AliGFWFlowContainer::VN2Value(Double_t cor2) {
  return TMath::Sqrt(cor2);
};
Double_t AliGFWFlowContainer::VN2Error(Double_t cor2, Double_t cor2e) {
  return 0.5*cor2e/TMath::Sqrt(cor2);
};
Double_t AliGFWFlowContainer::VDN2Value(Double_t cor2d, Double_t cor2) {
  return cor2d/TMath::Sqrt(cor2);
};
Double_t AliGFWFlowContainer::VDN2Error(Double_t cor2d, Double_t cor2de, Double_t cor2, Double_t cor2e) {
  return TMath::Sqrt(cor2de*cor2de/cor2 + 0.25*cor2d*cor2d*cor2e*cor2e/(cor2*cor2*cor2));
};


//C{4} and V{4} calculations 
Double_t AliGFWFlowContainer::CN4Value(Double_t cor4, Double_t cor2) {
  return cor4 - 2*TMath::Power(cor2,2);
};
Double_t AliGFWFlowContainer::CN4Error(Double_t cor4e, Double_t cor2, Double_t cor2e) {
  return TMath::Sqrt(cor4e*cor4e + 16*cor2*cor2*cor2e*cor2e);
};
Double_t AliGFWFlowContainer::DN4Value(Double_t cor4d, Double_t cor2d, Double_t cor2) {
  return cor4d - 2 * cor2d * cor2;
};
Double_t AliGFWFlowContainer::DN4Error(Double_t cor4de, Double_t cor2d, Double_t cor2de, Double_t cor2, Double_t cor2e) {
  return TMath::Sqrt(cor4de*cor4de + 4*cor2*cor2*cor2de*cor2de + 4*cor2d*cor2d*cor2e*cor2e);
};
Double_t AliGFWFlowContainer::VN4Value(Double_t c4) {
  return TMath::Power(-c4,1./4);
};
Double_t AliGFWFlowContainer::VN4Error(Double_t c4, Double_t c4e) {
  return TMath::Power(-c4,(-3./4))*c4e/4;
};
Double_t AliGFWFlowContainer::VDN4Value(Double_t d4, Double_t c4) {
  return -d4*TMath::Power(-c4,(-3./4));
};
Double_t AliGFWFlowContainer::VDN4Error(Double_t d4, Double_t d4e, Double_t c4, Double_t c4e) {
  return TMath::Sqrt(TMath::Power(-c4, -6./4)*d4e*d4e + 
		     TMath::Power(-c4,-14./4)*d4*d4*c4e*c4e*9./16);
};

//{6} particle correlations

Double_t AliGFWFlowContainer::CN6Value(Double_t cor6, Double_t cor4, Double_t cor2) {
  return cor6 - 9*cor2*cor4 + 12*cor2*cor2*cor2;
};
Double_t AliGFWFlowContainer::CN6Error(Double_t cor6e, Double_t cor4, Double_t cor4e,  Double_t cor2, Double_t cor2e) {
  Double_t inters[3];
  inters[0] = cor6e;
  inters[1] = -9*cor2*cor4e;
  inters[2] = (-9*cor4+36*cor2*cor2)*cor2e;
  Double_t sum=0;
  for(Int_t i=0;i<3;i++) sum+=(inters[i]*inters[i]);
  return TMath::Sqrt(sum);
};
Double_t AliGFWFlowContainer::DN6Value(Double_t cor6d, Double_t cor4d, Double_t cor2d, Double_t cor4, Double_t cor2) {
  return cor6d - 6*cor4d*cor2 - 3*cor2d*cor4 + 12*cor2d*cor2*cor2;
};
Double_t AliGFWFlowContainer::DN6Error(Double_t d6e, Double_t d4, Double_t d4e, Double_t d2,
				 Double_t d2e, Double_t c4, Double_t c4e, Double_t c2,
				 Double_t c2e) {
  Double_t inters[5];
  inters[0] = d6e;
  inters[1] = -6*c2*d4e;
  inters[2] = (-3*c4+12*c2*c2)*d2e;
  inters[3] = -3*d2*c4e;
  inters[4] = (-6*d4+24*d2*c2)*c2e;
  Double_t sum=0;
  for(Int_t i=0;i<5;i++) sum+=(inters[i]*inters[i]);
  return TMath::Sqrt(sum);
};
Double_t AliGFWFlowContainer::VN6Value(Double_t c6) {
  return TMath::Power(c6/4, 1./6);
};
Double_t AliGFWFlowContainer::VN6Error(Double_t c6, Double_t c6e) {
  return c6e/6*TMath::Power(4,-1./6)*TMath::Power(c6,-5./6);
};
Double_t AliGFWFlowContainer::VDN6Value(Double_t d6, Double_t c6) {
  return d6*TMath::Power(4,-1./6)*TMath::Power(c6,-5./6);
};
Double_t AliGFWFlowContainer::VDN6Error(Double_t d6, Double_t d6e, Double_t c6, Double_t c6e) {
  Double_t vdn6 = VDN6Value(d6, c6);
  Double_t dp = d6e/d6;
  Double_t cp = 5*c6e/6;
  return vdn6 * TMath::Sqrt(dp*dp+cp*cp);
};

// {8} particle correlations

Double_t AliGFWFlowContainer::CN8Value(Double_t cor8, Double_t cor6, Double_t cor4, Double_t cor2) {
  return cor8-16*cor6*cor2-18*cor4*cor4+144*cor4*cor2*cor2-144*cor2*cor2*cor2*cor2;
};
Double_t AliGFWFlowContainer::CN8Error(Double_t cor8e, Double_t cor6, Double_t cor6e,
				 Double_t cor4, Double_t cor4e, Double_t cor2, Double_t cor2e) {
  Double_t parts[4];
  parts[0] = cor8e;
  parts[1] = -16*cor2*cor6e;
  parts[2] = (-36*cor4+144*cor2*cor2)*cor4e;
  parts[3] = (-16*cor6+288*cor4*cor2+576*cor2*cor2*cor2)*cor2e;
  Double_t retval=0;
  for(Int_t i=0;i<4;i++) retval+=TMath::Power(parts[i],2);
  return TMath::Sqrt(retval);
};
Double_t AliGFWFlowContainer::DN8Value(Double_t cor8d, Double_t cor6d, Double_t cor4d, Double_t cor2d, Double_t cor6, Double_t cor4, Double_t cor2) {
  return cor8d - 12*cor6d*cor2 - 4*cor2d*cor6 - 18*cor4d*cor4 + 72*cor4d*cor2*cor2
    + 72*cor4*cor2*cor2d-144*cor2d*cor2*cor2*cor2;
};
Double_t AliGFWFlowContainer::DN8Error(Double_t d8e, Double_t d6, Double_t d6e, Double_t d4, 
				 Double_t d4e, Double_t d2, Double_t d2e, Double_t c6,
				 Double_t c6e, Double_t c4, Double_t c4e, Double_t c2,
				 Double_t c2e) {
  Double_t parts[7];
  parts[0] = d8e; //d/d8'
  parts[1] = -12*c2*d6e; //d/d6'
  parts[2] = -4*d2*c6e; //d/d6
  parts[3] = (-16*c4+72*d2)*d4e; //d/d4'
  parts[4] = (-16*d4+72*c2*d2)*c4e; //d/d4
  parts[5] = (-4*c6+72*c4*c2-144*c2*c2*c2)*d2e;
  parts[6] = (-12*d6+144*d4*c2+72*c4*d2-432*d2*c2*c2)*c2e;
  Double_t retval = 0;
  for(Int_t i=0;i<7;i++) retval+=TMath::Power(parts[i],2);
  return TMath::Sqrt(retval);
};
Double_t AliGFWFlowContainer::VN8Value(Double_t c8) {
  return TMath::Power(-c8/33, 1./8);
};
Double_t AliGFWFlowContainer::VN8Error(Double_t c8, Double_t c8e) {
  return c8e * 1./(8*c8) * VN8Value(c8);
};
Double_t AliGFWFlowContainer::VDN8Value(Double_t d8, Double_t c8) {
  return d8/c8 * VN8Value(c8);
};
Double_t AliGFWFlowContainer::VDN8Error(Double_t d8, Double_t d8e, Double_t c8, Double_t c8e) {
  Double_t vdn8v = VDN8Value(d8,c8);
  Double_t dd = d8e/d8;
  Double_t dc = -7*c8e/(8*c8);
  return vdn8v * TMath::Sqrt(dd*dd+dc*dc);
};
