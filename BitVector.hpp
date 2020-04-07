#ifndef LABELVECTOR_H_
#define LABELVECTOR_H_

#include <assert.h>
#include <vector>

#include "config.hpp"
using namespace std;

class BitVector {
public:
    BitVector():num_bits_(0),bits_(nullptr) {};
    BitVector(vector<word_t>& bitmap); //传入LOUDS转化后的结果
    ~BitVector() {if(bits_!=nullptr) delete[] bits_;}
    void serialize(char*& dst) const;
    //BitVector* deSerialize(char*& src);//读入模块
    void deSerialize(char*& src);
    position_t getSerializedSize() const;
    //void show();
public:
    bool readBit(const position_t pos) const;
    static bool setBit(const position_t pos);
    position_t distanceToNextSetBit(const position_t pos) const;
    position_t distanceToPrevSetBit(const position_t pos) const;
protected:
    position_t num_bits_; 
    position_t num_words_; 
    word_t* bits_; 
protected: //assistant
    position_t getBitNum() {return num_bits_;} 
    position_t getWordsNum() const {return num_words_;};
    position_t getVectorWords();
};

BitVector::BitVector(vector<word_t>& bitmap) {
    bits_ = new word_t[bitmap.size()];
    position_t i = 0;
	for(vector<word_t>::iterator it=bitmap.begin(); it!=bitmap.end();++it) {
		bits_[i] = *it;
		i++;
	}
    num_words_ = bitmap.size();
    num_bits_ = num_words_*64;
}

//  | num_bits_(32)| num_words(32)| bits_(64)
void BitVector::serialize(char*& dst) const{
    memcpy(dst, &num_bits_, sizeof(num_bits_));
	dst += sizeof(num_bits_); 
    memcpy(dst, &num_words_, sizeof(num_words_)); 
	dst += sizeof(num_words_); 
	memcpy(dst, bits_, num_words_*sizeof(word_t)); 
	dst += num_words_*sizeof(word_t);
	align(dst); 
}

void BitVector::deSerialize(char*& src) { 
	memcpy(&(num_bits_), src, sizeof(num_bits_)); 
    src += sizeof(num_bits_);
    memcpy(&(num_words_), src, sizeof(num_words_));
	src += sizeof(num_words_);
	bits_ = const_cast<word_t*>(reinterpret_cast<const word_t*>(src));
	src += num_words_*sizeof(word_t);
	align(src);
}

position_t BitVector::getSerializedSize() const {
    position_t size = sizeof(num_bits_) + sizeof(num_words_)+ num_words_*sizeof(word_t);
	sizeAlign(size);   //SizeAlign：64位对齐
	return size;
}


bool BitVector::readBit (const position_t pos) const{ 
    assert(pos < num_bits_);
    position_t word_id = pos / kWordSize; 
    position_t offset = pos & (kWordSize - 1); 
    return bits_[word_id] & (kMsbMask >> offset); 
}

static void setBit(std::vector<word_t>& bits, const position_t pos) { 
	assert(pos < (bits.size() * kWordSize));
	position_t word_id = pos / kWordSize;
	position_t offset = pos % kWordSize;
	bits[word_id] |= (kMsbMask >> offset);
}

position_t BitVector::distanceToNextSetBit (const position_t pos) const {
    assert(pos < num_bits_);
    position_t distance = 1;

    position_t word_id = (pos + 1) / kWordSize; 
    position_t offset = (pos + 1) % kWordSize; 

    //first word left-over bits
    word_t test_bits = bits_[word_id] << offset; 
    if (test_bits > 0) { 
	return (distance + __builtin_clzll(test_bits));
    } else {
	if (word_id == num_words_ - 1) 
	    return (num_bits_ - pos);
	distance += (kWordSize - offset);
    }

    while (word_id < num_words_ - 1) {
	word_id++;
	test_bits = bits_[word_id];
	if (test_bits > 0)
	    return (distance + __builtin_clzll(test_bits));
	distance += kWordSize;
    }
    return distance;
}


position_t BitVector::distanceToPrevSetBit (const position_t pos) const {
    assert(pos <= num_bits_);
    if (pos == 0) return 0;
    position_t distance = 1;

    position_t word_id = (pos - 1) / kWordSize;
    position_t offset = (pos - 1) % kWordSize;

    //first word left-over bits
    word_t test_bits = bits_[word_id] >> (kWordSize - 1 - offset);
    if (test_bits > 0) {
	return (distance + __builtin_ctzll(test_bits));
    } else {
	//if (word_id == 0)
	//return (offset + 1);
	distance += (offset + 1);
    }

    while (word_id > 0) {
	word_id--;
	test_bits = bits_[word_id];
	if (test_bits > 0)
	    return (distance + __builtin_ctzll(test_bits));
	distance += kWordSize;
    }
    return distance;
}
/*
void BitVector::show() {
    cout << "num_bits_ = " << num_bits_ <<endl;
    cout << "num_words_=" << num_words_<<endl;
    
	for (position_t i=0; i< num_words_ ; ++i) {
		cout <<bitset<64>(bits_[i]);
	}
	cout << endl;
}*/
#endif