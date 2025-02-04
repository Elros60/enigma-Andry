#include "MCHAlign/AliMillePedeRecord.h"
#include <TMath.h>
// #include "MCHAlign/AliLog.h"
#include "Framework/Logger.h"

/**********************************************************************************************/
/* AliMillePedeRecords: class to store the data of single track processing                    */
/* Format: for each measured point the data is stored consequtively                           */
/* INDEX                                                      VALUE                           */
/* -1                                                         residual                        */
/* Local_param_id                                             dResidual/dLocal_param          */
/* ...                                                        ...                             */
/* -2                                                         weight of the measurement       */
/* Global_param_od                                            dResidual/dGlobal_param         */
/* ...                                                        ...                             */
/*                                                                                            */
/* The records for all processed tracks are stored in the temporary tree in orgder to be      */
/* reused for multiple iterations of MillePede                                                */
/*                                                                                            */
/* Author: ruben.shahoyan@cern.ch                                                             */
/*                                                                                            */
/**********************************************************************************************/

ClassImp(AliMillePedeRecord)

  //_____________________________________________________________________________________________
  AliMillePedeRecord::AliMillePedeRecord() : fSize(0), fNGroups(0), fRunID(0), fGroupID(0), fIndex(0), fValue(0), fWeight(1)
{
  SetUniqueID(0);
}

//_____________________________________________________________________________________________
AliMillePedeRecord::AliMillePedeRecord(const AliMillePedeRecord& src) : TObject(src), fSize(src.fSize), fNGroups(src.fNGroups), fRunID(src.fRunID), fGroupID(0), fIndex(0), fValue(0), fWeight(src.fWeight)
{
  // copy ct-r
  fIndex = new Int_t[GetDtBufferSize()];
  memcpy(fIndex, src.fIndex, fSize * sizeof(Int_t));
  fValue = new Double_t[GetDtBufferSize()];
  memcpy(fValue, src.fValue, fSize * sizeof(Double_t));
  fGroupID = new UShort_t[GetGrBufferSize()];
  memcpy(fGroupID, src.fGroupID, GetGrBufferSize() * sizeof(UShort_t));
}

//_____________________________________________________________________________________________
AliMillePedeRecord& AliMillePedeRecord::operator=(const AliMillePedeRecord& rhs)
{
  // assignment op-r
  if (this != &rhs) {
    Reset();
    for (int i = 0; i < rhs.GetSize(); i++) {
      Double_t val;
      Int_t ind;
      rhs.GetIndexValue(i, ind, val);
      AddIndexValue(ind, val);
    }
    fWeight = rhs.fWeight;
    fRunID = rhs.fRunID;
    for (int i = 0; i < rhs.GetNGroups(); i++)
      MarkGroup(rhs.GetGroupID(i));
  }
  return *this;
}

//_____________________________________________________________________________________________
AliMillePedeRecord::~AliMillePedeRecord()
{
  delete[] fIndex;
  delete[] fValue;
  delete[] fGroupID;
}

//_____________________________________________________________________________________________
void AliMillePedeRecord::Reset()
{
  // reset all
  fSize = 0;
  for (int i = fNGroups; i--;)
    fGroupID[i] = 0;
  fNGroups = 0;
  fRunID = 0;
  fWeight = 1.;
}

//_____________________________________________________________________________________________
void AliMillePedeRecord::Print(const Option_t*) const
{
  // print itself
  if (!fSize) {
    LOG(info) << "No data";
    return;
  }
  int cnt = 0, point = 0;
  //
  if (fNGroups)
    printf("Groups: ");
  for (int i = 0; i < fNGroups; i++)
    printf("%4d |", GetGroupID(i));
  printf("Run: %9d Weight: %+.2e\n", fRunID, fWeight);
  while (cnt < fSize) {
    //
    Double_t resid = fValue[cnt++];
    Double_t* derLoc = GetValue() + cnt;
    int* indLoc = GetIndex() + cnt;
    int nLoc = 0;
    while (!IsWeight(cnt)) {
      nLoc++;
      cnt++;
    }
    Double_t weight = GetValue(cnt++);
    Double_t* derGlo = GetValue() + cnt;
    int* indGlo = GetIndex() + cnt;
    int nGlo = 0;
    while (!IsResidual(cnt) && cnt < fSize) {
      nGlo++;
      cnt++;
    }
    //
    printf("\n*** Point#%2d | Residual = %+.4e | Weight = %+.4e\n", point++, resid, weight);
    printf("Locals : ");
    for (int i = 0; i < nLoc; i++)
      printf("[%5d] %+.4e|", indLoc[i], derLoc[i]);
    printf("\n");
    printf("Globals: ");
    for (int i = 0; i < nGlo; i++)
      printf("[%5d] %+.4e|", indGlo[i], derGlo[i]);
    printf("\n");
    //
  }
  //
}

//_____________________________________________________________________________________________
Double_t AliMillePedeRecord::GetGloResWProd(Int_t indx) const
{
  // get sum of derivative over global variable indx * res. at point * weight
  if (!fSize) {
    LOG(info) << "No data";
    return 0;
  }
  int cnt = 0;
  double prodsum = 0.0;
  //
  while (cnt < fSize) {
    //
    Double_t resid = fValue[cnt++];
    while (!IsWeight(cnt))
      cnt++;
    Double_t weight = GetValue(cnt++);
    Double_t* derGlo = GetValue() + cnt;
    int* indGlo = GetIndex() + cnt;
    int nGlo = 0;
    while (!IsResidual(cnt) && cnt < fSize) {
      nGlo++;
      cnt++;
    }
    for (int i = nGlo; i--;)
      if (indGlo[i] == indx)
        prodsum += resid * weight * derGlo[i];
    //
  }
  return prodsum;
}

//_____________________________________________________________________________________________
Double_t AliMillePedeRecord::GetGlobalDeriv(Int_t pnt, Int_t indx) const
{
  // get derivative over global variable indx at point pnt
  if (!fSize) {
    LOG(error) << "No data";
    return 0;
  }
  int cnt = 0, point = 0;
  //
  while (cnt < fSize) {
    //
    cnt++;
    while (!IsWeight(cnt))
      cnt++;
    cnt++;
    Double_t* derGlo = GetValue() + cnt;
    int* indGlo = GetIndex() + cnt;
    int nGlo = 0;
    while (!IsResidual(cnt) && cnt < fSize) {
      nGlo++;
      cnt++;
    }
    //
    if (pnt != point++)
      continue;
    for (int i = nGlo; i--;)
      if (indGlo[i] == indx)
        return derGlo[i];
    break;
  }
  return 0;
  //
}

//_____________________________________________________________________________________________
Double_t AliMillePedeRecord::GetLocalDeriv(Int_t pnt, Int_t indx) const
{
  // get derivative over local variable indx at point pnt
  if (!fSize) {
    LOG(error) << "No data";
    return 0;
  }
  int cnt = 0, point = 0;
  //
  while (cnt < fSize) {
    //
    cnt++;
    Double_t* derLoc = GetValue() + cnt;
    int* indLoc = GetIndex() + cnt;
    int nLoc = 0;
    while (!IsWeight(cnt)) {
      nLoc++;
      cnt++;
    }
    cnt++;
    while (!IsResidual(cnt) && cnt < fSize)
      cnt++;
    if (pnt != point++)
      continue;
    for (int i = nLoc; i--;)
      if (indLoc[i] == indx)
        return derLoc[i];
    break;
  }
  return 0;
  //
}

//_____________________________________________________________________________________________
Double_t AliMillePedeRecord::GetResidual(Int_t pnt) const
{
  // get residual at point pnt
  if (!fSize) {
    LOG(error) << "No data";
    return 0;
  }
  int cnt = 0, point = 0;
  //
  while (cnt < fSize) {
    //
    Double_t resid = fValue[cnt++];
    while (!IsWeight(cnt))
      cnt++;
    cnt++;
    while (!IsResidual(cnt) && cnt < fSize)
      cnt++;
    if (pnt != point++)
      continue;
    return resid;
  }
  return 0;
  //
}

//_____________________________________________________________________________________________
Double_t AliMillePedeRecord::GetWeight(Int_t pnt) const
{
  // get weight of point pnt
  if (!fSize) {
    LOG(error) << "No data";
    return 0;
  }
  int cnt = 0, point = 0;
  //
  while (cnt < fSize) {
    //
    cnt++;
    while (!IsWeight(cnt))
      cnt++;
    if (point == pnt)
      return GetValue(cnt);
    ;
    cnt++;
    while (!IsResidual(cnt) && cnt < fSize)
      cnt++;
    point++;
  }
  return -1;
  //
}

//_____________________________________________________________________________________________
void AliMillePedeRecord::ExpandDtBuffer(Int_t bfsize)
{
  // add extra space for derivatives data
  bfsize = TMath::Max(bfsize, GetDtBufferSize());
  Int_t* tmpI = new Int_t[bfsize];
  memcpy(tmpI, fIndex, fSize * sizeof(Int_t));
  delete[] fIndex;
  fIndex = tmpI;
  //
  Double_t* tmpD = new Double_t[bfsize];
  memcpy(tmpD, fValue, fSize * sizeof(Double_t));
  delete[] fValue;
  fValue = tmpD;
  //
  SetDtBufferSize(bfsize);
}

//_____________________________________________________________________________________________
void AliMillePedeRecord::ExpandGrBuffer(Int_t bfsize)
{
  // add extra space for groupID data
  bfsize = TMath::Max(bfsize, GetGrBufferSize());
  UShort_t* tmpI = new UShort_t[bfsize];
  memcpy(tmpI, fGroupID, fNGroups * sizeof(UShort_t));
  delete[] fGroupID;
  fGroupID = tmpI;
  for (int i = fNGroups; i < bfsize; i++)
    fGroupID[i] = 0;
  //
  SetGrBufferSize(bfsize);
}

//_____________________________________________________________________________________________
void AliMillePedeRecord::MarkGroup(Int_t id)
{
  // mark the presence of the detector group
  id++; // groupID is stored as realID+1
  if (fNGroups > 0 && fGroupID[fNGroups - 1] == id)
    return; // already there
  if (fNGroups >= GetGrBufferSize())
    ExpandGrBuffer(2 * (fNGroups + 1));
  fGroupID[fNGroups++] = id;
}
