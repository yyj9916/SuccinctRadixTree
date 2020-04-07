#ifndef LABEL_VECTOR_H_
#define LABEL_VECTOR_H_

#include <emmintrin.h>
#include <vector>
#include "config.hpp"
#include "assert.h"
using namespace std;

class LabelVector {
public:
    LabelVector():labels_(nullptr),num_bytes_(0){}
    LabelVector(vector<label_t>& bytemap);
	//LabelVector(char*& src);
    ~LabelVector(){if(labels_ != nullptr) delete[] labels_;}
	void serialize(char*& dst) const;
    //LabelVector* deSerialize(char*& src);
	void deSerialize(char*& src);
	position_t getSerializeSize() const; 
	position_t getNumBytes() const{return num_bytes_;}
	//void show(); //test
	//lookupkey
	string getMultiKey(const position_t start,const position_t strlen);
	bool IsSameKey(const string& str, const position_t& pos,const position_t strlen);
	bool search(const label_t target, position_t& pos, const position_t search_len) const;
private:
    label_t* labels_;
    position_t num_bytes_;
private:
    label_t read(const position_t pos) const{return labels_[pos];}
    label_t operator[](const position_t pos) const{return labels_[pos];}
    //小用线性，大用二分，在pos起始的search_len长度内搜索等于target的值，搜到返回true，并让pos指向该值

    bool linearSearch(const label_t target, position_t& pos, const position_t search_len) const;
    //bool binarySearch(const label_t target, position_t& pos, const position_t search_len) const;
    bool simdSearch(const label_t target, position_t& pos, const position_t search_len) const;

    //bool binarySearchGreaterThan(const label_t target, position_t& pos, const position_t search_len) const;
    //bool linearSearchGreaterThan(const label_t target, position_t& pos, const position_t search_len) const;
};

LabelVector::LabelVector(vector<label_t>& bytemap) {
    labels_ = new label_t[bytemap.size()];
	position_t i = 0;
	for(vector<label_t>::iterator it=bytemap.begin(); it!=bytemap.end();++it) {
		labels_[i] = *it;
		i++;
	}
    num_bytes_ = bytemap.size();
}

position_t LabelVector::getSerializeSize() const  {
    position_t size = sizeof(num_bytes_) + num_bytes_; 
	sizeAlign(size);   
	return size;
}

// | num_bytes_(32) | labels_(8)
void LabelVector::serialize(char*& dst) const {
	memcpy(dst, &num_bytes_, sizeof(num_bytes_)); //32bit
	dst += sizeof(num_bytes_);
	memcpy(dst, labels_, num_bytes_); //labels_[num_bytes_]->dst
	dst += num_bytes_;
	align(dst);
    }
/*    
LabelVector* LabelVector::deSerialize(char*& src) {
	LabelVector* lv = new LabelVector();
	memcpy(&(lv->num_bytes_), src, sizeof(lv->num_bytes_));//memcpy(dest,source,n)
	src += sizeof(lv->num_bytes_);
	lv->labels_ = new label_t[lv->num_bytes_];
	memcpy(lv->labels_,src,lv->num_bytes_);
	//lv->labels_ = const_cast<label_t*>(reinterpret_cast<const label_t*>(src));
	src += lv->num_bytes_;
	//align(src);
	return lv;
    }*/
void LabelVector::deSerialize(char*& src) {
	memcpy(&(num_bytes_), src, sizeof(num_bytes_));
	src += sizeof(num_bytes_);
	labels_ = const_cast<label_t*>(reinterpret_cast<const label_t*>(src));
	src += num_bytes_;
	align(src);
}

bool LabelVector::search(const label_t target, position_t& pos, position_t search_len) const {
    //skip terminator label
    if ((search_len > 1) && (labels_[pos] == kTerminator)) { //const label_t kTerminator = 255;
	pos++;
	search_len--;
    }

    if (search_len < 12)
		return linearSearch(target, pos, search_len);
    //if (search_len < 12)
	//return binarySearch(target, pos, search_len);
    else
		return simdSearch(target, pos, search_len);
}

//线性搜索，在pos起始的search_len长度内搜索target，搜到返回true，并让pos指向目标位置
bool LabelVector::linearSearch(const label_t target, position_t&  pos, const position_t search_len) const {
    for (position_t i = 0; i < search_len; i++) {
		if (target == labels_[pos + i]) {
	    	pos += i;
	    	return true;
		}
    }
    return false;
}
/*THIS IS WRONG
bool LabelVector::binarySearch(const label_t target, position_t& pos, const position_t search_len) const {
    position_t l = pos;
    position_t r = pos + search_len;
    while (l < r) {
	position_t m = (l + r) >> 1;
	if (target < labels_[m]) {
	    r = m ;
	} else if (target == labels_[m]) {
	    pos = m;
	    return true;
	} else {
	    l = m + 1;
	}
    }
    return false;
}*/

bool LabelVector::simdSearch(const label_t target, position_t& pos, const position_t search_len) const {
    position_t num_labels_searched = 0;
    position_t num_labels_left = search_len;
    while ((num_labels_left >> 4) > 0) {
	label_t* start_ptr = labels_ + pos + num_labels_searched;
	__m128i cmp = _mm_cmpeq_epi8(_mm_set1_epi8(target), 
				     _mm_loadu_si128(reinterpret_cast<__m128i*>(start_ptr)));
	unsigned check_bits = _mm_movemask_epi8(cmp);
	if (check_bits) {
	    pos += (num_labels_searched + __builtin_ctz(check_bits));
	    return true;
	}
	num_labels_searched += 16;
	num_labels_left -= 16;
    }

    if (num_labels_left > 0) {
	label_t* start_ptr = labels_ + pos + num_labels_searched;
	__m128i cmp = _mm_cmpeq_epi8(_mm_set1_epi8(target), 
				     _mm_loadu_si128(reinterpret_cast<__m128i*>(start_ptr)));
	unsigned leftover_bits_mask = (1 << num_labels_left) - 1;
	unsigned check_bits = _mm_movemask_epi8(cmp) & leftover_bits_mask;
	if (check_bits) {
	    pos += (num_labels_searched + __builtin_ctz(check_bits));
	    return true;
	}
    }

    return false;
}

//lookupKey
string LabelVector::getMultiKey(const position_t start,const position_t strlen) {
	assert(!((start+strlen) > num_bytes_));
	string str(labels_,start,strlen);
	return str;
}

bool LabelVector::IsSameKey(const string& str,const position_t& pos,const position_t strlen) {
	assert(!((pos+strlen) > num_bytes_));
	string s(labels_,pos,strlen);
	if(str == s) 
		return true;
	return false;
}
/*
void LabelVector::show() {
	//cout << "label num:"<<num_bytes_<<endl;//---------------1
	for (position_t i=0; i< num_bytes_ ; ++i) {
		cout << labels_[i];
	}
	cout << endl;
}*/

#endif