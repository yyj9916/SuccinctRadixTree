#ifndef SUCC_RADIX_TREE_H_
#define SUCC_RADIX_TREE_H_

#include <map>
#include <bitset>
#include <string>
#include <utility>
#include <algorithm>
#include "config.hpp"
#include "LabelVector.hpp"
#include "BitVectorRank.hpp"
#include "BitVectorSelect.hpp"
#include "LOUDSBuilder.hpp"

void vecToMemSerialize(vector<int>& src,char*& dst);
void MemToVecDeSerialize(char*& src,vector<int>& dst);

class SuccRadixTree {
public:
    SuccRadixTree(){}
    SuccRadixTree(char*& Mem);
    SuccRadixTree(map<string,string>& keylist);
    ~SuccRadixTree() {destory();};
    //void ShowTree();
public:
    void serialize(char*& dstMem); //transfom to dst, size = dstSize(Byte)
    position_t getSerializedSize() const; 
    //point Query
    bool lookupKey(const string& key,int& valuepos); 
    //Range Query
    // [left,∞) bounded,∞
    void QueryLeftBounded(const string& key,int& leftValueIndex,int& rightValueIndex);
    //(left,∞) unbounded,∞
    void QueryLeftUnBounded(const string& key,int& leftValueIndex,int& rightValueIndex);
    //(∞,right] ∞,bounded
    void QueryRightBounded(const string& key,int& leftValueIndex,int& rightValueIndex);
    //(∞,right) ∞,unbounded
    void QueryRightUnBounded(const string& key,int& leftValueIndex,int& rightValueIndex);
    //[left,right] bounded,bounded
    void QueryLeftBoundedRightBounded(const string& leftKey,const string& rightKey,int& leftValueIndex,int& rightValueIndex);
    //(left,right) unbounded,unbounded
    void QueryLeftUnBoundedRightUnBounded(const string& leftKey,const string& rightKey,int& leftValueIndex,int& rightValueIndex);
    //[left,right) bounded,unbounded
    void QueryLeftBoundedRightUnBounded(const string& leftKey,const string& rightKey,int& leftValueIndex,int& rightValueIndex);
    //(left,right] unbounded,bounded
    void QueryLeftUnBoundedRightBounded(const string& leftKey,const string& rightKey,int& leftValueIndex,int& rightValueIndex);
    //bool boundQuery(const string& key, int& leftpoint,int &rightpoint,int& point);
private: 
    LabelVector* Labels;
    BitVectorRank* HasChild;
    BitVectorSelect* Louds;
    BitVector* IsKey;
    vector<int> ValuesPos;

    position_t labels_num_ = 0;
    position_t serializedSize = 0;
private:
    void destory();
    position_t nodeSize(const position_t pos) const; //Louds curr 1 to next 1
    position_t keySize(const position_t pos) const;
    vector<pair<string,position_t> > childNodeList(const position_t start,const position_t nodesize);
    int nodeLeftValue(const position_t nodepos);
    int nodeRightValue(const position_t nodepos);
    // if key founded, point -> valuepos , else  leftpoint < key < leftpoint,point = -1
    bool boundQuery(const string& key, int& leftpoint,int &rightpoint,int& point);
};

SuccRadixTree::SuccRadixTree(char*& Mem) {
    Labels = new LabelVector();
    Labels->deSerialize(Mem);
    HasChild = new BitVectorRank();
    HasChild->deSerialize(Mem);
    Louds = new BitVectorSelect();
    Louds->deSerialize(Mem);
    IsKey = new BitVector();
    IsKey->deSerialize(Mem);
    MemToVecDeSerialize(Mem,ValuesPos);

    serializedSize = Labels->getSerializeSize()+HasChild->getSerializedSize()
        +Louds->getSerializedSize()+IsKey->getSerializedSize();
    sizeAlign(serializedSize);
    serializedSize = serializedSize + 4 + ValuesPos.size()*sizeof(int);
    sizeAlign(serializedSize);
}

SuccRadixTree::SuccRadixTree(map<string,string>& keylist) {
    Regional R(keylist);
    R.transform(keylist); //map->vector radix tree
    LOUDSBuilder L;
	L.unification(R);

    Labels = new LabelVector(L.Labels);
    HasChild = new BitVectorRank(L.HasChild);
    Louds = new BitVectorSelect(L.Louds);
    IsKey = new BitVector(L.IsKey);
    ValuesPos.assign(L.ValuesPos.begin(),L.ValuesPos.end());
    labels_num_ = L.getLabelNum();

    serializedSize = Labels->getSerializeSize()+HasChild->getSerializedSize()
        +Louds->getSerializedSize()+IsKey->getSerializedSize();
    serializedSize = serializedSize + 4 + ValuesPos.size()*sizeof(int);
}

void SuccRadixTree::serialize(char*& dstMem) {
    Labels->serialize(dstMem);
    HasChild->serialize(dstMem);
    Louds->serialize(dstMem);
    IsKey->serialize(dstMem);
    vecToMemSerialize(ValuesPos,dstMem);
}

position_t SuccRadixTree::getSerializedSize() const {
    return serializedSize;
}

bool SuccRadixTree::lookupKey(const std::string& key,int& valuepos) {
    position_t key_curr_pos = 0;
    position_t bitmap_curr_pos = 0;

    while(bitmap_curr_pos < labels_num_)  {
        //cout << "mmmm bitmap="<<bitmap_curr_pos<<endl;//------------------------
        //cout << "nnnn key="<<key[key_curr_pos]<<endl;//-----------------
        position_t nodesize = nodeSize(bitmap_curr_pos);
        if(!Labels->search(key[key_curr_pos],bitmap_curr_pos,nodesize)) {
            valuepos = -1;
            return false;
        }

        position_t keysize = keySize(bitmap_curr_pos);
        if(IsKey->readBit(bitmap_curr_pos)) {
            //cout << "ooooo bitmap="<<bitmap_curr_pos<<endl;//------------------------
            string labelKey = Labels->getMultiKey(bitmap_curr_pos,keysize);
            string subKey = key.substr(key_curr_pos,keysize);
            //cout << "label=" <<labelKey <<"  subKey="<<subKey<<endl;//---------------
             if(subKey == Terminator) { //不存在形如fTerminator的key
                valuepos = -1;
                return false;
            }
            if(labelKey == subKey) {
                key_curr_pos += keysize;
                //key terminate
                if(key_curr_pos == key.length()) {
                    cout << "miao~"<<endl;//----------------------
                    if(ValuesPos[bitmap_curr_pos] != -1) {
                        valuepos = ValuesPos[bitmap_curr_pos];
                        return true;
                    }
                    if(HasChild->readBit(bitmap_curr_pos)) {
                        position_t childPos = HasChild->rank(bitmap_curr_pos)+1;
                        bitmap_curr_pos = Louds->select(childPos);
                        if(Labels->getMultiKey(bitmap_curr_pos,1) == Terminator) {//IsPrefixKey
                            valuepos = ValuesPos[bitmap_curr_pos];
                            return true;
                        }                          
                        valuepos = -1;
                        return false;
                    }
                    valuepos = -1;
                    return false;
                } //end if
                    
                //key large
                 if(key_curr_pos > (key.length())) {
                     valuepos = -1;
                     return false;
                 }                 

                // if trie branch terminates
	            if (!HasChild->readBit(bitmap_curr_pos)) {
                    valuepos = -1;
                    return false;
                }
                    
                //move to child
                position_t childPos = HasChild->rank(bitmap_curr_pos)+1;
                bitmap_curr_pos = Louds->select(childPos);

            }
            else {
                valuepos = -1;
                return false;
            }
                
        }
        else {
            valuepos = -1;
            return false;
        }
            
        
    }
}

//assist class function
void SuccRadixTree::destory() {
    delete Labels;
    delete HasChild;
    delete Louds;
    delete IsKey;
}

position_t SuccRadixTree::nodeSize(const position_t pos) const {
    assert(Louds->readBit(pos));
    if(pos + Louds->distanceToNextSetBit(pos) > labels_num_)
        return labels_num_ - pos;
    return Louds->distanceToNextSetBit(pos);
}

position_t SuccRadixTree::keySize(const position_t pos) const {
    assert(IsKey->readBit(pos));
    if(pos + IsKey->distanceToNextSetBit(pos) > labels_num_)
        return labels_num_ - pos;
    return IsKey->distanceToNextSetBit(pos);
}

//childstring,haschildpos
vector<pair<string,position_t> > SuccRadixTree::childNodeList(const position_t start,const position_t nodesize) {
    assert(IsKey->readBit(start) == 1);
    vector<pair<string,position_t> > childList;
    position_t keypos = start;

    while(keypos < (start+nodesize)) {
        childList.push_back(make_pair(Labels->getMultiKey(keypos,keySize(keypos)),keypos));
        keypos += IsKey->distanceToNextSetBit(keypos);
    }
    return childList;
}

//return the rightest valuepos of node
int SuccRadixTree::nodeRightValue(const position_t nodepos)  {
    if(!HasChild->readBit(nodepos)) {
        return ValuesPos[nodepos];
    }
    else {
        if(HasChild->rank(nodepos)+1 == Louds->getOnesNum()) { //最后一个节点
            return ValuesPos[labels_num_-IsKey->distanceToPrevSetBit(labels_num_)];
        }
        else {
            position_t pos = Louds->select(HasChild->rank(nodepos)+2);
            pos -= IsKey->distanceToPrevSetBit(pos);
            return nodeLeftValue(pos);
        }
    }

}

int SuccRadixTree::nodeLeftValue(const position_t nodepos)  {
    if(!HasChild->readBit(nodepos)) {
        return ValuesPos[nodepos];
    }
    else {
        return nodeLeftValue(Louds->select(HasChild->rank(nodepos)+1));
    }
}

bool SuccRadixTree::boundQuery(const string& key, int& leftpoint,int &rightpoint,int& point)  {
    if(lookupKey(key,point)) {
        leftpoint = point-1;
        rightpoint = point+1;
        return true;
    }
    else {
        position_t key_curr_pos = 0;
        position_t bitmap_curr_pos = 0;
        bool flag = true;

        while(flag) {
            vector<pair<string,position_t> > childList = childNodeList(bitmap_curr_pos,nodeSize(bitmap_curr_pos));
            int i,loopflag = 0;
            for(i=0; i < childList.size();++i) {
                string subKey = key.substr(key_curr_pos,keySize(childList[i].second));
                //join loop
                if(childList[i].first == subKey) {
                    key_curr_pos += subKey.length();

                    //over
                    if(HasChild->readBit(childList[i].second) == 0 && key_curr_pos < key.length()) {
                        leftpoint = ValuesPos[childList[i].second];
                        rightpoint = leftpoint +1;
                        return false; 
                    }
                    //under
                    if(HasChild->readBit(childList[i].second) == 1 && key_curr_pos >= key.length()) {
                        position_t pos = HasChild->rank(childList[i].second)+1;
                        bitmap_curr_pos = Louds->select(pos);
                        rightpoint = nodeLeftValue(bitmap_curr_pos);
                        leftpoint = rightpoint -1;
                        return false;
                    }

                    //continue
                    if(HasChild->readBit(childList[i].second) == 1 && key_curr_pos < key.length()) {
                        position_t pos = HasChild->rank(childList[i].second)+1;
                        bitmap_curr_pos = Louds->select(pos);
                        loopflag= 1;
                    }
                    break;
                }//end if
            }
            if(loopflag == 1) continue;
            else flag=false;
            
            string subKey = key.substr(key_curr_pos,key.length()-key_curr_pos);
            i = 0;
            while(i < childList.size() && childList[i].first < subKey) i++;
             // key < left node
            if(i == 0) {
                if(HasChild->readBit(childList[0].second)) {
                    rightpoint = nodeLeftValue(bitmap_curr_pos);
                    leftpoint = rightpoint - 1;
                    return false;
                }
                else {
                    rightpoint = ValuesPos[bitmap_curr_pos];
                    leftpoint = rightpoint - 1;
                    return false; 
                }
            }

            // key > right node
            else if(i >= childList.size()) {
                if(HasChild->readBit(childList[childList.size()-1].second)) {
                    leftpoint = nodeRightValue(childList[childList.size()-1].second);
                    rightpoint = leftpoint + 1;
                    return false;
                }
                else {
                    leftpoint = ValuesPos[childList[childList.size()-1].second];
                    rightpoint = leftpoint+1;
                    return false; 
                }  
            }
             // left node < key < right node
            else {
                if(HasChild->readBit(childList[i-1].second) == 1) {
                    leftpoint = nodeRightValue(childList[i-1].second);
                    rightpoint = leftpoint+1;
                    return false;
                }
                else {
                    leftpoint = ValuesPos[childList[i-1].second];
                    rightpoint = leftpoint+1;
                    return false;
                }
            }
        }// end while
    }//end else
}

//Range Query
// [left,∞) bounded,∞
void SuccRadixTree::QueryLeftBounded(const string& key,int& leftValueIndex,int& rightValueIndex) {
    int point,left,right;
    if(boundQuery(key,left,right,point)) {
        leftValueIndex = point;
        rightValueIndex = labels_num_-1;
        return;
    }
    else {
        leftValueIndex = right;
        rightValueIndex = labels_num_-1;
        return;
    }
}
//(left,∞) unbounded,∞
void SuccRadixTree::QueryLeftUnBounded(const string& key,int& leftValueIndex,int& rightValueIndex) {
    int point,left;
    rightValueIndex = labels_num_-1;
    return;
}

//(∞,right] ∞,bounded
void SuccRadixTree::QueryRightBounded(const string& key,int& leftValueIndex,int& rightValueIndex) {
    int point,left,right;
    if(boundQuery(key,left,right,point)) {
        rightValueIndex = point;
        leftValueIndex = 0;
        return;
    }
    else {
        leftValueIndex = left;
        rightValueIndex = 0;
        return;
    }
}
//(∞,right) ∞,unbounded
void SuccRadixTree::QueryRightUnBounded(const string& key,int& leftValueIndex,int& rightValueIndex) {
    int point,right;
    boundQuery(key,rightValueIndex,right,point);
    leftValueIndex = 0;
    return;
}
//[left,right] bounded,bounded
void SuccRadixTree::QueryLeftBoundedRightBounded(const string& leftKey,const string& rightKey,int& leftValueIndex,int& rightValueIndex) {
    int point,left,right;
    if(boundQuery(leftKey,left,right,point)) 
        leftValueIndex = point;
    else 
        leftValueIndex = right;

    if(boundQuery(rightKey,left,right,point)) 
        leftValueIndex = point;
    else 
        leftValueIndex = left;
    return;
}
//(left,right) unbounded,unbounded
void SuccRadixTree::QueryLeftUnBoundedRightUnBounded(const string& leftKey,const string& rightKey,int& leftValueIndex,int& rightValueIndex) {
    int point,left,right;
    boundQuery(leftKey,left,leftValueIndex,point);
    boundQuery(rightKey,rightValueIndex,right,point);
    return;
}
//[left,right) bounded,unbounded
void SuccRadixTree::QueryLeftBoundedRightUnBounded(const string& leftKey,const string& rightKey,int& leftValueIndex,int& rightValueIndex) {
    int point,left,right;
    if(boundQuery(leftKey,left,right,point)) 
        leftValueIndex = point;
    else 
        leftValueIndex = right;

    boundQuery(rightKey,rightValueIndex,right,point);
    return;
}
//(left,right] unbounded,bounded
void SuccRadixTree::QueryLeftUnBoundedRightBounded(const string& leftKey,const string& rightKey,int& leftValueIndex,int& rightValueIndex) {
    int point,left,right;
    boundQuery(leftKey,left,leftValueIndex,point);
    
    if(boundQuery(rightKey,left,right,point)) 
        leftValueIndex = point;
    else 
        leftValueIndex = left;
}
//show
/*
void SuccRadixTree::ShowTree() {
    if(labels_num_ > 0) {
        Labels->show();
        HasChild->show();
        Louds->show();
        IsKey->show();
        cout << "RankLut:"<<endl;
        HasChild->showRankLut();
        cout << "SelectLut:" <<endl;
        Louds->showSelectLut();
    }
}*/

//assistant
void vecToMemSerialize(vector<int>& src,char*& dst) {
    position_t num_ints_ = static_cast<position_t>(src.size());
    memcpy(dst,&num_ints_,sizeof(num_ints_));
    dst += sizeof(num_ints_);
    memcpy(dst, &src[0], src.size()*sizeof(int));
	dst += src.size()*sizeof(int);
	align(dst);
}

void MemToVecDeSerialize(char*& src,vector<int>& dst)  {
    position_t num_ints_;
    memcpy(&(num_ints_), src, sizeof(num_ints_));
    src += sizeof(num_ints_);
    int* ints_ = const_cast<int*>(reinterpret_cast<const int*>(src));
    dst.clear();
    for(int i =0;i < num_ints_;++i) {
        dst.push_back(ints_[i]);
    }
    src += num_ints_*sizeof(int);
    align(src);
}

#endif