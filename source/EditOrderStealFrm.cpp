//---------------------------------------------------------------------------
#pragma hdrstop
#include <vcl.h>
#include <stdlib.h>
#include <vector.h>
#include <algorithm>
#include "util.h"
#include "orders.h"
#include "items.h"
#include "unit.h"
#include "EditOrderStealFrm.h"
#include "faction.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TEditOrderStealForm *EditOrderStealForm;
//---------------------------------------------------------------------------
__fastcall TEditOrderStealForm::TEditOrderStealForm(TComponent* Owner, AOrderSteal *_ord, AItems *its, AUnits *tars)
    : TForm(Owner),ord(_ord),items(its),targets(tars)
    ,slist(new TStringList)
    ,facnum_search(0)
{
 cbCommented->Checked=ord->commented;
 cbRepeating->Checked=ord->repeating;

 curtarnum=ord->tarnum;
 curitem=ord->item;
 AnsiString s;
 facdiap=-6;
 cbFacDiap->Items->BeginUpdate();
 cbFacDiap->Items->AddObject(" nonlocal",(TObject*)-4);
 cbFacDiap->Items->AddObject(" nonfriendly",(TObject*)-7);
 cbFacDiap->Items->AddObject(" unfriendly",(TObject*)-6);
 AFaction *myfac=_ord->orders->unit->faction;
 foreach(tars)
 {
   AUnit *un=*(AUnit**)iter;
   AFaction *fac=un->faction;
   int facnum=fac->number;
   if(un->num==curtarnum)
   {
     int att=myfac->GetAttitude(facnum);
     if(att>A_NEUTRAL)
       facdiap=-4;
     else if(att>A_UNFRIENDLY)
       facdiap=-7;
   }
   int ind=cbFacDiap->Items->IndexOfObject((TObject*)facnum);
   if(ind<0)
     cbFacDiap->Items->AddObject(fac->FullName(),(TObject*)facnum);
 }
 cbFacDiap->Items->EndUpdate();
 cbFacDiap->Sorted=true;
 cbFacDiap->ItemIndex=cbFacDiap->Items->IndexOfObject((TObject*)facdiap);

 cbItems->Items->BeginUpdate();
 foreach(items){
  AItem *it=*(AItem**)iter;
  AItemType *itype=it->type;
  s=itype->name;
  cbItems->Items->AddObject(s,(TObject*)itype);
  if(it->type->abr==curitem){
   cbItems->ItemIndex=cbItems->Items->Count-1;
  }
 }
 cbItems->Items->EndUpdate();
 cbItemsChange(this);
}
//---------------------------------------------------------------------------
__fastcall TEditOrderStealForm::~TEditOrderStealForm()
{
  delete slist;
}
//---------------------------------------------------------------------------
struct SortItem{
 int num;
 AUnit *unit;
};
//---------------------------------------------------------------------------
static int SortItemCompare(const SortItem &a,const SortItem &b){
 return a.num>b.num;
}
//---------------------------------------------------------------------------
void TEditOrderStealForm::MakeTargetList()
{
  AnsiString s;

  vector<SortItem> list;
  int itemcount = 0;
  AFaction *basefac = ord->orders->unit->faction;
  foreach(targets)
  {
	AUnit *un = *(AUnit**)iter;
	if(!inFacDiap(facdiap, un->faction,basefac)) continue;
	SortItem si;
	si.unit = un;
	si.num = un->enditems->GetNum(curitem);
	if(si.num) itemcount += si.num;
	list.push_back(si);
  }
  lbAll->Caption = itemcount;
  if(list.size()) stable_sort(list.begin(), list.end(), SortItemCompare);
	else
  if(facdiap == -6)
  {
	int ind = cbFacDiap->Items->IndexOfObject((TObject*)facdiap);
	cbFacDiap->Items->Delete(ind);
	facdiap = -7;
	cbFacDiap->ItemIndex = cbFacDiap->Items->IndexOfObject((TObject*)facdiap);
	MakeTargetList();
	return;
  }
  slist->Clear();
  for(int i=0,endi=list.size();i<endi;i++)
  {
	SortItem &si=list[i];
	AUnit *un=si.unit;
	s=un->FullName();
	if(un->faction->number)
	{
	  s+=", ";
	  s+=un->faction->FullName();
	}
	s+=" - ";
	s+=si.num;
	slist->AddObject(s,(TObject*)un);
	if(un->num==curtarnum)
	{
	  cbTargets->Text=s;
	}
  }
  cbTargets->Items=slist;
  cbTargetsChange(0);
}
//---------------------------------------------------------------------------
void __fastcall TEditOrderStealForm::bnOkClick(TObject *Sender)
{
 ord->commented=cbCommented->Checked;
 ord->repeating=cbRepeating->Checked;
 int inditem=cbItems->ItemIndex;
 if(inditem!=-1){
  AItemType *itype=(AItemType*)cbItems->Items->Objects[inditem];
  if(itype){
   ord->item=itype->abr;
  }else
   return;
 }else
   return;
 int indtar=cbTargets->Items->IndexOf(cbTargets->Text);
 AUnit *un=0;
 if(indtar<0)
 {
  AnsiString str=cbTargets->Text;
  if(str.Length())
  {
    if(isdigit(str[1])&&(str.Length()<2||isdigit(str[str.Length()-1])))
    {
      int num=atoi(str.c_str());
      un=targets->GetUnit(num);
    }
  }
  if(!un)
    return;
 }else
  un=(AUnit*)cbTargets->Items->Objects[indtar];
 if(ord->target!=un){
  ord->tarnum=un->num;
  ord->DeleteMessage(ord->message);
  ord->target=un;
 }
}
//---------------------------------------------------------------------------
void __fastcall TEditOrderStealForm::bnCancelClick(TObject *Sender)
{
 ord=0;
}
//---------------------------------------------------------------------------
void __fastcall TEditOrderStealForm::cbItemsChange(TObject *Sender)
{
 int ind=cbItems->ItemIndex;
 if(ind<0) return;
 AItemType *itype=(AItemType*)cbItems->Items->Objects[ind];
 if(itype){
   curitem=itype->abr;
 }else curitem="";
 MakeTargetList();
}
void __fastcall TEditOrderStealForm::cbTargetsChange(TObject *Sender)
{
 AnsiString str=cbTargets->Text;
 int indtar=cbTargets->Items->IndexOf(str);
 if(indtar==-1){
   TStrings *list=cbTargets->Items;
   if(!str.Length())
     list->Clear();
   for(int i=list->Count-1;i>=0;i--)
   {
     if(AnsiIPos(list->Strings[i],str)!=0)
       continue;
     list->Delete(i);
   }
   if(!list->Count)
   {
     int i=cbTargets->SelStart;
     list->Assign(slist);
     cbTargets->SelStart=i;
   }
   return;
 }
 AUnit *un=(AUnit*)cbTargets->Items->Objects[indtar];
 curtarnum=un->num;
}
//---------------------------------------------------------------------------
void __fastcall TEditOrderStealForm::cbFacDiapChange(TObject *Sender)
{
  int ind=cbFacDiap->ItemIndex;
  if(ind<0) return;
  ind=(int)cbFacDiap->Items->Objects[ind];
  if(facdiap==ind) return;
  facdiap=ind;
  MakeTargetList();
}
//---------------------------------------------------------------------------
void __fastcall TEditOrderStealForm::cbFacDiapEnter(TObject *Sender)
{
  facnum_search=0;
}
//---------------------------------------------------------------------------
void __fastcall TEditOrderStealForm::cbFacDiapKeyPress(TObject *Sender,
      char &Key)
{
  if(Key>='0'&&Key<='9')
  {
    facnum_search*=10;
    facnum_search+=Key-'0';
    Key=0;
    int ind=cbFacDiap->Items->IndexOfObject((TObject*)facnum_search);
    if(ind<0)
      return;
    cbFacDiap->ItemIndex=ind;
    cbFacDiapChange(0);
  }else
  {
    facnum_search=0;
  }
}
//---------------------------------------------------------------------------

