#ifndef BITVECTORSELECT_H_
#define BITVECTORSELECT_H_

#include <assert.h>
#include <vector>
#include "BitVector.hpp"
#include "config.hpp"
#include "popcount.h"

class BitVectorSelect : public BitVector {
public:
    BitVectorSelect():sample_interval_(0), num_ones_(0), select_lut_(nullptr) {};
    BitVectorSelect(vector<uint64_t>& selectVector);
	//BitVectorSelect(char*& src);
    ~BitVectorSelect() {delete[] bits_; delete[] select_lut_;}
    position_t select(position_t rank) const;
    void serialize(char*& dst) const;
    //BitVectorSelect* deSerialize(char*& src);//读入模块
	void deSerialize(char*& src);
	position_t getSerializedSize() const; 
	position_t getOnesNum() {return num_ones_;}
	void showSelectLut();
private:
    position_t sample_interval_ = Select_Block_Size; //Interval
    position_t num_ones_ = 0;    
	position_t select_lut_size = 0;
    position_t* select_lut_ = nullptr; //select look-up table
private:
    void initSelectLut();
};

BitVectorSelect::BitVectorSelect(vector<uint64_t>& selectVector):BitVector(selectVector) 
{
	initSelectLut();
}

/*
BitVectorSelect::BitVectorSelect(char*& src) {
	memcpy(&(this->num_bits_), src, sizeof(this->num_bits_));
	src += sizeof(this->num_bits_);
	memcpy(&(this->num_ones_), src, sizeof(this->num_ones_));
	src += sizeof(this->num_ones_);
	this->bits_ = const_cast<word_t*>(reinterpret_cast<const word_t*>(src));
	src += this->getVectorBytes();
	this->select_lut_ = const_cast<position_t*>(reinterpret_cast<const position_t*>(src));
	src += this->getSelectLutSize();
	align(src);
	initSelectLut();
}
*/
position_t BitVectorSelect::select(position_t rank) const {
	assert(rank > 0);
	assert(rank <= num_ones_);
	position_t lut_idx = rank / sample_interval_;
	position_t rank_left = rank % sample_interval_;
	// The first slot in select_lut_ stores the position of the first 1 bit.
	// Slot i > 0 stores the position of (i * sample_interval_)-th 1 bit
	if (lut_idx == 0)
	    rank_left--;

	position_t pos = select_lut_[lut_idx];

	if (rank_left == 0)
	    return pos;

	position_t word_id = pos / kWordSize; 
	position_t offset = pos % kWordSize;
	if (offset == kWordSize - 1) {
	    word_id++;
	    offset = 0;
	} else {
	    offset++;
	}
	word_t word = bits_[word_id] << offset >> offset; //zero-out most significant bits
	position_t ones_count_in_word = popcount(word); 
	while (ones_count_in_word < rank_left) {
	    word_id++;
	    word = bits_[word_id];
	    rank_left -= ones_count_in_word;
	    ones_count_in_word = popcount(word);
	}
	return (word_id * kWordSize + select64_popcount_search(word, rank_left));
}

// | num_bits_(32) | num_words_(32)| bits_(64) | select_lut_size(32)|select_lut_
void BitVectorSelect::serialize(char*& dst) const {
	memcpy(dst, &num_bits_, sizeof(num_bits_)); 
	dst += sizeof(num_bits_); 
	memcpy(dst, &num_words_, sizeof(num_words_)); 
	dst += sizeof(num_words_); 
	memcpy(dst, bits_, num_words_*sizeof(word_t)); 
	dst += num_words_*sizeof(word_t);
	memcpy(dst, &select_lut_size, sizeof(select_lut_size)); 
	dst += sizeof(select_lut_size);
	memcpy(dst, select_lut_, select_lut_size*sizeof(position_t)); 
	dst += select_lut_size*sizeof(position_t);
	align(dst); 
    }

void BitVectorSelect::deSerialize(char*& src) {
	memcpy(&(num_bits_), src, sizeof(num_bits_)); 
    src += sizeof(num_bits_);
    memcpy(&(num_words_), src, sizeof(num_words_));
	src += sizeof(num_words_);
    bits_ = new word_t[num_words_];
    memcpy(bits_,src,num_words_*sizeof(word_t));
	src += num_words_*sizeof(word_t);
	memcpy(&(select_lut_size),src,sizeof(position_t));
	src += sizeof(select_lut_size);
	memcpy(select_lut_,src,select_lut_size*sizeof(position_t));
	src += select_lut_size*sizeof(position_t);
	align(src);
    }


position_t BitVectorSelect::getSerializedSize() const {
	position_t size = sizeof(num_bits_) + sizeof(num_words_)+ num_words_*sizeof(word_t)
		+ sizeof(select_lut_size) + select_lut_size*sizeof(position_t);
	sizeAlign(size);//size8字节对齐
	return size;
}

void BitVectorSelect::initSelectLut() {
	position_t num_words = num_bits_ / kWordSize;
	if (num_bits_ % kWordSize != 0)
	    num_words++;

	std::vector<position_t> select_lut_vector;
	select_lut_vector.push_back(0); //ASSERT: first bit is 1
	position_t sampling_ones = sample_interval_;
	position_t cumu_ones_upto_word = 0;
	for (position_t i = 0; i < num_words; i++) {
	    position_t num_ones_in_word = popcount(bits_[i]);
	    while (sampling_ones <= (cumu_ones_upto_word + num_ones_in_word)) {
		int diff = sampling_ones - cumu_ones_upto_word;
		position_t result_pos = i * kWordSize + select64_popcount_search(bits_[i], diff);
		select_lut_vector.push_back(result_pos);
		sampling_ones += sample_interval_;
	    }
	    cumu_ones_upto_word += popcount(bits_[i]);
	}

	num_ones_ = cumu_ones_upto_word;
	position_t num_samples = select_lut_vector.size();
	select_lut_ = new position_t[num_samples];
	for (position_t i = 0; i < num_samples; i++)
	    select_lut_[i] = select_lut_vector[i];
	select_lut_size = (num_ones_ / sample_interval_) +1;
}
/*
void BitVectorSelect::showSelectLut() {
	cout << select_lut_size<<endl;
	for(int i=0;i<select_lut_size;++i) {
		cout << select_lut_[i]<<" ";
	}
	cout << endl;
}*/
#endif