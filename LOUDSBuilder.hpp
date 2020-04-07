#ifndef LOUDSBUILDER_H_
#define LOUDSBUILDER_H_
#include <iostream>
#include <vector>
#include <map>
#include <iomanip>
#include <bitset>
#include "Regional.hpp"
#include "config.hpp"

using namespace std;

class LOUDSBuilder
{
public:
	LOUDSBuilder() {};
	~LOUDSBuilder() { this->clear(); };
	void unification(Regional& R); //sLOUDS
	void ShowUnif(); 
	int getLabelNum();
	
public:
	vector<label_t> Labels;
	vector<word_t> HasChild; 
	vector<word_t> Louds;      
	vector<word_t> IsKey;       
	vector<int> ValuesPos;

private:
	position_t label_num_ = 0;    
	//assist function
	void push(label_t,int,int,int,int);
	void clear() {
		Labels.clear();
		HasChild.clear();
		Louds.clear();
		IsKey.clear();
		ValuesPos.clear();
	}
	void setBit(std::vector<word_t>& bits, const int pos);
};

void LOUDSBuilder::unification(Regional& R) {
	if (R.getNodeNum() == 0) exit(0);
	
	label_num_ = 0;
	this->clear();
	vector<vector<Sparse>>::iterator it;
	for (it = R.sLOUDS.begin(); it != R.sLOUDS.end(); ++it) {
		for (int i = 0; i < (*it).size(); ++i) {
			if ((*it)[i].SMulti.length() > 1) {  
				push((*it)[i].SLabels, (*it)[i].SHasChild, (*it)[i].SLOUDS, 1, (*it)[i].SValues);
				for (int pos = 1; pos < (*it)[i].SMulti.length(); ++pos) {
					push((*it)[i].SMulti[pos],0,0,0,-1);
				}
			}
			else {
				push((*it)[i].SLabels, (*it)[i].SHasChild, (*it)[i].SLOUDS, 1, (*it)[i].SValues);
			}
		}
	}//end for
}

void LOUDSBuilder::push(label_t c, int haschild, int louds, int iskey, int v) {
	Labels.push_back(c);
	if ((Labels.size() % kWordSize == 0) || (HasChild.size() == 0) ) {
		HasChild.push_back((word_t)(0));
		Louds.push_back((word_t)(0));
		IsKey.push_back((word_t)(0));
	}
	if (haschild == 1) setBit(HasChild, Labels.size()-1);
	if (louds == 1) setBit(Louds, Labels.size() - 1);
	if (iskey == 1) setBit(IsKey, Labels.size() - 1);
	/*
	HasChild.push_back(haschild);
	Louds.push_back(louds);
	IsKey.push_back(iskey);*/
	ValuesPos.push_back(v);
	label_num_++;
}

void LOUDSBuilder::ShowUnif() {
	for (int n = 0; n < label_num_; ++n) {
		cout <<Labels[n] ;
	}
	cout << endl;
	for (int n = 0; n < HasChild.size(); ++n) {
		cout << bitset<64>(HasChild[n]) << " ";
	}
	cout << endl;
	for (int n = 0; n < Louds.size(); ++n) {
		cout << bitset<64>(Louds[n]) << " ";
	}
	cout << endl;
	for (int n = 0; n < IsKey.size(); ++n) {
		cout << bitset<64>(IsKey[n]) << " ";
	}
	cout << endl;
	for (int n = 0; n < label_num_; ++n) {
		cout << setw(3) << ValuesPos[n] << " ";
	}
	cout << endl;
}

int LOUDSBuilder::getLabelNum() {
	return label_num_;
}

void LOUDSBuilder::setBit(std::vector<word_t>& bits, const int pos) { 
	if (pos > (bits.size() * kWordSize)) exit(0);
	int word_id = pos / kWordSize;
	int offset = pos % kWordSize;
	bits[word_id] |= (kMsbMask >> offset);
}

#endif





